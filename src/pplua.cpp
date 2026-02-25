// src/pplua.cpp
//
// The preprocessor engine: input parsing, Lua block extraction,
// inline expansion, and output assembly.

#include "pplua.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <regex>

namespace pplua {

// =================================================================
//  Construction / destruction
// =================================================================

Preprocessor::Preprocessor(const Config& cfg)
    : cfg_(cfg)
    , lua_()
    , output_()
    , lroff_(output_)
{
    // Open standard Lua libraries.
    lua_.open_libraries(
        sol::lib::base,
        sol::lib::string,
        sol::lib::table,
        sol::lib::math,
        sol::lib::io,
        sol::lib::os,
        sol::lib::package,
        sol::lib::utf8
    );

    // Extend package.path if the user asked.
    if (!cfg_.lua_paths.empty()) {
        std::string path = lua_["package"]["path"];
        for (auto& p : cfg_.lua_paths)
            path += ";" + p;
        lua_["package"]["path"] = path;
    }

    // Register the lroff library.
    lroff_.register_into(lua_);

    // Run preamble files.
    for (auto& pf : cfg_.preamble_files) {
        auto result = lua_.safe_script_file(pf,
            sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "pplua: error in preamble '"
                      << pf << "': " << err.what() << '\n';
        }
    }
}

Preprocessor::~Preprocessor() = default;

// =================================================================
//  exec_lua — run a Lua chunk, report errors
// =================================================================

bool Preprocessor::exec_lua(const std::string& code,
                             const std::string& source_name,
                             int source_line)
{
    // Prefix the chunk with a line directive so that Lua error
    // messages refer to the original source location.
    // We use the @filename convention for chunk names.
    std::string chunk_name = "@" + source_name + ":"
                             + std::to_string(source_line);

    auto result = lua_.safe_script(code,
        sol::script_pass_on_error, chunk_name);

    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "pplua: " << source_name
                  << ":" << source_line
                  << ": lua error: " << err.what() << '\n';
        return false;
    }

    // If the chunk returned a value, and it is a string or number,
    // emit it (like LuaTeX's \directlua returning a string).
    sol::object ret = result;
    if (ret.is<std::string>()) {
        output_.write(ret.as<std::string>());
    } else if (ret.is<int>()) {
        output_.write(std::to_string(ret.as<int>()));
    } else if (ret.is<double>()) {
        output_.write(std::to_string(ret.as<double>()));
    }
    // nil / table / function / etc. are silently ignored.

    return true;
}

// =================================================================
//  expand_inline — process \lua'…' on a single line
// =================================================================

std::string Preprocessor::expand_inline(const std::string& line)
{
    const std::string& open = cfg_.inline_open;
    const char          close = cfg_.inline_close;

    // Quick reject: if the line doesn't contain the open tag at all,
    // return it unchanged (the common case — fast path).
    if (line.find(open) == std::string::npos)
        return line;

    std::string result;
    result.reserve(line.size());

    std::size_t pos = 0;
    while (pos < line.size()) {
        // Find next occurrence of the open delimiter.
        std::size_t start = line.find(open, pos);
        if (start == std::string::npos) {
            // No more inline expressions; copy the rest.
            result.append(line, pos, std::string::npos);
            break;
        }

        // Copy everything before the open delimiter.
        result.append(line, pos, start - pos);

        // Skip past the open delimiter.
        std::size_t expr_start = start + open.size();

        // Find the matching close delimiter, respecting nesting
        // of the same character for single-character delimiters.
        // For the default case (\lua'…'), we simply find the next
        // unescaped close character.
        std::size_t expr_end = std::string::npos;
        int depth = 1;
        for (std::size_t i = expr_start; i < line.size(); ++i) {
            if (line[i] == '\\' && i + 1 < line.size()) {
                ++i;  // skip escaped character
                continue;
            }
            if (line[i] == close) {
                --depth;
                if (depth == 0) {
                    expr_end = i;
                    break;
                }
            }
        }

        if (expr_end == std::string::npos) {
            // Unterminated inline expression — pass through verbatim.
            std::cerr << "pplua: " << current_file_
                      << ":" << current_line_
                      << ": warning: unterminated \\lua expression\n";
            result.append(line, start, std::string::npos);
            break;
        }

        // Extract the Lua expression.
        std::string expr = line.substr(expr_start,
                                        expr_end - expr_start);

        // Wrap in "return (…)" so that the expression's value
        // is captured.
        std::string chunk = "return tostring(" + expr + ")";

        auto lua_result = lua_.safe_script(chunk,
            sol::script_pass_on_error,
            "@" + current_file_ + ":" +
                std::to_string(current_line_) + ":inline");

        if (lua_result.valid()) {
            sol::object obj = lua_result;
            if (obj.is<std::string>())
                result += obj.as<std::string>();
        } else {
            sol::error err = lua_result;
            std::cerr << "pplua: " << current_file_
                      << ":" << current_line_
                      << ": inline lua error: "
                      << err.what() << '\n';
            // leave the expression site empty on error
        }

        pos = expr_end + 1;   // skip past close delimiter
    }

    return result;
}

// =================================================================
//  emit_lf — keep groff line numbers in sync
// =================================================================

void Preprocessor::emit_lf(int line, const std::string& file) {
    if (cfg_.emit_lf)
        output_.writeln(".lf " + std::to_string(line)
                        + " " + file);
}

// =================================================================
//  passthrough — emit a non-Lua line
// =================================================================

void Preprocessor::passthrough(const std::string& line) {
    output_.writeln(line);
}

// =================================================================
//  process — main loop over an input stream
// =================================================================

int Preprocessor::process(std::istream& in,
                           const std::string& filename)
{
    current_file_ = filename;
    current_line_ = 0;

    bool in_lua_block = false;
    int  lua_block_start = 0;
    std::ostringstream lua_buf;

    std::string line;
    while (std::getline(in, line)) {
        ++current_line_;

        if (in_lua_block) {
            // ---- inside a .lua … .endlua block ----

            // Check for the closing delimiter.
            // We trim leading whitespace for the comparison, but
            // the canonical form is exactly ".endlua" at column 0.
            std::string trimmed = line;
            // Strip leading whitespace.
            auto first = trimmed.find_first_not_of(" \t");
            if (first != std::string::npos)
                trimmed = trimmed.substr(first);

            if (trimmed == cfg_.block_close
                || trimmed.rfind(cfg_.block_close + " ", 0) == 0
                || trimmed.rfind(cfg_.block_close + "\t", 0) == 0)
            {
                // End of Lua block.
                in_lua_block = false;

                std::string code = lua_buf.str();
                lua_buf.str("");
                lua_buf.clear();

                exec_lua(code, filename, lua_block_start);

                // Re-sync groff line counter.
                emit_lf(current_line_ + 1, filename);
            } else {
                // Accumulate Lua source.
                lua_buf << line << '\n';
            }
        } else {
            // ---- outside a Lua block ----

            // Check for block-open delimiter.
            std::string trimmed = line;
            auto first = trimmed.find_first_not_of(" \t");
            if (first != std::string::npos)
                trimmed = trimmed.substr(first);

            if (trimmed == cfg_.block_open
                || trimmed.rfind(cfg_.block_open + " ", 0) == 0
                || trimmed.rfind(cfg_.block_open + "\t", 0) == 0)
            {
                // Start of a Lua block.
                in_lua_block = true;
                lua_block_start = current_line_ + 1;

                // Anything after ".lua " on the same line is the
                // first line of Lua code (convenience for one-liners).
                std::string rest;
                if (trimmed.size() > cfg_.block_open.size()) {
                    rest = trimmed.substr(cfg_.block_open.size() + 1);
                    // If the one-liner also contains .endlua, handle
                    // that (unlikely but be safe):
                    auto ec = rest.find(cfg_.block_close);
                    if (ec != std::string::npos) {
                        std::string code = rest.substr(0, ec);
                        exec_lua(code, filename, current_line_);
                        in_lua_block = false;
                        emit_lf(current_line_ + 1, filename);
                        continue;
                    }
                    lua_buf << rest << '\n';
                }
                continue;
            }

            // Not a block delimiter — handle inline expressions
            // and pass through.
            std::string expanded = expand_inline(line);
            passthrough(expanded);
        }
    }

    // Check for unterminated block.
    if (in_lua_block) {
        std::cerr << "pplua: " << filename
                  << ":" << lua_block_start
                  << ": error: unterminated .lua block\n";
        return 1;
    }

    return 0;
}

// =================================================================
//  process_file — convenience wrapper
// =================================================================

int Preprocessor::process_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "pplua: cannot open '" << path << "'\n";
        return 1;
    }
    return process(f, path);
}

// =================================================================
//  flush — write accumulated output to a stream
// =================================================================

void Preprocessor::flush(std::ostream& out) {
    out << output_.contents();
    output_.clear();
}

} // namespace pplua
