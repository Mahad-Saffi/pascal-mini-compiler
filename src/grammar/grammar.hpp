#ifndef MINIPASCAL_GRAMMAR_GRAMMAR_HPP
#define MINIPASCAL_GRAMMAR_GRAMMAR_HPP

#include <functional>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

namespace mp {

struct Production {
    std::string              lhs;
    std::vector<std::string> rhs;

    bool isEpsilon() const { return rhs.empty(); }
};

class Grammar {
public:
    static Grammar loadBNF(const std::string& path);

    const std::vector<Production>& productions()  const { return productions_; }
    const std::set<std::string>&   nonterminals() const { return nonterminals_; }
    const std::set<std::string>&   terminals()    const { return terminals_; }
    const std::string&             start()        const { return start_; }

    bool isNonterminal(const std::string& symbol) const { return nonterminals_.count(symbol) > 0; }

    std::vector<int> productionsFor(const std::string& nonterminal) const;
    std::vector<std::string> nonterminalsInOrder() const;

    void print(std::ostream& out) const;

    void addProduction(const Production& production);
    void setStart(const std::string& symbol) { start_ = symbol; }

    static bool looksLikeNonterminal(const std::string& token);

private:
    void classify(const std::string& symbol);

    std::vector<Production> productions_;
    std::set<std::string>   nonterminals_;
    std::set<std::string>   terminals_;
    std::string             start_;
};

std::string formatRhs(const std::vector<std::string>& rhs);
std::string ruleText(const Production& production);

std::string freshPrimedName(const std::string& nonterminal,
                            const std::function<bool(const std::string&)>& isTaken);

}

#endif
