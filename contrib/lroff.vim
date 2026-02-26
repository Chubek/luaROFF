" Vim syntax file
" Language:    lroff — groff/troff with embedded Lua (pplua preprocessor)
" Maintainer:  pplua contributors
" Filenames:   *.lroff *.roff *.groff *.ms *.me *.mom
" Last Change: 2026-02-26
" License:     MIT

if exists("b:current_syntax")
    finish
endif

" --- Settings -----------------------------------------------------------
syn case match
syn sync minlines=300

" ========================================================================
"  1.  COMMENTS
" ========================================================================

" Lines starting with .\" or .\# are comments (also .\" with leading spaces)
syn match groffComment /^\.\s*\\[#"]\s*.*$/ contains=groffTodo
syn match groffComment /^'\s*\\[#"]\s*.*$/ contains=groffTodo
" Inline comment after \"
syn match groffInlineComment /\\[#"].*$/ contained contains=groffTodo
syn keyword groffTodo TODO FIXME XXX HACK NOTE WARN contained

" ========================================================================
"  2.  REQUESTS  (lines starting with . or ')
" ========================================================================

" --- 2a. Page layout & formatting requests ---
syn match groffRequestPage /^\.\s*\<\(br\|sp\|ne\|bp\|pl\|po\|ll\|in\|ti\|ce\|rj\|fi\|nf\|na\|ad\|hy\|nh\|hc\|hw\|hla\|hlm\|hpf\|hpfa\|hpfcode\)\>/ contains=groffInlineComment

" --- 2b. Font & style requests ---
syn match groffRequestFont /^\.\s*\<\(ft\|ps\|vs\|ss\|cs\|bd\|ul\|cu\|uf\|fam\|fcolor\|gcolor\|sty\|fzoom\|fspecial\|ftr\|fp\)\>/ contains=groffInlineComment

" --- 2c. Text & paragraph requests ---
syn match groffRequestText /^\.\s*\<\(par\|PP\|LP\|IP\|TP\|QP\|HP\|XP\|RS\|RE\|SH\|SS\|NH\|TL\|AU\|AI\|AB\|AE\|DA\|ND\|ds\|as\|ds1\|as1\|substring\|length\)\>/ contains=groffInlineComment

" --- 2d. Number register & variable requests ---
syn match groffRequestReg /^\.\s*\<\(nr\|rr\|rnn\|aln\|af\)\>/ contains=groffInlineComment

" --- 2e. Control flow requests ---
syn match groffRequestFlow /^\.\s*\<\(if\|ie\|el\|while\|break\|continue\|do\|nop\|return\)\>/ contains=groffInlineComment

" --- 2f. Macro definition requests ---
syn match groffRequestMacro /^\.\s*\<\(de\|de1\|dei\|dei1\|am\|am1\|ami\|ami1\|als\|rm\|rn\|ig\)\>/ contains=groffInlineComment

" --- 2g. Trap & environment requests ---
syn match groffRequestTrap /^\.\s*\<\(wh\|ch\|dt\|it\|itc\|em\|blm\|ev\|evc\)\>/ contains=groffInlineComment

" --- 2h. Diversion requests ---
syn match groffRequestDivert /^\.\s*\<\(di\|da\|box\|boxa\|output\)\>/ contains=groffInlineComment

" --- 2i. I/O & system requests ---
syn match groffRequestIO /^\.\s*\<\(so\|soquiet\|mso\|msoquiet\|cf\|sy\|pi\|pso\|open\|opena\|close\|write\|writec\|writem\|trf\|nx\|rd\|ex\|ab\|lf\|tm\|tm1\|tmc\)\>/ contains=groffInlineComment

" --- 2j. Table/eqn/pic/refer/soelim preprocessor delimiters ---
syn match groffPreproDelim /^\.\s*\<\(TS\|TE\|TH\|T&\|EQ\|EN\|PS\|PE\|PF\|PY\|GS\|GE\|GF\|G1\|G2\|R1\|R2\|IS\|IE\|DF\|DE\|cstart\|cend\)\>/ contains=groffInlineComment

" --- 2k. Generic fallback: any request at BOL ---
"     Match a dot at BOL followed by 1-6 word characters
syn match groffRequest /^\.\s*\a\w\{0,5\}/ contains=groffInlineComment

" ========================================================================
"  3.  ESCAPES  (inline backslash sequences)
" ========================================================================

" --- 3a. Special characters  \(xx  \[name]  \C'name' ---
syn match groffEscChar /\\(\.\./
syn match groffEscChar /\\\[[^\]]\+\]/
syn match groffEscChar /\\C'[^']\+'/

" --- 3b. String interpolation  \*x  \*(xx  \*[name] ---
syn match groffEscString /\\\*\a/
syn match groffEscString /\\\*(\a\a/
syn match groffEscString /\\\*\[[^\]]\+\]/

" --- 3c. Register interpolation  \nx  \n(xx  \n[name]  \n+[name]  \n-[name] ---
syn match groffEscRegister /\\n[+-]\?\a/
syn match groffEscRegister /\\n[+-]\?(\a\a/
syn match groffEscRegister /\\n[+-]\?\[[^\]]\+\]/

" --- 3d. Font escapes  \fx  \f(xx  \f[name] ---
syn match groffEscFont /\\f\a/
syn match groffEscFont /\\f(\a\a/
syn match groffEscFont /\\f\[[^\]]\+\]/

" --- 3e. Size escapes  \sx  \s(xx  \s[+-N]  \s'N' ---
syn match groffEscSize /\\s[+-]\?\d\{1,2\}/
syn match groffEscSize /\\s[+-]\?(\d\d/
syn match groffEscSize /\\s[+-]\?\[[^\]]\+\]/
syn match groffEscSize /\\s'[^']*'/

" --- 3f. Colour escapes  \m[name]  \M[name] ---
syn match groffEscColour /\\[mM]\[[^\]]\+\]/
syn match groffEscColour /\\[mM]\a/

" --- 3g. Horizontal/vertical motion  \h  \v  \u  \d  \r  \| \^ \0 \~ ---
syn match groffEscMotion /\\[hvwHlLDZoXYkRSAbp]/
syn match groffEscMotion /\\[udrzsc|^0~&)!?%{}/]/

" --- 3h. Numbered character  \N'nnn' ---
syn match groffEscNumChar /\\N'\d\+'/

" --- 3i. Backslash-newline continuation ---
syn match groffContinuation /\\$/

" --- 3j. Escape-to-literal  \e  \\  \&  \)  \!  \?  \/ \, ---
syn match groffEscLiteral /\\[e\\&)!?\/,]/

" ========================================================================
"  4.  STRINGS & SPECIAL INLINE MARKUP
" ========================================================================

" Quoted strings in macro arguments (double-quoted)
syn region groffString start=/"/ end=/"/ contained
" Macro calls with arguments — any line starting with . followed by word chars
syn match groffMacroCall /^\.\s*\a\w*/ contains=groffString,groffInlineComment

" ========================================================================
"  5.  PREPROCESSOR BLOCKS (tbl, eqn, pic, grap, refer, ideal, chem)
" ========================================================================

" --- tbl ---
syn region groffTblBlock start=/^\.\s*TS\s*$/ end=/^\.\s*TE\s*$/ contains=groffTblDelim,groffComment keepend
syn match groffTblDelim /^\.\s*\(TS\|TE\|T&\)\s*$/ contained
syn match groffTblDelim /^\.\s*TH\s*$/ contained

" --- eqn ---
syn region groffEqnBlock start=/^\.\s*EQ\s*$/ end=/^\.\s*EN\s*$/ contains=groffEqnDelim,groffComment keepend
syn match groffEqnDelim /^\.\s*\(EQ\|EN\)\s*$/ contained

" --- pic ---
syn region groffPicBlock start=/^\.\s*PS\s*$/ end=/^\.\s*\(PE\|PF\|PY\)\s*$/ contains=groffPicDelim,groffComment keepend
syn match groffPicDelim /^\.\s*\(PS\|PE\|PF\|PY\)\s*$/ contained

" ========================================================================
"  6.  EMBEDDED LUA  (.lua / .endlua blocks and \lua inline)
" ========================================================================

" Pull in Lua syntax for embedding
if exists("b:current_syntax")
    let s:saved_syntax = b:current_syntax
    unlet b:current_syntax
endif
syn include @luaCode syntax/lua.vim
if exists("s:saved_syntax")
    let b:current_syntax = s:saved_syntax
    unlet s:saved_syntax
else
    unlet! b:current_syntax
endif

" --- 6a. Block Lua: .lua ... .endlua ---
syn region ppluaBlock matchgroup=ppluaDelimiter start=/^\.\s*lua\s*$/ end=/^\.\s*endlua\s*$/ contains=@luaCode keepend

" --- 6b. Inline Lua with single-quote delimiter: \lua'...' ---
syn region ppluaInline matchgroup=ppluaInlineDelim start=/\\lua'/ end=/'/ contains=@luaCode oneline

" --- 6c. Inline Lua with brace delimiter: \lua{...} ---
syn region ppluaInlineBrace matchgroup=ppluaInlineDelim start=/\\lua{/ end=/}/ contains=@luaCode oneline

" --- 6d. Common lroff library names for highlighting inside Lua blocks ---
syn keyword ppluaLroffFunc emit emitln request printf blank contained containedin=@luaCode
syn keyword ppluaLroffFunc escape inline_escape contained containedin=@luaCode
syn keyword ppluaLroffFunc font size with_font with_size contained containedin=@luaCode
syn keyword ppluaLroffFunc nr_set nr_get nr_ref ds_set ds_get ds_ref contained containedin=@luaCode
syn keyword ppluaLroffFunc divert_begin divert_end divert_emit divert_get contained containedin=@luaCode
syn keyword ppluaLroffFunc macro_define macro_define_lua contained containedin=@luaCode
syn keyword ppluaLroffFunc bold italic mono special_char contained containedin=@luaCode
syn keyword ppluaLroffFunc section title author paragraph contained containedin=@luaCode
syn keyword ppluaLroffFunc bullet_list numbered_list def_list contained containedin=@luaCode
syn keyword ppluaLroffFunc unique version contained containedin=@luaCode
syn keyword ppluaLroffFunc map foreach indented contained containedin=@luaCode

" The "lroff" namespace accessor
syn match ppluaLroffNS /\<lroff\>/ contained containedin=@luaCode

" ========================================================================
"  7.  NUMERIC EXPRESSIONS  (used inside requests)
" ========================================================================

" Scaling units attached to numbers:  12p  0.5i  3n  1v  1m  1M  etc.
syn match groffNumExpr /[+-]\?\d\+\(\.\d*\)\?[icpPnmMvsufz]/ contained containedin=groffRequest,groffRequestPage,groffRequestFont,groffRequestReg

" ========================================================================
"  8.  SPECIAL LINE PATTERNS
" ========================================================================

" .lf (line-file directive from pplua)
syn match ppluaLineFile /^\.\s*lf\s\+\d\+\s\+.*$/

" Ignore (ig) block
syn region groffIgnore start=/^\.\s*ig\>/ end=/^\.\.\s*$/ contains=groffComment

" ========================================================================
"  9.  MAN / MS / MOM / ME / MM MACRO HIGHLIGHTS
" ========================================================================

" man(7) macros
syn match groffManMacro /^\.\s*\<\(TH\|SH\|SS\|TP\|IP\|HP\|LP\|PP\|P\|RS\|RE\|UR\|UE\|MT\|ME\|SY\|YS\|OP\|EX\|EE\|MR\|BR\|BI\|IR\|IB\|RI\|RB\|SM\|SB\)\>/ contains=groffString,groffInlineComment

" ms macros
syn match groffMsMacro /^\.\s*\<\(NH\|TL\|AU\|AI\|AB\|AE\|DA\|ND\|FS\|FE\|KS\|KE\|KF\|DS\|DE\|LD\|ID\|CD\|RD\|BD\|QP\|QS\|QE\|XP\|B1\|B2\|MC\|1C\|2C\|AM\|BX\|UL\|LG\|NL\|R\|B\|I\|BI\|BG\|CW\)\>/ contains=groffString,groffInlineComment

" mom macros
syn match groffMomMacro /^\.\s*\<\(START\|DOC_TITLE\|TITLE\|SUBTITLE\|AUTHOR\|CHAPTER\|CHAPTER_TITLE\|HEADING\|PARAHEAD\|SUBHEAD\|PP\|QUOTE\|BLOCKQUOTE\|CODE\|LIST\|ITEM\|ENDLIST\|FOOTNOTE\|ENDNOTE\|TOC\|COLLATE\|NEWPAGE\|PRINTSTYLE\|PAPER\|DOCTYPE\|COPYSTYLE\|FAM\|PT_SIZE\|LS\|JUSTIFY\|LEFT\|RIGHT\|CENTER\|QUAD\)\>/ contains=groffString,groffInlineComment

" ========================================================================
"  10. FOLDING (optional, enabled by g:lroff_fold)
" ========================================================================

if get(g:, 'lroff_fold', 0)
    syn region groffFoldMacro start=/^\.\s*de\>/ end=/^\.\.\s*$/ fold transparent keepend
    syn region groffFoldIgnore start=/^\.\s*ig\>/ end=/^\.\.\s*$/ fold transparent keepend
    syn region ppluaFoldBlock start=/^\.\s*lua\s*$/ end=/^\.\s*endlua\s*$/ fold transparent keepend
    set foldmethod=syntax
endif

" ========================================================================
"  11. SPELL CHECKING
" ========================================================================

" Enable spellcheck in normal text but not in code/escapes
syn cluster groffNoSpell contains=groffEscChar,groffEscString,groffEscRegister,groffEscFont,groffEscSize,groffEscColour,groffEscMotion,groffEscNumChar,groffEscLiteral,ppluaBlock,ppluaInline,ppluaInlineBrace,groffTblBlock,groffEqnBlock,groffPicBlock

" ========================================================================
"  12. HIGHLIGHT LINKS
" ========================================================================

" --- Comments ---
hi def link groffComment        Comment
hi def link groffInlineComment  Comment
hi def link groffTodo           Todo

" --- Requests (by category) ---
hi def link groffRequest        Statement
hi def link groffRequestPage    Statement
hi def link groffRequestFont    Type
hi def link groffRequestText    Keyword
hi def link groffRequestReg     Define
hi def link groffRequestFlow    Conditional
hi def link groffRequestMacro   Macro
hi def link groffRequestTrap    Special
hi def link groffRequestDivert  StorageClass
hi def link groffRequestIO      PreProc
hi def link groffPreproDelim    PreCondit

" --- Escapes ---
hi def link groffEscChar        SpecialChar
hi def link groffEscString      Identifier
hi def link groffEscRegister    Constant
hi def link groffEscFont        Type
hi def link groffEscSize        Number
hi def link groffEscColour      Special
hi def link groffEscMotion      Operator
hi def link groffEscNumChar     Number
hi def link groffEscLiteral     SpecialChar
hi def link groffContinuation   Special

" --- Strings & macro calls ---
hi def link groffString         String
hi def link groffMacroCall      Function

" --- Preprocessor blocks ---
hi def link groffTblDelim       PreCondit
hi def link groffEqnDelim       PreCondit
hi def link groffPicDelim       PreCondit
hi def link groffTblBlock       Normal
hi def link groffEqnBlock       Normal
hi def link groffPicBlock       Normal

" --- Embedded Lua ---
hi def link ppluaDelimiter      PreCondit
hi def link ppluaInlineDelim    PreCondit
hi def link ppluaBlock          Normal
hi def link ppluaLroffFunc      Function
hi def link ppluaLroffNS        Structure

" --- Numeric expressions ---
hi def link groffNumExpr        Number

" --- pplua line directives ---
hi def link ppluaLineFile       Debug

" --- Macro packages ---
hi def link groffManMacro       Keyword
hi def link groffMsMacro        Keyword
hi def link groffMomMacro       Keyword

" --- Ignore blocks ---
hi def link groffIgnore         Comment

let b:current_syntax = "lroff"
