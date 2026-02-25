// src/main.cpp
//
// pplua — a Lua preprocessor for the groff pipeline.
//
// Usage:
//   pplua [options] [file ...]
//
// If no files are given, reads from stdin.
// Output goes to stdout (suitable for piping into groff).
//
// Options:
//   -e CODE        Execute CODE before processing input.
//   -l FILE        Execute a Lua preamble file before processing.
//   -I PATH        Add PATH to Lua's package.path.
//   -D NAME=VALUE  Set a Lua global variable (string).
//   -n             Suppress .lf line directives.
//   -V             Print version and exit.
//   -h             Print help and exit.

#define SOL_ALL_SAFETIES_ON 1

#include "pplua.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

static void usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [options] [file ...]\n"
        << "\n"
        << "A Lua preprocessor for the groff pipeline.\n"
        << "\n"
        << "Options:\n"
        << "  -e CODE        Execute Lua CODE before processing input.\n"
        << "  -l FILE        Run a Lua preamble file.\n"
        << "  -I PATH        Add PATH to Lua package.path.\n"
        << "  -D NAME=VALUE  Define a Lua global variable (string).\n"
        << "  -n             Suppress .lf line-number directives.\n"
        << "  -V             Print version and exit.\n"
        << "  -h             Print this help and exit.\n"
        << "\n"
        << "Input is read from files (or stdin if none given).\n"
        << "Output is written to stdout.\n"
        << "\n"
        << "Lua blocks are delimited by .lua / .endlua requests.\n"
        << "Inline expressions use \\lua'expr' syntax.\n";
}

int main(int argc, char* argv[])
{
    pplua::Config cfg;

    std::vector<std::string>               input_files;
    std::vector<std::string>               exec_before;  // -e chunks
    std::vector<std::pair<std::string,
                          std::string>>    defines;      // -D pairs

    // ---- parse arguments ----
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return 0;
        }
        if (arg == "-V" || arg == "--version") {
            std::cout << "pplua 0.1.0\n";
            return 0;
        }
        if (arg == "-n") {
            cfg.emit_lf = false;
            continue;
        }

        // Options that take a following argument.
        auto need_arg = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "pplua: " << name
                          << " requires an argument\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if (arg == "-e") {
            exec_before.push_back(need_arg("-e"));
            continue;
        }
        if (arg == "-l") {
            cfg.preamble_files.push_back(need_arg("-l"));
            continue;
        }
        if (arg == "-I") {
            std::string p = need_arg("-I");
            // Append Lua path pattern.
            cfg.lua_paths.push_back(p + "/?.lua");
            cfg.lua_paths.push_back(p + "/?/init.lua");
            continue;
        }
        if (arg == "-D") {
            std::string d = need_arg("-D");
            auto eq = d.find('=');
            if (eq == std::string::npos) {
                // -D NAME  (no value, set to "1")
                defines.emplace_back(d, "1");
            } else {
                defines.emplace_back(d.substr(0, eq),
                                     d.substr(eq + 1));
            }
            continue;
        }

        if (arg == "--") {
            // Everything after -- is a filename.
            for (++i; i < argc; ++i)
                input_files.emplace_back(argv[i]);
            break;
        }

        if (arg.size() > 1 && arg[0] == '-') {
            std::cerr << "pplua: unknown option: " << arg << '\n';
            usage(argv[0]);
            return 1;
        }

        // Positional: input file.
        input_files.push_back(arg);
    }

    // ---- build the preprocessor ----
    pplua::Preprocessor pp(cfg);

    // Set -D globals.
    for (auto& [name, value] : defines)
        pp.lua()[name] = value;

    // Run -e chunks.
    for (auto& code : exec_before) {
        auto result = pp.lua().safe_script(code,
            sol::script_pass_on_error, "@-e");
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "pplua: -e: " << err.what() << '\n';
            return 1;
        }
    }

    // ---- process input ----
    int rc = 0;
    if (input_files.empty()) {
        // Read from stdin.
        rc = pp.process(std::cin, "<stdin>");
    } else {
        for (auto& path : input_files) {
            if (path == "-") {
                rc |= pp.process(std::cin, "<stdin>");
            } else {
                rc |= pp.process_file(path);
            }
        }
    }

    // ---- emit output ----
    pp.flush(std::cout);

    return rc;
}
