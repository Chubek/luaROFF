" Vim syntax file
" Language:    groff/troff with pplua embedded Lua
" Maintainer:  pplua project
" Last Change: 2026-02-26
" Remark:      Provides groff highlighting plus Lua inside .lua/.endlua
"              blocks and \lua'...' inline expressions.

" ─── Guard ────────────────────────────────────────────────────────────────
" Quit if a syntax file was already loaded for this buffer.
if exists("b:current_syntax")
  finish
endif

" We need Lua syntax for embedding. Source it under a cluster prefix.
" The standard approach: unlet b:current_syntax so that lua.vim will load,
" then capture all its groups under a cluster.

" Save the current syntax name (none yet since we cleared above) and
" include Lua syntax.
let s:saved_syntax = get(b:, 'current_syntax', '')
unlet! b:current_syntax
syntax include @ppluaLuaCode syntax/lua.vim
let b:current_syntax = s:saved_syntax


" Use 'spell' default: no spell-checking in code regions.
syn spell default

" ─── Synchronization ─────────────────────────────────────────────────────
" Because Lua blocks can be long, sync from the start of the file to be
" safe, or use a minlines value.  For performance, use minlines.
syn sync minlines=200

" ═══════════════════════════════════════════════════════════════════════════
"  SECTION 1: EMBEDDED LUA (must be defined BEFORE groff so containment
"             works — the Lua block consumes lines that would otherwise
"             match groff request patterns)
" ═══════════════════════════════════════════════════════════════════════════

" ── 1a. Block Lua: .lua / .endlua ────────────────────────────────────────
"    The region starts at a line beginning with  .lua  (with optional
"    trailing whitespace or comment) and ends at  .endlua .
"    Everything inside is highlighted as Lua.

syn region ppluaLuaBlock
      \ matchgroup=ppluaLuaDelimiter
      \ start=/^\.\s*lua\s*$/
      \ end=/^\.\s*endlua\s*$/
      \ contains=@ppluaLuaCode
      \ keepend

" ── 1b. Inline Lua: \lua'...' ────────────────────────────────────────────
"    A single expression delimited by \lua'...' — the value is inserted
"    into the groff text.  We handle nested single-quotes via Lua's
"    string escaping; for syntax purposes we match non-greedily.

syn region ppluaLuaInline
      \ matchgroup=ppluaLuaInlineDelim
      \ start=/\\lua'/
      \ end=/'/
      \ contains=@ppluaLuaCode
      \ oneline
      \ keepend

" ═══════════════════════════════════════════════════════════════════════════
"  SECTION 2: GROFF COMMENTS
" ═══════════════════════════════════════════════════════════════════════════

" Line comment:  .\"  or  .\#  (groff extension)
" Everything after the comment marker to EOL is a comment.
syn match groffComment /^\.\s*\\["#].*$/ contains=groffTodo

" Inline comment escape:  \"  anywhere in a line (rest of line is comment)
syn match groffInlineComment /\\".*$/ contained contains=groffTodo

" The \# comment (groff extension, no newline in output)
syn match groffInlineComment2 /\\#.*$/ contained contains=groffTodo

" TODO/FIXME/XXX/NOTE markers inside comments
syn keyword groffTodo TODO FIXME XXX NOTE HACK BUG WARN contained

" ═══════════════════════════════════════════════════════════════════════════
"  SECTION 3: GROFF REQUESTS
" ═══════════════════════════════════════════════════════════════════════════

" A request line starts with  .  or  '  (control/no-break control character)
" at column 1, followed by a 1-or-2 character request name (or longer for
" groff extended names).
"
" We distinguish several categories for richer highlighting.

" ── 3a. Page/text formatting requests ────────────────────────────────────
syn match groffRequest /^\.\s*\(br\|sp\|fi\|nf\|na\|ad\|ce\|ti\|in\|ll\|ls\|vs\|pl\|bp\|ne\|mk\|rt\|sv\|os\|ns\|rs\|ta\|tc\|lc\|fc\|lg\|hy\|nh\|hc\|hw\|hla\|hlm\|hpf\|hpfa\|hpfcode\|ss\|cs\|bd\|ul\|cu\|cc\|c2\|eo\|ec\|tr\|trin\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" No-break control versions (leading ' instead of .)
syn match groffRequest /^'\s*\(br\|sp\|fi\|nf\|na\|ad\|ce\|ti\|in\|ll\|ls\|vs\|pl\|bp\|ne\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3b. Font and size requests ───────────────────────────────────────────
syn match groffRequestFont /^\.\s*\(ft\|ps\|fam\|fp\|sty\|ftr\|fzoom\|fspecial\|fchar\|fschar\|fcolor\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3c. Number register and string requests ──────────────────────────────
syn match groffRequestRegister /^\.\s*\(nr\|rr\|af\|rnn\|aln\)\>/
      \ contains=groffInlineComment,groffInlineComment2

syn match groffRequestString /^\.\s*\(ds\|ds1\|as\|as1\|rm\|rn\|als\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3d. Macro definition requests ────────────────────────────────────────
"    .de, .dei, .de1, .am, .ami, .am1, .ig  ...  followed by ..
syn match groffRequestMacro /^\.\s*\(de\|de1\|dei\|am\|am1\|ami\|ig\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" End-of-macro marker
syn match groffMacroEnd /^\.\.\s*$/

" ── 3e. Conditional and flow-control requests ────────────────────────────
syn match groffRequestConditional /^\.\s*\(if\|ie\|el\|while\|break\|continue\|return\|shift\|do\|nop\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3f. Trap, environment, and diversion requests ────────────────────────
syn match groffRequestDiversion /^\.\s*\(di\|da\|box\|boxa\|ev\|evc\|wh\|ch\|dt\|it\|itc\|em\|blm\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3g. Input/output and file requests ───────────────────────────────────
syn match groffRequestIO /^\.\s*\(so\|soquiet\|mso\|msoquiet\|nx\|rd\|pi\|sy\|pso\|open\|opena\|close\|write\|writec\|writem\|lf\|cf\|trf\|output\)\>/
      \ contains=groffInlineComment,groffInlineComment2

" ── 3h. Miscellaneous / general catch-all ────────────────────────────────
"    Any remaining . or ' at the start of a line followed by letters
"    that we haven't matched above (e.g., user-defined macros, or
"    macro-package macros like .TL, .SH, .PP, etc.)
syn match groffMacroCall /^\.\s*[