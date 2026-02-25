// include/pplua.hpp
//
// The pplua preprocessor engine.
// Reads input, recognises Lua blocks, executes them, and emits
// the resulting groff source to stdout.

#ifndef PPLUA_PPLUA_HPP
#define PPLUA_PPLUA_HPP

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "output_buffer.hpp"
#include "lroff.hpp"

#include <string>
#include <vector>
#include <iosfwd>

namespace pplua {

// =====================================================================
//  Preprocessor configuration
// =====================================================================
struct Config {
    // Block delimiters (request-style, at start of line).
    std::string block_open  = ".lua";
    std::string block_close = ".endlua";

    // Inline expression delimiters.
    // \lua'expr' — the open/close character can be any single char
    // that is the same on both sides (like eqn's $ delimiters).
    std::string inline_open  = "\\lua'";
    char        inline_close = '\'';

    // If true, emit .lf (line-file) directives so that groff
    // error messages refer to the original source line numbers.
    bool emit_lf = true;

    // If true, pass soelim-style .so requests through to groff
    // rather than processing them ourselves.
    bool pass_so = true;

    // Files to pre-execute before processing input (like a preamble).
    std::vector<std::string> preamble_files;

    // Extra Lua package.path entries.
    std::vector<std::string> lua_paths;
};

// =====================================================================
//  Preprocessor engine
// =====================================================================
class Preprocessor {
public:
    explicit Preprocessor(const Config& cfg = Config{});
    ~Preprocessor();

    /// Process a single input stream.  The filename is used for
    /// .lf directives and error messages.
    /// Returns 0 on success, non-zero on error.
    int process(std::istream& in,
                const std::string& filename = "<stdin>");

    /// Process a named file.
    int process_file(const std::string& path);

    /// Write all accumulated output to the given stream.
    void flush(std::ostream& out);

    /// Access the Lua state (e.g. for running preamble scripts).
    sol::state& lua() { return lua_; }

private:
    Config        cfg_;
    sol::state    lua_;
    OutputBuffer  output_;
    LroffLibrary  lroff_;

    // Tracking for error messages.
    std::string   current_file_;
    int           current_line_ = 0;

    /// Execute a block of Lua code; errors are reported to stderr.
    /// Returns true on success.
    bool exec_lua(const std::string& code,
                  const std::string& source_name,
                  int source_line);

    /// Expand inline \lua'…' expressions on a single line.
    /// Returns the line with expressions replaced by their results.
    std::string expand_inline(const std::string& line);

    /// Emit a .lf directive to keep groff's idea of line numbers
    /// in sync with the original source.
    void emit_lf(int line, const std::string& file);

    /// Pass a non-Lua line through to the output.
    void passthrough(const std::string& line);
};

} // namespace pplua

#endif // PPLUA_PPLUA_HPP
