#ifndef MINIPASCAL_PARSERS_LR_HPP
#define MINIPASCAL_PARSERS_LR_HPP

#include "diagnostics.hpp"
#include "grammar/first_follow.hpp"
#include "grammar/grammar.hpp"
#include "grammar/slr_table.hpp"
#include "lexer/token.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace mp {

class LRParser {
public:
    LRParser(const std::vector<Token>& tokens, const Grammar& original,
             const SLRTable& table, ErrorReporter& reporter);

    bool parse(std::ostream& trace);

private:
    std::vector<Token> tokens_;
    const SLRTable&    table_;
    FirstFollow               firstFollow_;
    ErrorReporter&            reporter_;
    size_t                    position_ = 0;

    const Token& cur() const;
    std::string  termAt(size_t i) const;
    std::string  curTerm() const { return termAt(position_); }
    std::string  curText() const;

    bool recover(std::vector<int>& states, std::vector<std::string>& symbols,
                 std::ostream& trace);
    void report(int line, int col, const std::string& msg);
    void traceRow(std::ostream& trace, const std::vector<std::string>& symbols,
                  const std::vector<int>& states, const std::string& action) const;
};

}

#endif
