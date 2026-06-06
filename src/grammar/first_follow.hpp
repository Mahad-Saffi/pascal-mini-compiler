#ifndef MINIPASCAL_GRAMMAR_FIRST_FOLLOW_HPP
#define MINIPASCAL_GRAMMAR_FIRST_FOLLOW_HPP

#include "grammar/grammar.hpp"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mp {

class FirstFollow {
public:
    explicit FirstFollow(const Grammar& grammar);

    static const std::string kEps;
    static const std::string kEnd;

    const std::set<std::string>& first(const std::string& symbol) const;
    const std::set<std::string>& follow(const std::string& nonterminal) const;

    std::set<std::string> firstOfSeq(const std::vector<std::string>& sequence) const;

    void print(std::ostream& out) const;

private:
    const Grammar&                               grammar_;
    std::vector<std::string>                     nonterminalOrder_;
    std::map<std::string, std::set<std::string>> firstSets_;
    std::map<std::string, std::set<std::string>> followSets_;

    void computeFirst();
    void computeFollow();
};

}

#endif
