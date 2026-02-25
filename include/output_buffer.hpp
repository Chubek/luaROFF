// include/output_buffer.hpp
//
// Output buffering and diversion management for pplua.
// All groff text emitted by Lua code flows through these classes.

#ifndef PPLUA_OUTPUT_BUFFER_HPP
#define PPLUA_OUTPUT_BUFFER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>

namespace pplua {

// =====================================================================
//  OutputBuffer — linear accumulator for groff source text
// =====================================================================
class OutputBuffer {
public:
    OutputBuffer() = default;

    /// Append raw text (no trailing newline).
    void write(const std::string& text) { buf_ << text; }

    /// Append raw text followed by exactly one newline.
    void writeln(const std::string& text) { buf_ << text << '\n'; }

    /// Append a bare newline (blank line = paragraph break in groff).
    void blank_line() { buf_ << '\n'; }

    /// Return everything accumulated so far.
    std::string contents() const { return buf_.str(); }

    /// Discard all accumulated text.
    void clear() {
        buf_.str("");
        buf_.clear();           // clear error flags too
    }

    bool  empty() const { return buf_.str().empty(); }
    std::size_t size() const { return buf_.str().size(); }

private:
    std::ostringstream buf_;
};

// =====================================================================
//  DivertManager — named diversions, nestable
//
//  When no diversion is active, writes go straight to the main
//  OutputBuffer.  divert_begin("foo") pushes "foo" onto the stack
//  and redirects writes into that named buffer.  divert_end() pops.
// =====================================================================
class DivertManager {
public:
    explicit DivertManager(OutputBuffer& main_output)
        : main_(main_output) {}

    // -- stack operations --
    void begin(const std::string& name) {
        stack_.push_back(name);
        divs_.try_emplace(name);        // create if absent
    }

    void end() {
        if (stack_.empty())
            throw std::runtime_error("divert_end: no active diversion");
        stack_.pop_back();
    }

    // -- writing (routed to current target) --
    void write(const std::string& text) {
        if (stack_.empty()) main_.write(text);
        else                divs_[stack_.back()] << text;
    }

    void writeln(const std::string& text) {
        if (stack_.empty()) main_.writeln(text);
        else                divs_[stack_.back()] << text << '\n';
    }

    void blank_line() {
        if (stack_.empty()) main_.blank_line();
        else                divs_[stack_.back()] << '\n';
    }

    // -- query / retrieve --
    std::string get(const std::string& name) const {
        auto it = divs_.find(name);
        return (it != divs_.end()) ? it->second.str() : "";
    }

    bool exists(const std::string& name) const {
        return divs_.count(name) > 0;
    }

    void clear(const std::string& name) {
        auto it = divs_.find(name);
        if (it != divs_.end()) {
            it->second.str("");
            it->second.clear();
        }
    }

    void erase(const std::string& name) {
        divs_.erase(name);
    }

    bool        is_diverting()  const { return !stack_.empty(); }
    std::string current_name()  const {
        return stack_.empty() ? "" : stack_.back();
    }

private:
    OutputBuffer&                               main_;
    std::vector<std::string>                    stack_;
    std::map<std::string, std::ostringstream>   divs_;
};

} // namespace pplua

#endif // PPLUA_OUTPUT_BUFFER_HPP
