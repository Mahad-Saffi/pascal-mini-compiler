# Mini Pascal Compiler - build (C++17, standard library only).
# Sources are discovered with find, so adding a .cpp under src/ needs no edit here.

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Isrc -MMD -MP
LDFLAGS  :=

BIN_DIR := bin
BIN     := $(BIN_DIR)/minipascal

SRCS := $(shell find src -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# Sample driven by the FILE-based targets once the CLI lands (P3).
FILE ?= test/gcd.pas

# Every bundled sample - fed to the harness and the output/ generator.
SAMPLES := $(wildcard test/*.pas)

.PHONY: all run web test outputs report-tables cross-check clean

all: $(BIN)

$(BIN): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build, then run every stage (all three parsers + AST + symbol table + semantics)
# on FILE. Run the bare binary with no arguments for the interactive menu instead.
run: $(BIN)
	./$(BIN) all $(FILE)

# Presentation UI: a local browser front end (tools/web) that shells out to the
# binary for every stage and draws the AST as an interactive SVG tree. Needs only
# python3 (standard library); open the printed http://127.0.0.1:8000 URL.
web: $(BIN)
	python3 tools/web/server.py

# Cross-parser harness: run RDP, Predictive and LR over every sample and assert
# they return the same accept/reject verdict.
test: $(BIN)
	./$(BIN) test test/*.pas

# Regenerate output/ for every sample x module (tokens, AST text + dot, LL(1) and
# LR traces, symbol table, semantic report, consolidated errors).
outputs: $(BIN)
	./$(BIN) outputs $(SAMPLES)

# Dump grammar / FIRST / FOLLOW / LL(1) / SLR tables to docs/report for the report.
report-tables: $(BIN)
	./$(BIN) report-tables

# Cross-check the hand-built SLR(1) parser against a Bison-generated LALR(1) parser
# (bonus). Builds the oracle from tools/bison/{pascal.y,pascal.l}; skips gracefully
# with an install hint when bison/flex are absent, so it never breaks the build.
cross-check: $(BIN)
	@bash tools/bison/cross_check.sh ./$(BIN) $(SAMPLES)

# Generated oracle artifacts (only present after a cross-check on a bison machine).
ORACLE_GEN := tools/bison/pascal.tab.c tools/bison/pascal.tab.h tools/bison/lex.yy.c \
              tools/bison/pascal_oracle tools/bison/bison.stderr \
              tools/bison/flex.stderr tools/bison/cc.stderr

clean:
	$(RM) $(OBJS) $(DEPS) $(BIN) $(ORACLE_GEN)

-include $(DEPS)
