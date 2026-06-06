# Grammar notes

Companion to [original.bnf](original.bnf) — the Original Grammar that drives the
whole grammar engine (Transformed Grammar, FIRST/FOLLOW, the LL(1) table, and the
SLR ACTION/GOTO tables).

## BNF notation (what the P2 loader parses)

- `<name>` — a **nonterminal**: `<`, an identifier (`[A-Za-z_][A-Za-z0-9_]*`), `>`,
  with no internal spaces (so the wrapped token is at least 3 characters).
- any other whitespace-delimited token — a **terminal** (keyword, `id`, `num`,
  punctuation, or operator).
- `|` — separates alternatives for one left-hand side.
- `epsilon` — the empty string (an ε-production).
- `#` — comment to end of line; blank lines are ignored.
- A production is `<lhs> -> rhs`; alternatives may continue on following lines
  whose first non-space character is `|`.

### Why the angle brackets

The start symbol `program` collides with the `program` **keyword** terminal in the
program header. Wrapping every nonterminal in `<…>` removes the collision: the
nonterminal is `<program>`, the keyword is the bare terminal `program`.

### Terminals that contain `<` or `>`

The relational terminals `<`, `<=`, `>=`, `>`, `<>` look like angle brackets. The
loader rule keeps them unambiguous: a token is a nonterminal **only** when it
opens with `<`, closes with `>`, and wraps an identifier (length ≥ 3). So `<>`
(length 2) and `<=` (does not close with `>`) read as terminals, while
`<program>` reads as a nonterminal.

## Operator classes: addop / mulop / relop

The assigned grammar uses `addop`, `mulop`, `relop` as **terminal token classes**
but writes `sign -> + | -` with literal operators. Taken literally that is
inconsistent: the lexeme `+` would have to be both the terminal `+` (for `sign`)
and a member of the `addop` class (for `simple_expression`).

We resolve it by writing the three classes as **enumerating nonterminals**:

    <addop> -> + | - | or
    <mulop> -> * | / | div | mod | and
    <relop> -> = | <> | < | <= | >= | >

This is **language-preserving** (the set of accepted programs is identical) and:

- lets the lexer emit one **concrete** terminal per operator (`+`, `or`, `*`, …)
  instead of merging operators into classes and stashing the choice in an
  attribute;
- gives all three parsers the same concrete-terminal alphabet;
- keeps `sign -> + | -` correct without the lexer special-casing `+`/`-`.

The parsers disambiguate `+`/`-` between `sign` and `addop` by position, so the
lexer never has to. The report shows the assigned grammar as given alongside this
refactor.

## Token → grammar-terminal mapping

| Lexer token | Lexeme(s) | Grammar terminal |
|---|---|---|
| identifier | letter (letter \| digit)* | `id` |
| integer constant | digit+ | `num` |
| real constant | digit+ `.` digit+ (`[eE][+-]?` digit+)? | `num` |
| keyword | `program var array of integer real function procedure begin end if then else while do not div mod and or` | the keyword itself |
| `:=` | `:=` | `assignop` |
| additive | `+` `-` `or` | members of `<addop>` |
| multiplicative | `*` `/` `div` `mod` `and` | members of `<mulop>` |
| relational | `=` `<>` `<` `<=` `>=` `>` | members of `<relop>` |
| punctuation | `( ) [ ] ; : , . ..` | same |
| end of file | — | `$` (end marker, added by the engine) |

`+` and `-` carry two grammar roles (`sign` and `<addop>`); the lexer emits the
single terminal and the parsers pick the role by position. Integer and real
constants both map to `num`; the int/real distinction is preserved in the token
attribute for the semantic pass. The single `.` is a valid token but is never used
by the grammar (the program has no terminating dot — see below), so a stray `.`
is a syntax error.

## Language facts that constrain the sample programs

- **No terminating `.`** — the assigned program rule ends at `compound_statement`,
  with no trailing dot (unlike standard Pascal). The samples in `test/` follow this.
- **`else` is mandatory** — the only conditional is
  `if expression then statement else statement`; there is no else-less `if`, so
  there is no dangling-else ambiguity.
- **Arrays are write-only in expressions** — `variable -> id [ expression ]`
  allows an indexed element only as an **assignment target**. `factor` has no
  `id [ expression ]` form, so an array element can never be **read** inside an
  expression. A valid array program can fill array elements but cannot use them.
- **Each declaration group repeats `var`** — `declarations -> declarations var
  identifier_list : type ;`, so two groups read `var a : integer ; var b : real ;`
  (not one `var` spanning several lines).
- **`read` / `write` / `readln` / `writeln` are built-in procedures**, not
  keywords. The semantic pass pre-declares them (and the program-header file
  identifiers such as `input` / `output`) in the outermost scope so calls resolve.
