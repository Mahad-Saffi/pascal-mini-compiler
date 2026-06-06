#include "parsers/lr.hpp"

#include "util/text.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <set>

namespace mp {

LRParser::LRParser(const std::vector<Token>& tokens, const Grammar& original,
                   const SLRTable& table, ErrorReporter& reporter)
    : tokens_(tokens), table_(table), firstFollow_(original), reporter_(reporter) {}

const Token& LRParser::cur() const {
    return tokens_[std::min(position_, tokens_.size() - 1)];
}

std::string LRParser::termAt(size_t i) const {
    if (i >= tokens_.size()) return FirstFollow::kEnd;
    return grammarTerminal(tokens_[i]);
}

std::string LRParser::curText() const {
    const Token& token = cur();
    return token.type == TokType::EndOfFile ? "<eof>" : token.lexeme;
}

void LRParser::report(int line, int col, const std::string& msg) {
    reporter_.report(Phase::Syntactic, line, col, msg);
}

bool LRParser::recover(std::vector<int>& states, std::vector<std::string>& symbols,
                       std::ostream& trace) {
    static const std::vector<std::string> kSync = {
        "<statement>", "<declarations>", "<compound_statement>",
        "<expression>", "<program>"};

    for (const std::string& syncNonterminal : kSync) {
        const std::set<std::string>& followSet = firstFollow_.follow(syncNonterminal);
        for (int level = static_cast<int>(states.size()) - 1; level >= 0; --level) {
            int gotoTarget = table_.gotoState(states[level], syncNonterminal);
            if (gotoTarget < 0) continue;

            size_t savedPosition = position_;
            while (curTerm() != FirstFollow::kEnd && !followSet.count(curTerm())) ++position_;

            if (table_.action(gotoTarget, curTerm()).kind != SLRTable::ActionKind::Error) {
                states.resize(static_cast<size_t>(level) + 1);
                symbols.resize(static_cast<size_t>(level));
                symbols.push_back(syncNonterminal);
                states.push_back(gotoTarget);
                trace << "  -> recover: pop to a state expecting " << syncNonterminal
                      << ", skip to '" << curText() << "', GOTO state " << gotoTarget << "\n";
                return true;
            }
            position_ = savedPosition;
        }
    }

    if (curTerm() != FirstFollow::kEnd) {
        trace << "  -> recover: discard '" << curText() << "'\n";
        ++position_;
        return true;
    }
    return false;
}

void LRParser::traceRow(std::ostream& trace, const std::vector<std::string>& symbols,
                        const std::vector<int>& states, const std::string& action) const {
    std::string symbolText;
    for (const std::string& symbol : symbols) symbolText += symbol + " ";
    if (!symbolText.empty()) symbolText.pop_back();

    std::string stateText;
    for (size_t i = 0; i < states.size(); ++i) {
        if (i) stateText += ' ';
        stateText += std::to_string(states[i]);
    }

    std::string inputText;
    for (size_t i = position_; i < tokens_.size(); ++i) inputText += termAt(i) + " ";
    if (!inputText.empty()) inputText.pop_back();

    trace << "  " << std::left << std::setw(26) << clipColumn(stateText, 26, true)
          << std::setw(28) << clipColumn(symbolText, 28, true)
          << std::setw(26) << clipColumn(inputText, 26, false) << action << "\n";
}

bool LRParser::parse(std::ostream& trace) {
    std::vector<int>         states{0};
    std::vector<std::string> symbols;

    trace << "SLR(1) shift-reduce parse (table-driven over the Original Grammar):\n\n";
    trace << "  " << std::left << std::setw(26) << "STATE STACK"
          << std::setw(28) << "SYMBOL STACK" << std::setw(26) << "INPUT" << "ACTION\n";
    trace << "  " << std::string(96, '-') << "\n";

    const long guardMax = static_cast<long>(tokens_.size()) * 50 + 1000;
    for (long guard = 0; guard < guardMax; ++guard) {
        int state = states.back();
        std::string terminal = curTerm();
        SLRTable::Action action = table_.action(state, terminal);

        switch (action.kind) {
            case SLRTable::ActionKind::Shift:
                traceRow(trace, symbols, states, "shift " + std::to_string(action.value));
                symbols.push_back(terminal);
                states.push_back(action.value);
                if (position_ + 1 < tokens_.size()) ++position_;
                break;

            case SLRTable::ActionKind::Reduce: {
                const Production& production = table_.production(action.value);
                traceRow(trace, symbols, states, "reduce  " + ruleText(production));
                for (size_t k = 0; k < production.rhs.size(); ++k) {
                    if (!states.empty())  states.pop_back();
                    if (!symbols.empty()) symbols.pop_back();
                }
                int topState = states.back();
                int gotoTarget = table_.gotoState(topState, production.lhs);
                if (gotoTarget < 0) {
                    report(cur().line, cur().col,
                           "no GOTO for " + production.lhs + " in state " + std::to_string(topState));
                    if (!recover(states, symbols, trace)) goto done;
                    break;
                }
                symbols.push_back(production.lhs);
                states.push_back(gotoTarget);
                break;
            }

            case SLRTable::ActionKind::Accept:
                traceRow(trace, symbols, states, "accept");
                goto done;

            case SLRTable::ActionKind::Error:
            default:
                traceRow(trace, symbols, states, "error: unexpected '" + curText() + "'");
                report(cur().line, cur().col, "unexpected '" + curText() + "'");
                if (!recover(states, symbols, trace)) goto done;
                break;
        }
    }

done:
    bool accepted = reporter_.count(Phase::Syntactic) == 0;
    trace << "\nResult: " << (accepted ? "ACCEPTED" : "REJECTED")
          << " by the LR parser.\n";
    return accepted;
}

}
