# Mini Pascal Compiler (CS-471L)

A single C++17 program implementing six compiler front-end modules over the
assigned **Subset of Pascal**: a lexer, a recursive descent parser, a predictive
LL(1) parser, an SLR(1) LR parser, a symbol table, and a cross-cutting error
handler. The three parsers consume one shared token stream and must agree on what
they accept and reject.

See [PLAN.md](PLAN.md) for the build blueprint, [CONTEXT.md](CONTEXT.md) for the
domain glossary, and [docs/adr/](docs/adr/) for design decisions. The Original
Grammar (the source of truth) lives in
[docs/grammar/original.bnf](docs/grammar/original.bnf), with rationale in
[docs/grammar/notes.md](docs/grammar/notes.md).

## Build

Requires a C++17 compiler (`g++` or `clang++`) and `make`. No third-party
dependencies — standard library only.

    make            # builds bin/minipascal
    ./bin/minipascal

Convenience targets:

    make test           # cross-parser harness over every test/*.pas
    make outputs        # regenerate output/ (per sample x module)
    make report-tables  # dump grammar / parse tables to docs/report/
    make cross-check    # compare our SLR(1) parser against a Bison LALR(1) oracle
    make clean          # remove objects, the binary, and generated oracle files

`make clean` removes the objects and the binary. Graphviz (`dot`), `bison` and
`flex` are optional: `dot` renders the AST `.dot` files to images, and
`bison`/`flex` build the cross-check oracle. Their absence never breaks the build —
`make cross-check` prints an install hint and skips when they are missing.

## Status

**All phases complete (P0–P7).** Scaffold + diagnostics (P0), the lexer with double
buffering (P1), the grammar engine (P2: BNF loading, left-recursion removal,
left-factoring, FIRST/FOLLOW, the LL(1) table, the SLR(1) automaton), the recursive
descent parser + AST (P3), the symbol table + semantic analysis (P4), the
predictive and LR parsers + cross-parser harness (P5), error-recovery depth (P6),
and the visualization/output bonuses (P7). Scope ends at semantic analysis — there
is no code generation.

Running the binary with no arguments opens an interactive menu. With arguments it
runs one command; grammar commands default to
[docs/grammar/original.bnf](docs/grammar/original.bnf).

    ./bin/minipascal lex <file>      tokenize a .pas file
    ./bin/minipascal rdp <file>      recursive-descent parse + print the AST
    ./bin/minipascal dot <file>      emit the AST as a Graphviz DOT digraph
    ./bin/minipascal ll1 <file>      LL(1) predictive parse + trace
    ./bin/minipascal lr <file>       SLR(1) shift-reduce parse + trace
    ./bin/minipascal symtab <file>   build and dump the symbol table
    ./bin/minipascal semant <file>   semantic analysis (types + symbol table)
    ./bin/minipascal all <file>      every parser + semantics on one file
    ./bin/minipascal test <file...>  cross-parser harness (assert RDP/LL(1)/LR agree)
    ./bin/minipascal outputs <file>  write per-module artifacts to output/
    ./bin/minipascal grammar         Original + Transformed grammars
    ./bin/minipascal first-follow    FIRST/FOLLOW for the Transformed grammar
    ./bin/minipascal ll1-table       LL(1) table + conflicts
    ./bin/minipascal slr-table       SLR(1) item sets, ACTION/GOTO, conflicts
    ./bin/minipascal report-tables   dump all of the above to docs/report/

The LL(1) table reports one conflict — `M[<statement>, id]` (assignment vs.
procedure call) — which is inherent and not removable by left-factoring. The same
grammar is **SLR(1) with zero conflicts**, since the LR automaton distinguishes the
two cases by the token after the identifier. `make cross-check` confirms a
Bison-generated **LALR(1)** parser accepts/rejects exactly the same programs.

## Layout

| Path | Contents |
|---|---|
| `src/` | compiler source, one folder per module |
| `docs/grammar/` | the Original Grammar and notes |
| `docs/adr/` | architecture decision records |
| `docs/report/` | grammar / FIRST-FOLLOW / LL(1) / SLR table dumps (`make report-tables`) |
| `test/` | sample `.pas` programs (valid and invalid) |
| `output/` | generated per-module outputs (`make outputs`) |
| `tools/bison/` | Bison + Flex SLR-vs-LALR cross-check oracle (`make cross-check`) |
