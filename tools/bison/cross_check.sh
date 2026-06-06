#!/usr/bin/env bash
# Cross-check the hand-built SLR(1) parser against Bison's LALR(1) parser.
#
#   usage: cross_check.sh <minipascal-binary> <file.pas> [file.pas ...]
#
# For each sample we compare two accept/reject verdicts:
#   - ours  : `minipascal lr <file>` exit code (our SLR(1) shift-reduce parser)
#   - theirs: the Bison/Flex oracle built from pascal.y + pascal.l (LALR(1))
# Because LALR(1) is at least as powerful as SLR(1) over the same grammar, the two
# must agree on every input; a disagreement means a bug in our table construction.
#
# The script degrades gracefully: if bison or flex is not installed it prints an
# install hint and exits 0, so `make cross-check` never breaks a build on a machine
# without the tools (the hand-built parser is still exercised by `make test`).
set -u

here="$(cd "$(dirname "$0")" && pwd)"

if [ "$#" -lt 2 ]; then
    echo "usage: $0 <minipascal-binary> <file.pas> [file.pas ...]" >&2
    exit 2
fi
bin="$1"; shift

if ! command -v bison >/dev/null 2>&1 || ! command -v flex >/dev/null 2>&1; then
    echo "cross-check skipped: bison and/or flex not found on PATH."
    echo "  install with:  sudo apt-get install bison flex     (Debian/Ubuntu)"
    echo "                 sudo dnf install bison flex          (Fedora)"
    echo "  the hand-built SLR(1) parser is still verified by 'make test'."
    exit 0
fi

cc="${CC:-cc}"

echo "Building the Bison(LALR(1)) + Flex oracle ..."
if ! bison -d -o "$here/pascal.tab.c" "$here/pascal.y" 2> "$here/bison.stderr"; then
    echo "error: bison failed to build the grammar:" >&2
    cat "$here/bison.stderr" >&2
    exit 1
fi
if grep -qi 'conflict' "$here/bison.stderr"; then
    echo "  NOTE: Bison reported conflicts (unexpected for this grammar):"
    sed 's/^/    /' "$here/bison.stderr"
else
    echo "  Bison built a clean LALR(1) automaton (0 conflicts) - agrees with our SLR(1)."
fi

if ! flex -o "$here/lex.yy.c" "$here/pascal.l" 2> "$here/flex.stderr"; then
    echo "error: flex failed to build the scanner:" >&2
    cat "$here/flex.stderr" >&2
    exit 1
fi
if ! "$cc" -O2 -o "$here/pascal_oracle" "$here/pascal.tab.c" "$here/lex.yy.c" 2> "$here/cc.stderr"; then
    echo "error: failed to compile the oracle:" >&2
    cat "$here/cc.stderr" >&2
    exit 1
fi

echo ""
printf "  %-30s %-10s %-10s %s\n" "FILE" "SLR(ours)" "LALR(gnu)" "AGREE"
printf "  %s\n" "---------------------------------------------------------------"

fail=0
for f in "$@"; do
    if [ ! -f "$f" ]; then
        printf "  %-30s %-10s %-10s %s\n" "$f" "-" "-" "MISSING"
        fail=$((fail + 1))
        continue
    fi
    "$bin" lr "$f" >/dev/null 2>&1 && ours="accept" || ours="reject"
    "$here/pascal_oracle" "$f" >/dev/null 2>&1 && theirs="accept" || theirs="reject"
    if [ "$ours" = "$theirs" ]; then agree="yes"; else agree="NO"; fail=$((fail + 1)); fi
    printf "  %-30s %-10s %-10s %s\n" "$f" "$ours" "$theirs" "$agree"
done

echo ""
if [ "$fail" -eq 0 ]; then
    echo "SLR(1) and LALR(1) agree on every sample. Cross-check PASSED."
    exit 0
else
    echo "$fail disagreement(s)/error(s). Cross-check FAILED."
    exit 1
fi
