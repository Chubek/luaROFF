// include/lroff.hpp
//
// The lroff Lua library — C++ implementation registered via sol2.
// Provides groff-aware facilities to embedded Lua code.

#ifndef PPLUA_LROFF_HPP
#define PPLUA_LROFF_HPP

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "output_buffer.hpp"

#include <string>
#include <map>
#include <vector>
#include <optional>

namespace pplua {

// =====================================================================
//  DocumentState — bookkeeping mirror of troff state
//
//  Because we are a *preprocessor* (not an embedded engine like
//  LuaTeX), we cannot interrogate the formatter at run-time.
//  Instead we maintain a shadow copy that Lua code can query and
//  that stays in sync as long as all state changes go through lroff.
// =====================================================================
struct DocumentState {
    std::map<std::string, int>         number_registers;
    std::map<std::string, std::string> string_registers;

    std::string font_family  = "T";     // e.g. Times
    std::string font_style   = "R";     // R, B, I, BI, …
    int         point_size   = 10;
    int         vert_spacing = 12;

    // auto-increment counter for unique names
    int unique_counter = 0;
    std::string unique_name(const std::string& pfx = "_lua") {
        return pfx + std::to_string(++unique_counter);
    }
};

// =====================================================================
//  LroffLibrary
// =====================================================================
class LroffLibrary {
public:
    explicit LroffLibrary(OutputBuffer& output);

    /// Register the "lroff" table into a Lua state.
    void register_into(sol::state& lua);

    /// Accessors used by the preprocessor engine.
    OutputBuffer&   output()      { return output_; }
    DivertManager&  diversions()  { return diverts_; }
    DocumentState&  state()       { return state_; }

private:
    OutputBuffer&   output_;
    DivertManager   diverts_;
    DocumentState   state_;

    /* ---- helpers called from Lua ---- */

    // output
    void        emit(const std::string& text);
    void        emitln(const std::string& text);
    void        request(const std::string& req);
    void        request_with(const std::string& req,
                             const std::string& args);
    void        comment(const std::string& text);
    void        blank();

    // escaping
    std::string escape(const std::string& text);
    std::string inline_escape(const std::string& ec,
                              const std::string& arg);

    // fonts / sizes
    void font(const std::string& f);
    void font_bold();
    void font_italic();
    void font_roman();
    void font_bold_italic();
    void font_previous();
    void size(int pts);
    void size_relative(int delta);

    // number registers
    void        nr_set (const std::string& n, int v);
    void        nr_incr(const std::string& n, int d);
    sol::object nr_get (sol::this_state ts, const std::string& n);
    std::string nr_ref (const std::string& n);

    // string registers
    void        ds_set(const std::string& n, const std::string& v);
    sol::object ds_get(sol::this_state ts, const std::string& n);
    std::string ds_ref(const std::string& n);

    // diversions
    void        divert_begin(const std::string& name);
    void        divert_end();
    void        divert_emit(const std::string& name);
    std::string divert_get(const std::string& name);
    void        divert_clear(const std::string& name);

    // macros
    void macro_define    (const std::string& name,
                          const std::string& body);
    void macro_define_lua(const std::string& name,
                          const std::string& lua_code);

    // inline styling (return strings, don't emit)
    std::string styled      (const std::string& fc,
                             const std::string& text);
    std::string bold         (const std::string& text);
    std::string italic       (const std::string& text);
    std::string bold_italic  (const std::string& text);
    std::string mono         (const std::string& text);
    std::string special_char (const std::string& name);

    // document structure helpers (ms / man macros)
    void paragraph  (const std::string& macro);
    void section    (const std::string& title);
    void subsection (const std::string& title);
    void title      (const std::string& t);
    void author     (const std::string& a);
    void display_begin(const std::string& type);
    void display_end();

    // compound structures
    void table_emit   (const std::vector<std::string>&              hdr,
                       const std::vector<std::vector<std::string>>& rows,
                       const std::string& fmt);
    void bullet_list  (const std::vector<std::string>& items);
    void numbered_list(const std::vector<std::string>& items);
    void def_list     (const std::vector<std::pair<std::string,
                                                    std::string>>& items);

    // utility
    std::string unique (const std::string& prefix);
    std::string version();
};

} // namespace pplua

#endif // PPLUA_LROFF_HPP
