#ifndef MINIPASCAL_GRAMMAR_LL1_TABLE_HPP
#define MINIPASCAL_GRAMMAR_LL1_TABLE_HPP

#include "grammar/first_follow.hpp"
#include "grammar/grammar.hpp"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace mp {

class LL1Table {
public:
    LL1Table(const Grammar& grammar, const FirstFollow& firstFollow);

    struct Conflict {
        std::string      nonterminal;
        std::string      lookahead;
        std::vector<int> productions;
    };

    int cell(const std::string& nonterminal, const std::string& term) const;

    bool hasConflicts() const { return !conflicts_.empty(); }
    const std::vector<Conflict>& conflicts() const { return conflicts_; }

    void print(std::ostream& out) const;
    void printConflicts(std::ostream& out) const;

private:
    const Grammar&                                                 grammar_;
    std::vector<std::string>                                       nonterminalOrder_;
    std::map<std::string, std::map<std::string, std::vector<int>>> table_;
    std::vector<Conflict>                                          conflicts_;
};

}

#endif
