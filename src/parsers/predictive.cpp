#include "parsers/predictive.hpp"

#include "util/text.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <ostream>

namespace mp {
namespace {

std::string phraseFor(const std::string& nonterminal) {
    static const std::map<std::string, std::string> kPhrase = {
        {"<program>",                 "the program"},
        {"<identifier_list>",         "an identifier list"},
        {"<declarations>",            "a variable declaration"},
        {"<type>",                    "a type"},
        {"<standard_type>",           "a type"},
        {"<subprogram_declarations>", "a subprogram declaration"},
        {"<subprogram_declaration>",  "a subprogram declaration"},
        {"<subprogram_head>",         "a subprogram header"},
        {"<arguments>",               "an argument list"},
        {"<parameter_list>",          "a parameter list"},
        {"<compound_statement>",      "a statement block"},
        {"<optional_statements>",     "a statement block"},
        {"<statement_list>",          "a statement"},
        {"<statement>",               "a statement"},
        {"<variable>",                "a variable"},
        {"<procedure_statement>",     "a procedure call"},
        {"<expression_list>",         "an argument list"},
        {"<expression>",              "an expression"},
        {"<simple_expression>",       "an expression"},
        {"<term>",                    "a term"},
        {"<factor>",                  "a factor"},
        {"<sign>",                    "a sign"},
        {"<addop>",                   "an operator"},
        {"<mulop>",                   "an operator"},
        {"<relop>",                   "a relational operator"},
    };
    std::string base = nonterminal;
    auto primePos = base.find('\'');
    if (primePos != std::string::npos) base.erase(base.begin() + primePos, base.end() - 1);
    auto found = kPhrase.find(base);
    return found != kPhrase.end() ? found->second : nonterminal;
}

std::string phraseForTerminal(const std::string& terminal) {
    if (terminal == "id")              return "an identifier";
    if (terminal == "num")             return "a number";
    if (terminal == "assignop")        return "':='";
    if (terminal == FirstFollow::kEnd) return "end of input";
    return "'" + terminal + "'";
}

}

PredictiveParser::PredictiveParser(const std::vector<Token>& tokens,
                                   const Grammar& transformed, const LL1Table& table,
                                   ErrorReporter& reporter)
    : tokens_(tokens), grammar_(transformed), table_(table), firstFollow_(transformed),
      reporter_(reporter) {}

const Token& PredictiveParser::cur() const {
    return tokens_[std::min(position_, tokens_.size() - 1)];
}

std::string PredictiveParser::termAt(size_t i) const {
    if (i >= tokens_.size()) return FirstFollow::kEnd;
    return grammarTerminal(tokens_[i]);
}

std::string PredictiveParser::curText() const {
    const Token& token = cur();
    return token.type == TokType::EndOfFile ? "<eof>" : token.lexeme;
}

void PredictiveParser::report(int line, int col, const std::string& msg) {
    reporter_.report(Phase::Syntactic, line, col, msg);
}

int PredictiveParser::pickStatementProd(const std::string& afterId) const {
    const bool assign = (afterId == "assignop" || afterId == "[");
    const std::string wantedHead = assign ? "<variable>" : "<procedure_statement>";
    for (int idx : grammar_.productionsFor("<statement>")) {
        const Production& production = grammar_.productions()[idx];
        if (!production.rhs.empty() && production.rhs[0] == wantedHead) return idx;
    }
    return -1;
}

bool PredictiveParser::panicSync(std::vector<std::string>& stack, const std::string& nonterminal) {
    const std::set<std::string>& followSet = firstFollow_.follow(nonterminal);
    for (;;) {
        std::string terminal = curTerm();
        if (terminal == FirstFollow::kEnd) { stack.pop_back(); return true; }
        if (followSet.count(terminal))     { stack.pop_back(); return true; }
        if (table_.cell(nonterminal, terminal) >= 0) return true;
        ++position_;
    }
}

void PredictiveParser::traceRow(std::ostream& trace, const std::vector<std::string>& stack,
                                const std::string& action) const {
    std::string stackText;
    for (const std::string& symbol : stack) stackText += symbol + " ";
    if (!stackText.empty()) stackText.pop_back();

    std::string inputText;
    for (size_t i = position_; i < tokens_.size(); ++i) inputText += termAt(i) + " ";
    if (!inputText.empty()) inputText.pop_back();

    trace << "  " << std::left << std::setw(46) << clipColumn(stackText, 46, true)
          << std::setw(34) << clipColumn(inputText, 34, false) << action << "\n";
}

bool PredictiveParser::parse(std::ostream& trace) {
    std::vector<std::string> stack;
    stack.push_back(FirstFollow::kEnd);
    stack.push_back(grammar_.start());

    trace << "LL(1) predictive parse (table-driven over the Transformed Grammar):\n\n";
    trace << "  " << std::left << std::setw(46) << "STACK (bottom..top)"
          << std::setw(34) << "INPUT" << "ACTION\n";
    trace << "  " << std::string(92, '-') << "\n";

    for (;;) {
        if (stack.empty()) break;
        std::string symbol = stack.back();
        std::string terminal = curTerm();

        if (symbol == FirstFollow::kEnd) {
            if (terminal == FirstFollow::kEnd) {
                traceRow(trace, stack, "accept");
                stack.pop_back();
                break;
            }
            report(cur().line, cur().col,
                   "unexpected '" + curText() + "' after the end of the program");
            break;
        }

        if (!grammar_.isNonterminal(symbol)) {
            if (symbol == terminal) {
                traceRow(trace, stack, "match " + terminal);
                stack.pop_back();
                if (position_ + 1 < tokens_.size()) ++position_;
            } else {
                traceRow(trace, stack, "error: expected '" + symbol + "'");
                report(cur().line, cur().col,
                       "missing " + phraseForTerminal(symbol) + " before '" + curText() + "'");
                stack.pop_back();
            }
            continue;
        }

        int productionIndex = (symbol == "<statement>" && terminal == "id")
                                  ? pickStatementProd(termAt(position_ + 1))
                                  : table_.cell(symbol, terminal);

        if (productionIndex < 0) {
            traceRow(trace, stack, "error: no rule for (" + symbol + ", " + terminal + ")");
            report(cur().line, cur().col,
                   "unexpected '" + curText() + "' while parsing " + phraseFor(symbol));
            if (!panicSync(stack, symbol)) break;
            continue;
        }

        const Production& production = grammar_.productions()[productionIndex];
        traceRow(trace, stack, "output  " + ruleText(production));
        stack.pop_back();
        for (size_t i = production.rhs.size(); i-- > 0;) stack.push_back(production.rhs[i]);
    }

    bool accepted = reporter_.count(Phase::Syntactic) == 0;
    trace << "\nResult: " << (accepted ? "ACCEPTED" : "REJECTED")
          << " by the predictive parser.\n";
    return accepted;
}

}
