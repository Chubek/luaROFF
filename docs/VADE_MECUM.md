

# The `pplua` User Guide

## A Lua Preprocessor for the Groff Document Formatting System

---

## 1. What Is `pplua`?

`pplua` sits in the classic **roff preprocessor pipeline** — right alongside `tbl`, `eqn`, and `pic`. It reads your `.roff` source, finds embedded Lua code, executes it, and emits clean groff output. The philosophy is simple: **anything tedious or repetitive in groff can be scripted in Lua**.

your_doc.roff → pplua → tbl → eqn → groff -ms -Tpdf → output.pdf


There are two ways to embed Lua:

| Syntax | Purpose |
|---|---|
| `.lua` / `.endlua` block | Multi-line Lua code that calls `lroff.*` to emit groff |
| `\lua'expr'` inline | Single expression whose return value is inserted into the text |

---

## 2. Installation & Invocation

### Building

```bash
mkdir build && cd build
cmake .. -DSOL2_INCLUDE_DIR=/path/to/sol2/include
make -j$(nproc)
sudo make install    # installs 'pplua' to /usr/local/bin
```

### Command-Line Options

pplua [options] [file ...]

  -e CODE        Execute Lua CODE before processing any input.
  -l FILE        Run a Lua preamble file (e.g., shared data).
  -I PATH        Add PATH to Lua's package.path.
  -D NAME=VALUE  Set a Lua global variable (string).
  -n             Suppress .lf line-number directives.
  -V             Print version and exit.
  -h             Print help and exit.


### Typical Pipeline

```bash
# Simple
pplua doc.roff | groff -ms -Tpdf > doc.pdf

# With tbl tables and eqn equations
pplua doc.roff | tbl | eqn | groff -ms -Tpdf > doc.pdf

# With a shared data file and a debug flag
pplua -l data.lua -D DEBUG=1 report.roff | groff -ms -Tpdf > report.pdf

# From stdin
cat doc.roff | pplua | groff -ms -Tpdf > doc.pdf
```

---

## 3. The `lroff` API at a Glance

Every Lua block has access to the `lroff` table. Here is the complete API grouped by category:

### Output

| Function | Description |
|---|---|
| `lroff.emit(text)` | Write raw text (no trailing newline) |
| `lroff.emitln(text)` | Write text + newline |
| `lroff.printf(fmt, ...)` | Formatted emit (like `string.format`) |
| `lroff.printfln(fmt, ...)` | Formatted emit + newline |
| `lroff.request(name)` | Emit `.name` |
| `lroff.request_with(name, args)` | Emit `.name args` |
| `lroff.comment(text)` | Emit `.\" text` |
| `lroff.blank()` | Emit a blank line (paragraph break) |

### Escaping

| Function | Description |
|---|---|
| `lroff.escape(text)` | Escape `\`, leading `.` and `'` for safe groff output |
| `lroff.inline_escape(ec, arg)` | Build an inline escape like `\f[B]` or `\n[reg]` |

### Inline Styling (return strings — do not emit)

| Function | Description |
|---|---|
| `lroff.bold(text)` | `\fBtext\fP` |
| `lroff.italic(text)` | `\fItext\fP` |
| `lroff.bold_italic(text)` | `\f[BI]text\f[P]` |
| `lroff.mono(text)` | `\f[CR]text\f[P]` |
| `lroff.styled(font, text)` | Generic: `\f[font]text\f[P]` |
| `lroff.special_char(name)` | `\[name]` (groff special character) |

### Fonts & Sizes (emit requests)

| Function | Description |
|---|---|
| `lroff.font(f)` | `.ft f` |
| `lroff.font_bold()` | `.ft B` |
| `lroff.font_italic()` | `.ft I` |
| `lroff.font_roman()` | `.ft R` |
| `lroff.font_previous()` | `.ft` (revert) |
| `lroff.size(pts)` | `.ps pts` |
| `lroff.size_relative(delta)` | `.ps +delta` or `.ps -delta` |

### Number & String Registers

| Function | Description |
|---|---|
| `lroff.nr_set(name, val)` | `.nr name val` + track in Lua |
| `lroff.nr_incr(name, delta)` | `.nr name +delta` |
| `lroff.nr_get(name)` | Return tracked value (or `nil`) |
| `lroff.nr_ref(name)` | Return string `\n[name]` for embedding in text |
| `lroff.ds_set(name, val)` | `.ds name val` |
| `lroff.ds_get(name)` | Return tracked value (or `nil`) |
| `lroff.ds_ref(name)` | Return string `\*[name]` |

### Diversions (preprocessor-level capture)

| Function | Description |
|---|---|
| `lroff.divert_begin(name)` | Start capturing output into buffer `name` |
| `lroff.divert_end()` | Stop capturing |
| `lroff.divert_emit(name)` | Replay captured content |
| `lroff.divert_get(name)` | Return captured content as a Lua string |
| `lroff.divert_clear(name)` | Discard captured content |

### Document Structure (ms macros)

| Function | Description |
|---|---|
| `lroff.title(text)` | `.TL` / text |
| `lroff.author(text)` | `.AU` / text |
| `lroff.section(text)` | `.SH` / text |
| `lroff.subsection(text)` | `.SS` / text |
| `lroff.paragraph(macro?)` | `.PP` (default) or `.LP`, `.IP`, etc. |
| `lroff.display_begin(type?)` | `.DS` or `.DS type` |
| `lroff.display_end()` | `.DE` |

### Compound Structures

| Function | Description |
|---|---|
| `lroff.table(headers, rows, fmt?)` | Full `tbl` table |
| `lroff.bullet_list(items)` | Bullet list via `.IP \(bu 2` |
| `lroff.numbered_list(items)` | Numbered list via `.IP n. 4` |
| `lroff.def_list(items)` | Definition list via `.TP` |

### Macros

| Function | Description |
|---|---|
| `lroff.macro_define(name, body)` | `.de name` / body / `..` |
| `lroff.macro_define_lua(name, code)` | Define a macro whose body is a `.lua` block |

### Utility & Helpers

| Function | Description |
|---|---|
| `lroff.unique(prefix?)` | Return a unique name like `_lua1`, `_lua2`, … |
| `lroff.version()` | Return `"pplua 0.1.0"` |
| `lroff.map(tbl, fn)` | Apply `fn` to each element; emit non-nil returns |
| `lroff.foreach(tbl, fn)` | Call `fn(k,v)` for each pair |
| `lroff.with_font(f, fn)` | Scoped font change |
| `lroff.with_size(s, fn)` | Scoped size change |
| `lroff.indented(fn)` | Wrap `fn()` in `.RS` / `.RE` |
| `lroff.groff_if(cond, body)` | Emit `.if cond \{ body \}` |
| `lroff.groff_while(cond, body)` | Emit `.while cond \{ body \}` |
| `lroff.concat(tbl, sep?)` | Like `table.concat` but calls `tostring` |

---

## 4. Creative Examples

### Example 1 — Automated Software Release Notes

This demonstrates using Lua tables as a structured data source, then generating an entire formatted document from them.

```roff
.\" release-notes.roff
.\" Build: pplua release-notes.roff | tbl | groff -ms -Tpdf > release.pdf
.
.lua
-- ============================================================
-- Structured release data — could also be loaded from a file
-- with  dofile("releases.lua")
-- ============================================================
releases = {
    {
        version = "3.2.0",
        date    = "2026-02-20",
        status  = "stable",
        changes = {
            { type = "feature", desc = "Added ARM64 JIT backend" },
            { type = "feature", desc = "New streaming JSON parser" },
            { type = "bugfix",  desc = "Fixed race condition in thread pool (#4217)" },
            { type = "bugfix",  desc = "Corrected off-by-one in ring buffer" },
            { type = "perf",    desc = "Reduced allocator overhead by 18%" },
        },
    },
    {
        version = "3.1.2",
        date    = "2026-01-05",
        status  = "maintenance",
        changes = {
            { type = "bugfix",  desc = "Patched CVE-2026-00142 in TLS handshake" },
            { type = "bugfix",  desc = "Fixed SIGPIPE handling on FreeBSD" },
            { type = "docs",    desc = "Rewrote the configuration chapter" },
        },
    },
    {
        version = "3.1.1",
        date    = "2025-11-30",
        status  = "maintenance",
        changes = {
            { type = "bugfix",  desc = "Locale-dependent date formatting corrected" },
            { type = "perf",    desc = "Batch insert now 2x faster on large datasets" },
        },
    },
}

-- helper: icon for change type
function change_icon(t)
    if t == "feature" then return lroff.special_char("*>")   -- ✱
    elseif t == "bugfix" then return lroff.special_char("bu") -- •
    elseif t == "perf" then return lroff.special_char("ua")   -- ↑
    elseif t == "docs" then return lroff.special_char("lh")   -- ☞
    else return lroff.special_char("em")                       -- —
    end
end

-- helper: human label
function change_label(t)
    local labels = {
        feature = "New Feature",
        bugfix  = "Bug Fix",
        perf    = "Performance",
        docs    = "Documentation",
    }
    return labels[t] or t
end
.endlua
.
.\" ---- Title page ----
.lua
lroff.title("Project Helios " .. lroff.special_char("em") .. " Release Notes")
lroff.author("Engineering Team")
.endlua
.LP
.DA
.AB
This document covers the last \lua'#releases' releases of
Project Helios, automatically generated from structured Lua data.
.AE
.
.\" ---- Generate a section for each release ----
.lua
for _, rel in ipairs(releases) do
    -- Section heading with version, date, and status badge
    lroff.section(string.format("Version %s  (%s)  [%s]",
        rel.version, rel.date, rel.status:upper()))

    -- Summary table: count changes by type
    local counts = {}
    for _, c in ipairs(rel.changes) do
        counts[c.type] = (counts[c.type] or 0) + 1
    end

    -- Build the summary as a small tbl table
    local hdr = {"Category", "Count"}
    local rows = {}
    for _, t in ipairs({"feature", "bugfix", "perf", "docs"}) do
        if counts[t] then
            rows[#rows + 1] = {change_label(t), tostring(counts[t])}
        end
    end
    if #rows > 0 then
        lroff.paragraph()
        lroff.emitln(lroff.italic("Change summary:"))
        lroff.blank()
        lroff.table(hdr, rows, "center;")
    end

    -- Detailed list
    lroff.paragraph()
    lroff.emitln(lroff.italic("Details:"))

    for _, c in ipairs(rel.changes) do
        lroff.emitln(string.format(
            ".IP \"%s %s\" 4",
            change_icon(c.type),
            lroff.bold(change_label(c.type))
        ))
        lroff.emitln(c.desc)
    end
end
.endlua
```

**What's happening:** A single Lua table holds *all* the release data. The loop generates one groff section per release, with an auto-computed summary table and a categorized change list. Adding a new release means adding a table entry — zero groff editing.

---

### Example 2 — A Fictional Bestiary with Procedural Stats

This creates a fantasy creature compendium where each creature's stat block, description, and danger rating are computed from base attributes.

```roff
.\" bestiary.roff
.\" Build: pplua bestiary.roff | tbl | groff -ms -Tpdf > bestiary.pdf
.
.lua
-- ============================================================
-- Creature database
-- ============================================================
creatures = {
    {
        name   = "Ember Wyrm",
        class  = "Dragon",
        habitat = "Volcanic caverns",
        str = 18, dex = 12, con = 20, int = 14, wis = 10, cha = 16,
        abilities = {"Fire Breath (60 ft. cone)", "Tremorsense",
                     "Frightful Presence"},
        lore = [[A serpentine dragon that nests in pools of magma.
Its scales glow like banked coals, and its breath can melt
castle walls.  Ancient texts warn: where the ground steams
and stone runs like water, the Ember Wyrm is near.]],
    },
    {
        name   = "Gloom Stalker",
        class  = "Aberration",
        habitat = "Deep forest, twilight zones",
        str = 14, dex = 20, con = 12, int = 6, wis = 18, cha = 4,
        abilities = {"Shadow Step", "Paralyzing Gaze",
                     "Spider Climb"},
        lore = [[A creature of pure shadow that hunts at dusk.
It moves between patches of darkness as if stepping through
doors.  Victims report a chill and a sense of being watched
before the creature strikes with terrifying speed.]],
    },
    {
        name   = "Thornback Basilisk",
        class  = "Monstrosity",
        habitat = "Arid scrublands, ruins",
        str = 16, dex = 8, con = 18, int = 3, wis = 12, cha = 6,
        abilities = {"Petrifying Gaze", "Venomous Bite",
                     "Thorn Shield (1d6 reflect)"},
        lore = [[An eight-legged reptile covered in calcite spines.
Its gaze petrifies prey, which it later consumes at leisure.
Alchemists prize its blood for stoneshaping elixirs, but
harvesting it is, unsurprisingly, hazardous.]],
    },
}

-- Compute derived stats
function modifier(score)
    return math.floor((score - 10) / 2)
end

function mod_str(score)
    local m = modifier(score)
    if m >= 0 then return string.format("%d (+%d)", score, m)
    else           return string.format("%d (%d)", score, m)
    end
end

function danger_rating(c)
    -- simple formula: average of all stats, scaled
    local avg = (c.str + c.dex + c.con + c.int + c.wis + c.cha) / 6
    if avg >= 17 then return "Lethal", "\\m[red]Lethal\\m[]"
    elseif avg >= 13 then return "Dangerous", "\\m[red3]Dangerous\\m[]"
    elseif avg >= 9  then return "Moderate", "Moderate"
    else return "Low", "Low"
    end
end
.endlua
.
.\" ---- Document header ----
.lua
lroff.title("Bestiary of the Shattered Realms")
lroff.author("The Cartographer's Guild")
.endlua
.LP
.AB
A field guide to \lua'#creatures' creatures encountered in the
Shattered Realms.  Danger ratings are computed from base
attributes.  Handle with care.
.AE
.
.\" ---- Table of Contents (dynamically built) ----
.lua
lroff.section("TABLE OF CONTENTS")
lroff.blank()
for i, c in ipairs(creatures) do
    local _, dr_plain = danger_rating(c)
    lroff.printfln(".IP %d. 4", i)
    lroff.printfln("%s %s %s",
        lroff.bold(c.name),
        lroff.special_char("em"),
        c.class)
end
.endlua
.
.\" ---- Creature entries ----
.lua
for _, c in ipairs(creatures) do
    local dr_text, dr_styled = danger_rating(c)

    -- Section heading
    lroff.section(c.name)

    -- Classification line
    lroff.paragraph()
    lroff.printfln("%s %s  |  %s %s  |  %s %s",
        lroff.bold("Class:"), c.class,
        lroff.bold("Habitat:"), c.habitat,
        lroff.bold("Danger:"), dr_styled)

    -- Stat block as a tbl table
    lroff.blank()
    lroff.table(
        {"STR", "DEX", "CON", "INT", "WIS", "CHA"},
        {{mod_str(c.str), mod_str(c.dex), mod_str(c.con),
          mod_str(c.int), mod_str(c.wis), mod_str(c.cha)}},
        "center box;"
    )

    -- Abilities
    lroff.blank()
    lroff.paragraph()
    lroff.emitln(lroff.bold("Abilities:"))
    lroff.bullet_list(c.abilities)

    -- Lore
    lroff.paragraph()
    lroff.emitln(lroff.italic("Field Notes:"))
    lroff.paragraph("QP")
    -- Escape the lore text so that any '.' at line start is safe
    lroff.emitln(lroff.escape(c.lore))
end
.endlua
```

**What's happening:** Modifier values ($\lfloor (\text{score} - 10) / 2 \rfloor$) and danger ratings are computed in Lua. The stat block is a `tbl` table generated per creature. Adding a creature means adding a table entry — the entire stat block, ability list, and formatting propagate automatically.

---

### Example 3 — Literate Configuration with Validation

This example generates a system administrator's configuration reference where the configuration schema is defined in Lua, validated, and rendered as both documentation and an example config file (via diversions).

```roff
.\" config-guide.roff
.\" Build: pplua config-guide.roff | tbl | groff -ms -Tpdf > config.pdf
.
.lua
-- ============================================================
-- Configuration schema
-- ============================================================
schema = {
    {
        key     = "listen_address",
        type    = "string",
        default = "0.0.0.0",
        range   = nil,
        desc    = "IP address to bind the server socket to.",
    },
    {
        key     = "listen_port",
        type    = "integer",
        default = 8080,
        range   = {1, 65535},
        desc    = "TCP port number.  Must be in the range 1\\(en65535.",
    },
    {
        key     = "max_connections",
        type    = "integer",
        default = 1024,
        range   = {1, 100000},
        desc    = "Maximum simultaneous client connections.",
    },
    {
        key     = "tls_enabled",
        type    = "boolean",
        default = false,
        range   = nil,
        desc    = "Enable TLS encryption.  Requires tls_cert and tls_key.",
    },
    {
        key     = "tls_cert",
        type    = "path",
        default = "/etc/myapp/cert.pem",
        range   = nil,
        desc    = "Path to the TLS certificate file (PEM format).",
    },
    {
        key     = "tls_key",
        type    = "path",
        default = "/etc/myapp/key.pem",
        range   = nil,
        desc    = "Path to the TLS private key file.",
    },
    {
        key     = "log_level",
        type    = "enum",
        default = "info",
        range   = {"debug", "info", "warn", "error", "fatal"},
        desc    = "Logging verbosity level.",
    },
    {
        key     = "worker_threads",
        type    = "integer",
        default = 4,
        range   = {1, 256},
        desc    = "Number of worker threads in the request pool.",
    },
}

-- Validation: check that defaults match declared types/ranges
warnings = {}
for _, opt in ipairs(schema) do
    if opt.type == "integer" and opt.range then
        if opt.default < opt.range[1] or opt.default > opt.range[2] then
            warnings[#warnings + 1] = string.format(
                "%s: default %s out of range [%d, %d]",
                opt.key, tostring(opt.default),
                opt.range[1], opt.range[2])
        end
    end
    if opt.type == "enum" and opt.range then
        local found = false
        for _, v in ipairs(opt.range) do
            if v == opt.default then found = true; break end
        end
        if not found then
            warnings[#warnings + 1] = string.format(
                "%s: default '%s' not in enum set",
                opt.key, tostring(opt.default))
        end
    end
end

-- type badge
function type_badge(t)
    return lroff.mono(t)
end

-- format range
function fmt_range(opt)
    if not opt.range then return lroff.special_char("em") end
    if opt.type == "integer" then
        return string.format("%d \\(en %d", opt.range[1], opt.range[2])
    elseif opt.type == "enum" then
        local parts = {}
        for _, v in ipairs(opt.range) do
            parts[#parts + 1] = lroff.mono(v)
        end
        return table.concat(parts, " | ")
    end
    return tostring(opt.range)
end
.endlua
.
.\" ---- Document ----
.lua
lroff.title("myappd.conf " .. lroff.special_char("em") ..
    " Configuration Reference")
lroff.author("Operations Team")
.endlua
.LP
.DA
.AB
Auto-generated reference for all \lua'#schema' configuration
directives of
.B myappd .
Validated at document build time (\lua'os.date("%Y-%m-%d %H:%M")').
.AE
.
.\" ---- Validation warnings (if any) ----
.lua
if #warnings > 0 then
    lroff.section("BUILD WARNINGS")
    lroff.paragraph()
    lroff.emitln("The following schema validation issues were detected:")
    lroff.blank()
    lroff.bullet_list(warnings)
else
    lroff.comment("Schema validation passed — no warnings.")
end
.endlua
.
.\" ---- Quick-reference summary table ----
.lua
lroff.section("SUMMARY TABLE")
lroff.paragraph()
local hdr = {"Directive", "Type", "Default"}
local rows = {}
for _, opt in ipairs(schema) do
    rows[#rows + 1] = {
        lroff.mono(opt.key),
        type_badge(opt.type),
        lroff.mono(tostring(opt.default)),
    }
end
lroff.table(hdr, rows, "center box;")
.endlua
.
.\" ---- Detailed directive entries ----
.lua
lroff.section("DIRECTIVE REFERENCE")

for _, opt in ipairs(schema) do
    -- Use .TP (tagged paragraph) style
    lroff.emitln(".TP 4")
    lroff.printfln("%s  %s", lroff.bold(opt.key), type_badge(opt.type))
    lroff.emitln(opt.desc)

    -- Sub-details via indentation
    lroff.indented(function()
        lroff.emitln(lroff.bold("Default: ") ..
            lroff.mono(tostring(opt.default)))
        lroff.emitln(lroff.bold("Range: ") .. fmt_range(opt))
    end)
    lroff.blank()
end
.endlua
.
.\" ---- Example config file (built via diversion) ----
.lua
lroff.section("EXAMPLE CONFIGURATION FILE")
lroff.paragraph()
lroff.emitln("A complete example with all defaults:")

-- Build the example config into a diversion
lroff.divert_begin("example_config")
lroff.emitln("# myappd.conf — generated " .. os.date("%Y-%m-%d"))
lroff.emitln("#")
for _, opt in ipairs(schema) do
    lroff.printfln("# %s", opt.desc:gsub("\n", " "))
    if opt.type == "boolean" then
        lroff.printfln("%s = %s",
            opt.key, opt.default and "true" or "false")
    else
        lroff.printfln("%s = %s", opt.key, tostring(opt.default))
    end
    lroff.emitln("")
end
lroff.divert_end()

-- Now emit it inside a display
lroff.display_begin("I")
lroff.request("ft CR")
lroff.divert_emit("example_config")
lroff.request("ft")
lroff.display_end()
.endlua
```

**What's happening:** The schema is validated *at document build time* — if a default is out of range, a warning section appears in the PDF. The summary table, detailed reference, and example config file are all generated from the same schema table. Change the schema, rebuild, and every section updates consistently. The example config is captured in a **diversion** and then replayed inside a display block.

---

### Example 4 — An Examination Paper with Randomized Answer Order

This shows Lua's `math.random` being used to shuffle multiple-choice answers while tracking the correct answer key — perfect for generating unique exam variants.

```roff
.\" exam.roff
.\" Build: pplua -D SEED=42 exam.roff | groff -ms -Tpdf > exam.pdf
.
.lua
-- ============================================================
-- Seed the RNG (use -D SEED=N for reproducible variants)
-- ============================================================
local seed = tonumber(SEED) or os.time()
math.randomseed(seed)

-- Fisher-Yates shuffle, returns shuffled copy + new index of
-- the element that was originally at position 'correct_idx'
function shuffle_answers(answers, correct_idx)
    local a = {}
    for i, v in ipairs(answers) do a[i] = {text = v, orig = i} end
    for i = #a, 2, -1 do
        local j = math.random(i)
        a[i], a[j] = a[j], a[i]
    end
    local new_correct
    local texts = {}
    for i, entry in ipairs(a) do
        texts[i] = entry.text
        if entry.orig == correct_idx then new_correct = i end
    end
    return texts, new_correct
end

-- Question bank
questions = {
    {
        text = "What is the time complexity of binary search on a sorted array of $n$ elements?",
        answers = {"$O(n)$", "$O(log n)$", "$O(n log n)$", "$O(1)$"},
        correct = 2,
    },
    {
        text = "Which data structure uses LIFO (Last In, First Out) ordering?",
        answers = {"Queue", "Stack", "Heap", "Deque"},
        correct = 2,
    },
    {
        text = "In Unix, which signal is sent by default when you press Ctrl+C?",
        answers = {"SIGTERM", "SIGKILL", "SIGINT", "SIGHUP"},
        correct = 3,
    },
    {
        text = "What does the HTTP status code 418 mean?",
        answers = {"Not Found", "I'm a teapot",
                   "Internal Server Error", "Unauthorized"},
        correct = 2,
    },
    {
        text = "Which sorting algorithm has a worst-case time complexity of $O(n^2)$ but is often fast in practice?",
        answers = {"Merge Sort", "Heap Sort", "Quick Sort", "Radix Sort"},
        correct = 3,
    },
}

-- Shuffle and record answer key
answer_key = {}
for i, q in ipairs(questions) do
    local shuffled, correct_pos = shuffle_answers(q.answers, q.correct)
    q.shuffled = shuffled
    q.shuffled_correct = correct_pos
    answer_key[i] = correct_pos
end

-- Letter labels
function letter(n) return string.char(64 + n) end   -- 1→A, 2→B, …
.endlua
.
.\" ---- Exam header ----
.lua
lroff.title("CS 201 " .. lroff.special_char("em") .. " Midterm Examination")
lroff.author("Department of Computer Science")
.endlua
.LP
.DA
.sp 1
.ce 2
Variant seed: \lua'seed'
Time allowed: 60 minutes
.sp 1
.LP
.B "Instructions:"
Answer all \lua'#questions' questions.
Circle the letter of the correct answer.
Each question is worth \lua'math.floor(100 / #questions)' points.
.
.\" ---- Questions ----
.lua
lroff.section("QUESTIONS")

for i, q in ipairs(questions) do
    lroff.blank()
    lroff.printfln(".IP %d. 4", i)
    lroff.emitln(q.text)

    lroff.blank()
    for j, ans in ipairs(q.shuffled) do
        lroff.printfln(".RS 4")
        lroff.printfln(".IP %s) 4", letter(j))
        lroff.emitln(ans)
        lroff.request("RE")
    end
end
.endlua
.
.\" ---- Page break, then answer key ----
.lua
lroff.request("bp")
lroff.section("ANSWER KEY (for instructor use only)")
lroff.paragraph()
lroff.printfln("Variant seed: %s", tostring(seed))
lroff.blank()

local hdr = {"#", "Answer"}
local rows = {}
for i, pos in ipairs(answer_key) do
    rows[#rows + 1] = {tostring(i), lroff.bold(letter(pos))}
end
lroff.table(hdr, rows, "center box;")
.endlua
```

**What's happening:** Every build with a different `-D SEED=N` produces a different answer order but the answer key on the last page always stays correct. The instructor can generate unique variants per exam room simply by changing the seed:

```bash
for s in 1 2 3 4 5; do
    pplua -D SEED=$s exam.roff | groff -ms -Tpdf > exam-variant-$s.pdf
done
```

---

### Example 5 — A Financial Report with Computed Totals and Sparkline-Style Bars

This generates a quarterly financial summary with computed totals, percentages, year-over-year deltas, and ASCII "bar charts" built from groff drawing primitives.

```roff
.\" finance.roff
.\" Build: pplua finance.roff | tbl | groff -ms -Tpdf > finance.pdf
.
.lua
-- ============================================================
-- Financial data
-- ============================================================
quarters = {
    {label = "Q1 2025", revenue = 1240000, expenses = 890000},
    {label = "Q2 2025", revenue = 1510000, expenses = 920000},
    {label = "Q3 2025", revenue = 1380000, expenses = 1010000},
    {label = "Q4 2025", revenue = 1720000, expenses = 980000},
}

-- Compute derived fields
total_revenue  = 0
total_expenses = 0
max_revenue    = 0

for _, q in ipairs(quarters) do
    q.profit = q.revenue - q.expenses
    q.margin = (q.profit / q.revenue) * 100
    total_revenue  = total_revenue  + q.revenue
    total_expenses = total_expenses + q.expenses
    if q.revenue > max_revenue then max_revenue = q.revenue end
end

total_profit = total_revenue - total_expenses
total_margin = (total_profit / total_revenue) * 100

-- Format a number as $X,XXX,XXX
function fmt_money(n)
    local s = string.format("%.0f", n)
    -- insert commas from right
    local result = ""
    local count = 0
    for i = #s, 1, -1 do
        if count > 0 and count % 3 == 0 then
            result = "," .. result
        end
        result = s:sub(i,i) .. result
        count = count + 1
    end
    return "$" .. result
end

-- Build a text bar: ████░░░░ style using groff's \[br] and spaces
-- Actually, we'll use a horizontal rule \l'Np' of proportional width
function bar(value, max_val, total_width_inches)
    local width = (value / max_val) * total_width_inches
    return string.format("\\l'%.2fi\\[ul]'", width)
end

-- QoQ delta
function delta_str(current, previous)
    if not previous then return lroff.special_char("em") end
    local pct = ((current - previous) / previous) * 100
    if pct >= 0 then
        return string.format("+%.1f%%", pct)
    else
        return string.format("%.1f%%", pct)
    end
end
.endlua
.
.\" ---- Header ----
.lua
lroff.title("Quarterly Financial Summary " ..
    lroff.special_char("em") .. " FY 2025")
lroff.author("Finance Department")
.endlua
.LP
.DA
.AB
Annual revenue: \lua'fmt_money(total_revenue)'.
Annual profit: \lua'fmt_money(total_profit)'
(\lua'string.format("%.1f%%", total_margin)' margin).
Report generated \lua'os.date("%B %d, %Y at %H:%M")'.
.AE
.
.\" ---- Summary Table ----
.lua
lroff.section("QUARTERLY BREAKDOWN")
lroff.paragraph()

local hdr = {"Quarter", "Revenue", "Expenses", "Profit",
             "Margin", "QoQ " .. lroff.special_char("De")}
local rows = {}
for i, q in ipairs(quarters) do
    local prev_rev = (i > 1) and quarters[i-1].revenue or nil
    rows[#rows + 1] = {
        lroff.bold(q.label),
        fmt_money(q.revenue),
        fmt_money(q.expenses),
        fmt_money(q.profit),
        string.format("%.1f%%", q.margin),
        delta_str(q.revenue, prev_rev),
    }
end
-- Totals row
rows[#rows + 1] = {
    lroff.bold("TOTAL"),
    lroff.bold(fmt_money(total_revenue)),
    lroff.bold(fmt_money(total_expenses)),
    lroff.bold(fmt_money(total_profit)),
    lroff.bold(string.format("%.1f%%", total_margin)),
    "",
}
lroff.table(hdr, rows, "center allbox;")
.endlua
.
.\" ---- Visual chart ----
.lua
lroff.section("REVENUE TREND")
lroff.paragraph()
lroff.emitln("Visual comparison (proportional bars):")
lroff.blank()

for _, q in ipairs(quarters) do
    lroff.printfln(".IP \"%s\" 12", q.label)
    lroff.printfln("%s  %s",
        bar(q.revenue, max_revenue, 3.0),
        fmt_money(q.revenue))
end
.endlua
.
.\" ---- Margin analysis ----
.lua
lroff.section("MARGIN ANALYSIS")
lroff.paragraph()

-- Find best and worst quarters
local best  = quarters[1]
local worst = quarters[1]
for _, q in ipairs(quarters) do
    if q.margin > best.margin  then best  = q end
    if q.margin < worst.margin then worst = q end
end

lroff.emitln("The " .. lroff.bold("best") ..
    " margin was in " .. lroff.bold(best.label) ..
    string.format(" at %.1f%%.", best.margin))
lroff.emitln("The " .. lroff.bold("worst") ..
    " margin was in " .. lroff.bold(worst.label) ..
    string.format(" at %.1f%%.", worst.margin))
lroff.blank()
lroff.emitln("Detailed margin breakdown:")
lroff.blank()

-- Definition list of margin per quarter
local items = {}
for _, q in ipairs(quarters) do
    items[#items + 1] = {
        q.label,
        string.format("%.1f%% margin on %s revenue (%s profit)",
            q.margin, fmt_money(q.revenue), fmt_money(q.profit))
    }
end
lroff.def_list(items)
.endlua
```

**What's happening:** All financial computations — totals, margins, quarter-over-quarter deltas, best/worst detection — happen in Lua. The proportional bar chart uses groff's `\l` horizontal line escape, computed to a fractional-inch width: $ \text{width} = \frac{\text{value}}{\text{max}} \times 3.0\,\text{in} $. Update the `quarters` table and every figure, table, chart, and narrative sentence updates.

---

### Example 6 — A Multi-Language Phrasebook Using `-D` for Language Selection

This shows how `-D` flags on the command line can select content variants — the same source file produces phrasebooks for different target languages.

```roff
.\" phrasebook.roff
.\" Build:
.\"   pplua -D LANG=fr phrasebook.roff | groff -ms -Tpdf > french.pdf
.\"   pplua -D LANG=de phrasebook.roff | groff -ms -Tpdf > german.pdf
.\"   pplua -D LANG=ja phrasebook.roff | groff -ms -Tpdf > japanese.pdf
.
.lua
-- ============================================================
-- Phrase database
-- ============================================================
phrases = {
    {
        en = "Hello",
        fr = "Bonjour",
        de = "Hallo",
        ja = "\\(kokonnichiwa\\(co",  -- placeholder romanization
        category = "Greetings",
    },
    {
        en = "Thank you",
        fr = "Merci",
        de = "Danke",
        ja = "Arigatou",
        category = "Greetings",
    },
    {
        en = "Where is the train station?",
        fr = "O\\(`u est la gare?",
        de = "Wo ist der Bahnhof?",
        ja = "Eki wa doko desu ka?",
        category = "Travel",
    },
    {
        en = "How much does this cost?",
        fr = "Combien \\(,ca co\\(^ute?",
        de = "Was kostet das?",
        ja = "Ikura desu ka?",
        category = "Shopping",
    },
    {
        en = "I would like a coffee, please.",
        fr = "Je voudrais un caf\\('e, s'il vous pla\\(^it.",
        de = "Ich h\\(\"atte gerne einen Kaffee, bitte.",
        ja = "Koohii o kudasai.",
        category = "Food & Drink",
    },
    {
        en = "Excuse me",
        fr = "Excusez-moi",
        de = "Entschuldigung",
        ja = "Sumimasen",
        category = "Politeness",
    },
}

-- Determine target language from -D LANG=xx
target = LANG or "fr"

lang_names = {
    fr = "French",
    de = "German",
    ja = "Japanese",
}
lang_name = lang_names[target] or target:upper()

-- Group phrases by category
categories = {}
cat_order  = {}
for _, p in ipairs(phrases) do
    if not categories[p.category] then
        categories[p.category] = {}
        cat_order[#cat_order + 1] = p.category
    end
    table.insert(categories[p.category], p)
end
.endlua
.
.\" ---- Header ----
.lua
lroff.title("Traveler's Phrasebook: English " ..
    lroff.special_char("->") .. " " .. lang_name)
lroff.author("pplua Language Tools")
.endlua
.LP
.DA
.AB
A pocket phrasebook containing \lua'#phrases' essential phrases
in \lua'lang_name', organized by category.
Target language code:
.B "\lua'target'" .
.AE
.
.\" ---- Phrases by category ----
.lua
for _, cat in ipairs(cat_order) do
    lroff.section(cat:upper())
    lroff.blank()

    local hdr  = {"English", lang_name}
    local rows = {}
    for _, p in ipairs(categories[cat]) do
        rows[#rows + 1] = {p.en, lroff.bold(p[target] or "(?)")}
    end
    lroff.table(hdr, rows, "center box;")
end
.endlua
.
.\" ---- Quick-reference card (all phrases, compact) ----
.lua
lroff.section("QUICK REFERENCE")
lroff.paragraph()
lroff.emitln("All phrases at a glance:")
lroff.blank()

local all_items = {}
for _, p in ipairs(phrases) do
    all_items[#all_items + 1] = {
        p.en,
        lroff.bold(p[target] or "(?)")
    }
end
lroff.def_list(all_items)
.endlua
```

**What's happening:** A single source generates different phrasebooks depending on the `-D LANG=xx` flag. The phrase database, category grouping, table generation, and definition list are all driven by data. Adding a new language means adding one key per phrase — no structural changes.

---

## 5. Tips and Patterns

### 5.1 Separating Data from Presentation

Keep your data in a separate `.lua` file and load it with `-l`:

```bash
pplua -l project_data.lua report.roff | groff -ms -Tpdf > report.pdf
```

```lua
-- project_data.lua
project = {
    name = "Helios",
    version = "3.2.0",
    modules = { ... },
}
```

### 5.2 Reusable Helper Libraries

Place shared functions in a Lua module and add its path with `-I`:

```bash
pplua -I ./lib report.roff | groff -ms -Tpdf > report.pdf
```

```lua
-- lib/formatting.lua
local M = {}
function M.fmt_money(n) ... end
function M.fmt_pct(n) ... end
return M
```

```roff
.lua
fmt = require("formatting")
lroff.emitln(fmt.fmt_money(1234567))
.endlua
```

### 5.3 Conditional Content

Use `-D` flags and Lua conditionals to produce different document variants from a single source:

```roff
.lua
if DRAFT then
    lroff.emitln(".ds LH DRAFT")
    lroff.emitln(".ds RH " .. os.date("%Y-%m-%d"))
end
.endlua
```

```bash
pplua -D DRAFT=1 paper.roff | groff -ms -Tpdf > draft.pdf
pplua paper.roff | groff -ms -Tpdf > final.pdf
```

### 5.4 Reading External Files at Build Time

Lua's `io` library is available, so you can ingest CSV, JSON (with a pure-Lua parser), or any text:

```roff
.lua
-- Read a CSV and generate a table
local rows = {}
for line in io.lines("data.csv") do
    local fields = {}
    for field in line:gmatch("[^,]+") do
        fields[#fields + 1] = field
    end
    rows[#rows + 1] = fields
end
local headers = table.remove(rows, 1)
lroff.table(headers, rows, "center box;")
.endlua
```

### 5.5 Debugging

Use `-e` to inject debug helpers:

```bash
# Print all lroff output to stderr as well as stdout
pplua -e 'DEBUG=true' doc.roff | groff -ms -Tpdf > doc.pdf
```

Inside your document:

```roff
.lua
if DEBUG then
    io.stderr:write("Processing section: " .. title .. "\n")
end
.endlua
```

### 5.6 The Inline Expression Sweet Spot

Inline `\lua'…'` is perfect for short computed values in running prose. Avoid putting complex logic inside it — use a block to define variables, then reference them inline:

```roff
.lua
total = 0
for _, q in ipairs(quarters) do total = total + q.revenue end
.endlua
.LP
The annual revenue was \lua'fmt_money(total)', representing
a \lua'string.format("%.1f", growth_pct)'% increase over the
prior fiscal year.
```

---

## 6. Pipeline Integration

`pplua` is designed to be a well-behaved Unix filter. It reads from files or stdin, writes to stdout, and reports errors to stderr. This means it slots cleanly into any groff pipeline:

```bash
# Full pipeline with all classical preprocessors
pplua doc.roff | soelim | tbl | eqn | pic | groff -ms -Tpdf > doc.pdf

# In a Makefile
%.pdf: %.roff
	pplua $< | tbl | eqn | groff -ms -Tpdf > $@

# Processing multiple input files (concatenated)
pplua front.roff body.roff back.roff | groff -ms -Tpdf > book.pdf
```

The `.lf` directives emitted by `pplua` (unless suppressed with `-n`) ensure that groff error messages point back to the correct line in your original source, not the post-processed output — just like `soelim`, `tbl`, and `eqn` do.

---

## 7. Quick Reference Card

┌──────────────────────────────────────────────────────┐
│  .lua            Start a Lua block                   │
│  .endlua         End a Lua block                     │
│  \lua'expr'      Inline expression (value inserted)  │
│                                                      │
│  lroff.emit(s)         Raw output                    │
│  lroff.emitln(s)       Raw output + newline          │
│  lroff.request(r)      Emit .r                       │
│  lroff.bold(s)         Return \fBs\fP                │
│  lroff.italic(s)       Return \fIs\fP                │
│  lroff.mono(s)         Return \f[CR]s\f[P]           │
│  lroff.section(s)      Emit .SH / s                  │
│  lroff.table(h,r)      Emit tbl table                │
│  lroff.bullet_list(t)  Emit bullet list              │
│  lroff.def_list(t)     Emit definition list          │
│  lroff.escape(s)       Escape text for groff         │
│  lroff.nr_set(n,v)     Set number register           │
│  lroff.ds_set(n,v)     Set string register           │
│  lroff.divert_begin(n) Start diversion               │
│  lroff.divert_end()    End diversion                 │
│  lroff.divert_emit(n)  Replay diversion              │
│  lroff.unique(pfx)     Unique name generator         │
└──────────────────────────────────────────────────────┘
