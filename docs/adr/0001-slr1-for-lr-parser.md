# Use SLR(1) for the LR Parser

§4.4 requires an LR parser and a justified variant. We chose **SLR(1)**: it is
the simplest variant that handles the Original Grammar, it reuses the FOLLOW sets
already produced by the grammar engine, and it yields compact ACTION/GOTO tables
that fit in the report.

The assigned grammar makes `else` mandatory
(`statement → if expression then statement else statement` only), so there is
**no dangling-else** ambiguity to resolve. The one decision point is `id` as the
head of both `variable` and `procedure_statement` in a statement; SLR resolves it
by lookahead because FOLLOW(variable) = { assignop } is disjoint from
FOLLOW(procedure_statement). Any residual shift/reduce that appears during
construction is reported and resolved by preferring shift, the same resolution a
stronger variant would apply — so LALR(1) or canonical LR(1) buy nothing here.

## Considered options

- **LALR(1)** — matches YACC/Bison exactly, so the hand-vs-Bison table
  cross-check (a listed bonus) would line up one-to-one. Rejected as the default
  because it needs LR(1) item construction + core merging for no behavioural gain
  on this grammar. We still run Bison and discuss the SLR/LALR difference for the
  bonus.
- **Canonical LR(1)** — most states, hardest to print compactly; overkill.
- **LR(0)** — too weak; reduce actions without lookahead conflict on this grammar.
