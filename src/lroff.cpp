// src/lroff.cpp
//
// Full implementation of every function in the lroff library.

#include "lroff.hpp"

#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace pplua {

static constexpr const char* PPLUA_VERSION = "pplua 0.1.0";

LroffLibrary::LroffLibrary(OutputBuffer& output)
    : output_(output), diverts_(output), state_() {}

// =================================================================
//  register_into — bind every C++ helper into the "lroff" table
// =================================================================
void LroffLibrary::register_into(sol::state& lua)
{
    sol::table L = lua.create_named_table("lroff");

    // ---- version ----
    L["_VERSION"] = PPLUA_VERSION;

    // ---- output ----
    L.set_function("emit",    [this](const std::string& t){ emit(t); });
    L.set_function("emitln",  [this](const std::string& t){ emitln(t); });
    L.set_function("request", [this](const std::string& r){ request(r); });
    L.set_function("request_with",
        [this](const std::string& r, const std::string& a){
            request_with(r, a);
        });
    L.set_function("comment", [this](const std::string& t){ comment(t); });
    L.set_function("blank",   [this](){ blank(); });

    // ---- escaping ----
    L.set_function("escape",
        [this](const std::string& t) -> std::string {
            return escape(t);
        });
    L.set_function("inline_escape",
        [this](const std::string& e, const std::string& a)
            -> std::string {
            return inline_escape(e, a);
        });

    // ---- fonts / sizes ----
    L.set_function("font",           [this](const std::string& f){ font(f); });
    L.set_function("font_bold",      [this](){ font_bold(); });
    L.set_function("font_italic",    [this](){ font_italic(); });
    L.set_function("font_roman",     [this](){ font_roman(); });
    L.set_function("font_bold_italic",[this](){ font_bold_italic(); });
    L.set_function("font_previous",  [this](){ font_previous(); });
    L.set_function("size",           [this](int p){ size(p); });
    L.set_function("size_relative",  [this](int d){ size_relative(d); });

    // ---- number registers ----
    L.set_function("nr_set",
        [this](const std::string& n, int v){ nr_set(n, v); });
    L.set_function("nr_incr",
        [this](const std::string& n, int d){ nr_incr(n, d); });
    L.set_function("nr_get",
        [this](sol::this_state ts, const std::string& n)
            -> sol::object { return nr_get(ts, n); });
    L.set_function("nr_ref",
        [this](const std::string& n) -> std::string {
            return nr_ref(n);
        });

    // ---- string registers ----
    L.set_function("ds_set",
        [this](const std::string& n, const std::string& v){
            ds_set(n, v);
        });
    L.set_function("ds_get",
        [this](sol::this_state ts, const std::string& n)
            -> sol::object { return ds_get(ts, n); });
    L.set_function("ds_ref",
        [this](const std::string& n) -> std::string {
            return ds_ref(n);
        });

    // ---- diversions ----
    L.set_function("divert_begin",
        [this](const std::string& n){ divert_begin(n); });
    L.set_function("divert_end",  [this](){ divert_end(); });
    L.set_function("divert_emit",
        [this](const std::string& n){ divert_emit(n); });
    L.set_function("divert_get",
        [this](const std::string& n) -> std::string {
            return divert_get(n);
        });
    L.set_function("divert_clear",
        [this](const std::string& n){ divert_clear(n); });

    // ---- macros ----
    L.set_function("macro_define",
        [this](const std::string& n, const std::string& b){
            macro_define(n, b);
        });
    L.set_function("macro_define_lua",
        [this](const std::string& n, const std::string& c){
            macro_define_lua(n, c);
        });

    // ---- inline styling (return strings) ----
    L.set_function("styled",
        [this](const std::string& f, const std::string& t)
            -> std::string { return styled(f, t); });
    L.set_function("bold",
        [this](const std::string& t) -> std::string {
            return bold(t);
        });
    L.set_function("italic",
        [this](const std::string& t) -> std::string {
            return italic(t);
        });
    L.set_function("bold_italic",
        [this](const std::string& t) -> std::string {
            return bold_italic(t);
        });
    L.set_function("mono",
        [this](const std::string& t) -> std::string {
            return mono(t);
        });
    L.set_function("special_char",
        [this](const std::string& n) -> std::string {
            return special_char(n);
        });

    // ---- document structure ----
    L.set_function("paragraph",
        [this](sol::optional<std::string> m){
            paragraph(m.value_or("PP"));
        });
    L.set_function("section",
        [this](const std::string& t){ section(t); });
    L.set_function("subsection",
        [this](const std::string& t){ subsection(t); });
    L.set_function("title",
        [this](const std::string& t){ title(t); });
    L.set_function("author",
        [this](const std::string& a){ author(a); });
    L.set_function("display_begin",
        [this](sol::optional<std::string> t){
            display_begin(t.value_or(""));
        });
    L.set_function("display_end", [this](){ display_end(); });

    // ---- compound structures ----
    //
    // These accept Lua tables and convert them to C++ vectors
    // at the binding boundary.

    L.set_function("table",
        [this](const sol::table& hdr_tbl,
               const sol::table& row_tbl,
               sol::optional<std::string> fmt)
    {
        std::vector<std::string> hdr;
        hdr.reserve(hdr_tbl.size());
        for (auto& kv : hdr_tbl)
            hdr.push_back(kv.second.as<std::string>());

        std::vector<std::vector<std::string>> rows;
        rows.reserve(row_tbl.size());
        for (auto& kv : row_tbl) {
            sol::table rt = kv.second.as<sol::table>();
            std::vector<std::string> row;
            row.reserve(rt.size());
            for (auto& cell : rt)
                row.push_back(cell.second.as<std::string>());
            rows.push_back(std::move(row));
        }
        table_emit(hdr, rows, fmt.value_or(""));
    });

    L.set_function("bullet_list",
        [this](const sol::table& tbl) {
            std::vector<std::string> items;
            items.reserve(tbl.size());
            for (auto& kv : tbl)
                items.push_back(kv.second.as<std::string>());
            bullet_list(items);
        });

    L.set_function("numbered_list",
        [this](const sol::table& tbl) {
            std::vector<std::string> items;
            items.reserve(tbl.size());
            for (auto& kv : tbl)
                items.push_back(kv.second.as<std::string>());
            numbered_list(items);
        });

    L.set_function("def_list",
        [this](const sol::table& tbl) {
            std::vector<std::pair<std::string,std::string>> items;
            for (auto& kv : tbl) {
                sol::table pair = kv.second.as<sol::table>();
                std::string term = pair[1].get<std::string>();
                std::string def  = pair[2].get<std::string>();
                items.emplace_back(std::move(term), std::move(def));
            }
            def_list(items);
        });

    // ---- utility ----
    L.set_function("unique",
        [this](sol::optional<std::string> pfx) -> std::string {
            return unique(pfx.value_or("_lua"));
        });
    L.set_function("version",
        [this]() -> std::string { return version(); });

    // ================================================================
    //  Pure-Lua convenience wrappers (defined on top of C++ bindings)
    // ================================================================

    lua.script(R"lua(
        -- formatted emit  (like C printf, uses string.format)
        function lroff.printf(fmt, ...)
            lroff.emit(string.format(fmt, ...))
        end

        function lroff.printfln(fmt, ...)
            lroff.emitln(string.format(fmt, ...))
        end

        -- apply fn(v) to every element; emit non-nil returns
        function lroff.map(tbl, fn)
            for _, v in ipairs(tbl) do
                local r = fn(v)
                if r ~= nil then lroff.emitln(tostring(r)) end
            end
        end

        -- call fn(k,v) for each pair (no output)
        function lroff.foreach(tbl, fn)
            for k, v in pairs(tbl) do fn(k, v) end
        end

        -- scoped font change
        function lroff.with_font(f, fn)
            lroff.font(f);  fn();  lroff.font_previous()
        end

        -- scoped size change
        function lroff.with_size(s, fn)
            local old = lroff._state_ps or 10
            lroff.size(s);  fn();  lroff.size(old)
        end

        -- emit a groff conditional:  .if cond \{ body \}
        function lroff.groff_if(cond, body)
            lroff.emitln(".if " .. cond .. " \\{")
            lroff.emitln(body)
            lroff.emitln(".\\}")
        end

        -- emit a groff .while loop
        function lroff.groff_while(cond, body)
            lroff.emitln(".while " .. cond .. " \\{")
            lroff.emitln(body)
            lroff.emitln(".\\}")
        end

        -- build a string from repeated calls (like table.concat)
        function lroff.concat(tbl, sep)
            sep = sep or ""
            local parts = {}
            for _, v in ipairs(tbl) do parts[#parts+1] = tostring(v) end
            return table.concat(parts, sep)
        end

        -- indent helper: emit .RS / block / .RE
        function lroff.indented(fn)
            lroff.request("RS")
            fn()
            lroff.request("RE")
        end
    )lua");
}

// =================================================================
//  Output
// =================================================================

void LroffLibrary::emit(const std::string& text)   { diverts_.write(text); }
void LroffLibrary::emitln(const std::string& text)  { diverts_.writeln(text); }
void LroffLibrary::blank()                          { diverts_.blank_line(); }

void LroffLibrary::request(const std::string& req) {
    diverts_.writeln("." + req);
}

void LroffLibrary::request_with(const std::string& req,
                                 const std::string& args) {
    diverts_.writeln("." + req + " " + args);
}

void LroffLibrary::comment(const std::string& text) {
    // groff comment: .\" text
    diverts_.writeln(".\\\" " + text);
}

// =================================================================
//  Escaping
// =================================================================

std::string LroffLibrary::escape(const std::string& text)
{
    std::string out;
    out.reserve(text.size() + text.size() / 4);

    bool at_line_start = true;
    for (char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            at_line_start = false;
            break;
        case '\n':
            out += '\n';
            at_line_start = true;
            break;
        case '.':
            if (at_line_start) out += "\\&";
            out += '.';
            at_line_start = false;
            break;
        case '\'':
            if (at_line_start) out += "\\&";
            out += '\'';
            at_line_start = false;
            break;
        default:
            out += c;
            at_line_start = false;
            break;
        }
    }
    return out;
}

std::string LroffLibrary::inline_escape(const std::string& ec,
                                         const std::string& arg)
{
    // bracket form for safety: \X'arg' or \f[arg]
    // Use bracket form when arg is > 1 char; otherwise short form.
    if (arg.size() <= 1 && ec.size() == 1)
        return "\\" + ec + arg;
    return "\\" + ec + "[" + arg + "]";
}

// =================================================================
//  Fonts / Sizes
// =================================================================

void LroffLibrary::font(const std::string& f) {
    state_.font_style = f;
    request_with("ft", f);
}
void LroffLibrary::font_bold()        { font("B"); }
void LroffLibrary::font_italic()      { font("I"); }
void LroffLibrary::font_roman()       { font("R"); }
void LroffLibrary::font_bold_italic() { font("BI"); }
void LroffLibrary::font_previous() {
    // .ft with no argument = previous font in groff
    request("ft");
}

void LroffLibrary::size(int pts) {
    state_.point_size = pts;
    request_with("ps", std::to_string(pts));
}

void LroffLibrary::size_relative(int delta) {
    state_.point_size += delta;
    std::string arg = (delta >= 0)
        ? ("+" + std::to_string(delta))
        : std::to_string(delta);
    request_with("ps", arg);
}

// =================================================================
//  Number Registers
// =================================================================

void LroffLibrary::nr_set(const std::string& n, int v) {
    state_.number_registers[n] = v;
    // .nr name value
    request_with("nr", n + " " + std::to_string(v));
}

void LroffLibrary::nr_incr(const std::string& n, int d) {
    state_.number_registers[n] += d;
    std::string sign = (d >= 0) ? "+" : "";
    request_with("nr", n + " " + sign + std::to_string(d));
}

sol::object LroffLibrary::nr_get(sol::this_state ts,
                                  const std::string& n) {
    auto it = state_.number_registers.find(n);
    if (it == state_.number_registers.end())
        return sol::make_object(ts, sol::lua_nil);
    return sol::make_object(ts, it->second);
}

std::string LroffLibrary::nr_ref(const std::string& n) {
    if (n.size() <= 2) {
        if (n.size() == 1) return "\\n" + n;
        return "\\n(" + n;
    }
    return "\\n[" + n + "]";
}

// =================================================================
//  String Registers
// =================================================================

void LroffLibrary::ds_set(const std::string& n,
                            const std::string& v) {
    state_.string_registers[n] = v;
    diverts_.writeln(".ds " + n + " " + v);
}

sol::object LroffLibrary::ds_get(sol::this_state ts,
                                  const std::string& n) {
    auto it = state_.string_registers.find(n);
    if (it == state_.string_registers.end())
        return sol::make_object(ts, sol::lua_nil);
    return sol::make_object(ts, it->second);
}

std::string LroffLibrary::ds_ref(const std::string& n) {
    if (n.size() <= 2) {
        if (n.size() == 1) return "\\*" + n;
        return "\\*(" + n;
    }
    return "\\*[" + n + "]";
}

// =================================================================
//  Diversions (preprocessor-level, independent of groff diversions)
// =================================================================

void LroffLibrary::divert_begin(const std::string& name) {
    diverts_.begin(name);
}
void LroffLibrary::divert_end() {
    diverts_.end();
}
void LroffLibrary::divert_emit(const std::string& name) {
    diverts_.write(diverts_.get(name));
}
std::string LroffLibrary::divert_get(const std::string& name) {
    return diverts_.get(name);
}
void LroffLibrary::divert_clear(const std::string& name) {
    diverts_.clear(name);
}

// =================================================================
//  Macros
// =================================================================

void LroffLibrary::macro_define(const std::string& name,
                                 const std::string& body)
{
    diverts_.writeln(".de " + name);
    diverts_.write(body);
    if (!body.empty() && body.back() != '\n')
        diverts_.write("\n");
    diverts_.writeln("..");
}

void LroffLibrary::macro_define_lua(const std::string& name,
                                     const std::string& lua_code)
{
    diverts_.writeln(".de " + name);
    diverts_.writeln(".lua");
    diverts_.write(lua_code);
    if (!lua_code.empty() && lua_code.back() != '\n')
        diverts_.write("\n");
    diverts_.writeln(".endlua");
    diverts_.writeln("..");
}

// =================================================================
//  Inline styling helpers (return strings — never emit directly)
// =================================================================

std::string LroffLibrary::styled(const std::string& fc,
                                   const std::string& text)
{
    // Use bracket form for multi-char font names
    if (fc.size() > 1)
        return "\\f[" + fc + "]" + text + "\\f[P]";
    return "\\f" + fc + text + "\\fP";
}

std::string LroffLibrary::bold(const std::string& t)
    { return styled("B", t); }
std::string LroffLibrary::italic(const std::string& t)
    { return styled("I", t); }
std::string LroffLibrary::bold_italic(const std::string& t)
    { return styled("BI", t); }
std::string LroffLibrary::mono(const std::string& t)
    { return styled("CR", t); }

std::string LroffLibrary::special_char(const std::string& name) {
    if (name.size() <= 2) return "\\(" + name;
    return "\\[" + name + "]";
}

// =================================================================
//  Document structure helpers
// =================================================================

void LroffLibrary::paragraph(const std::string& macro) {
    request(macro);
}

void LroffLibrary::section(const std::string& title) {
    diverts_.writeln(".SH");
    diverts_.writeln(title);
}

void LroffLibrary::subsection(const std::string& title) {
    diverts_.writeln(".SS");
    diverts_.writeln(title);
}

void LroffLibrary::title(const std::string& t) {
    diverts_.writeln(".TL");
    diverts_.writeln(t);
}

void LroffLibrary::author(const std::string& a) {
    diverts_.writeln(".AU");
    diverts_.writeln(a);
}

void LroffLibrary::display_begin(const std::string& type) {
    if (type.empty()) request("DS");
    else              request_with("DS", type);
}

void LroffLibrary::display_end() {
    request("DE");
}

// =================================================================
//  Compound structures
// =================================================================

void LroffLibrary::table_emit(
    const std::vector<std::string>&               hdr,
    const std::vector<std::vector<std::string>>&  rows,
    const std::string& fmt)
{
    diverts_.writeln(".TS");

    // user-supplied global options (e.g. "box center;")
    if (!fmt.empty())
        diverts_.writeln(fmt);

    // auto-generate column format from header count
    std::string hfmt, dfmt;
    for (std::size_t i = 0; i < hdr.size(); ++i) {
        if (i) { hfmt += ' '; dfmt += ' '; }
        hfmt += "cb";       // center bold
        dfmt += "l";        // left
    }
    diverts_.writeln(hfmt);
    diverts_.writeln(dfmt + ".");

    // header row
    {
        std::string line;
        for (std::size_t i = 0; i < hdr.size(); ++i) {
            if (i) line += '\t';
            line += hdr[i];
        }
        diverts_.writeln(line);
    }

    // horizontal rule
    diverts_.writeln("_");

    // data rows
    for (auto& row : rows) {
        std::string line;
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i) line += '\t';
            line += row[i];
        }
        diverts_.writeln(line);
    }

    diverts_.writeln(".TE");
}

void LroffLibrary::bullet_list(const std::vector<std::string>& items) {
    for (auto& item : items) {
        diverts_.writeln(".IP \\(bu 2");
        diverts_.writeln(item);
    }
}

void LroffLibrary::numbered_list(const std::vector<std::string>& items) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::string label = std::to_string(i + 1) + ".";
        diverts_.writeln(".IP " + label + " 4");
        diverts_.writeln(items[i]);
    }
}

void LroffLibrary::def_list(
    const std::vector<std::pair<std::string,std::string>>& items)
{
    for (auto& [term, def] : items) {
        diverts_.writeln(".TP");
        diverts_.writeln("\\fB" + term + "\\fP");
        diverts_.writeln(def);
    }
}

// =================================================================
//  Utility
// =================================================================

std::string LroffLibrary::unique(const std::string& prefix) {
    return state_.unique_name(prefix);
}

std::string LroffLibrary::version() { return PPLUA_VERSION; }

} // namespace pplua
