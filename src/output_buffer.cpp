// src/output_buffer.cpp
#include "output_buffer.hpp"
#include <map>
#include <stdexcept>

namespace pplua {

// ---- OutputBuffer ----

void OutputBuffer::write(const std::string& text) {
    buf_ << text;
}

void OutputBuffer::writeln(const std::string& text) {
    buf_ << text << '\n';
}

void OutputBuffer::blank_line() {
    buf_ << '\n';
}

std::string OutputBuffer::contents() const {
    return buf_.str();
}

void OutputBuffer::clear() {
    buf_.str("");
    buf_.clear();
}

bool OutputBuffer::empty() const {
    return buf_.str().empty();
}

std::size_t OutputBuffer::size() const {
    return buf_.str().size();
}

// ---- DivertManager ----

DivertManager::DivertManager(OutputBuffer& main_output)
    : main_output_(main_output) {}

void DivertManager::begin(const std::string& name) {
    diversion_stack_.push_back(name);
    // Create the buffer if it doesn't exist; otherwise we append.
    if (diversions_.find(name) == diversions_.end()) {
        diversions_.emplace(name, std::ostringstream{});
    }
}

void DivertManager::end() {
    if (diversion_stack_.empty()) {
        throw std::runtime_error(
            "lroff.divert_end: no diversion is active");
    }
    diversion_stack_.pop_back();
}

std::string DivertManager::get(const std::string& name) const {
    auto it = diversions_.find(name);
    if (it == diversions_.end()) {
        return "";
    }
    return it->second.str();
}

bool DivertManager::exists(const std::string& name) const {
    return diversions_.find(name) != diversions_.end();
}

void DivertManager::clear(const std::string& name) {
    auto it = diversions_.find(name);
    if (it != diversions_.end()) {
        it->second.str("");
        it->second.clear();
    }
}

void DivertManager::write(const std::string& text) {
    current_stream() << text;
}

void DivertManager::writeln(const std::string& text) {
    current_stream() << text << '\n';
}

bool DivertManager::is_diverting() const {
    return !diversion_stack_.empty();
}

std::string DivertManager::current_diversion_name() const {
    if (diversion_stack_.empty()) return "";
    return diversion_stack_.back();
}

std::ostream& DivertManager::current_stream() {
    if (diversion_stack_.empty()) {
        // Write to a temporary stringstream that we'll pull from
        // Actually, write directly into main_output_
        // We need a stream interface, so we use a small trick:
        // We'll store a reference. But OutputBuffer doesn't expose
        // a stream. Let's handle this differently.
        //
        // For the main output path, we return a special stream
        // that appends to main_output_.
        // Since OutputBuffer uses an ostringstream internally,
        // we have a design tension. Let's just use the diversions
        // map with a special key for "main".
        //
        // Actually, let's keep it simple: when not diverting,
        // callers should use main_output_ directly.
        // This function is only called internally; let's define
        // the contract: if not diverting, we write to main_output_
        // via its write method, but we need a stream... Let's
        // restructure slightly.

        // We'll use a static approach: buffer into a local stream,
        // but that's wrong. Let's just make this return an
        // ostringstream reference and handle main output differently.
        throw std::logic_error(
            "current_stream() called when not diverting; "
            "use main_output_ directly");
    }
    return diversions_.at(diversion_stack_.back());
}

} // namespace pplua
