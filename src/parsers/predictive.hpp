#ifndef MINIPASCAL_PARSERS_PREDICTIVE_HPP
#define MINIPASCAL_PARSERS_PREDICTIVE_HPP

#include "diagnostics.hpp"
#include "grammar/first_follow.hpp"
#include "grammar/grammar.hpp"
#include "grammar/ll1_table.hpp"
#include "lexer/token.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace mp {

class PredictiveParser {
public:
    PredictiveParser(const std::vector<Token>& tokens, const Grammar& transformed,
                     const LL1Table& table, ErrorReporter& reporter);

    bool parse(std::ostream& trace);

private:
    std::vector<Token>  tokens_;
    const Grammar&      grammar_;
    const LL1Table&     table_;
    FirstFollow               firstFollow_;
    ErrorReporter&            reporter_;
    size_t                    position_ = 0;

    const Token& cur() const;
    std::string  termAt(size_t i) const;
    std::string  curTerm() const { return termAt(position_); }
    std::string  curText() const;

    int  pickStatementProd(const std::string& afterId) const;
    bool panicSync(std::vector<std::string>& stack, const std::string& nonterminal);

    void report(int line, int col, const std::string& msg);
    void traceRow(std::ostream& trace, const std::vector<std::string>& stack,
                  const std::string& action) const;
};

}

#endif
