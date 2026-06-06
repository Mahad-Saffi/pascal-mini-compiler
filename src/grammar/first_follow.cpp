#include "grammar/first_follow.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>

namespace mp {

const std::string FirstFollow::kEps = "";
const std::string FirstFollow::kEnd = "$";

namespace {

std::string showSet(const std::set<std::string>& elements) {
    std::string text = "{ ";
    bool isFirst = true;
    for (const std::string& element : elements) {
        if (!isFirst) text += ", ";
        text += (element.empty() ? "\xCE\xB5" : element);
        isFirst = false;
    }
    text += " }";
    return text;
}

}

FirstFollow::FirstFollow(const Grammar& grammar)
    : grammar_(grammar), nonterminalOrder_(grammar.nonterminalsInOrder()) {
    computeFirst();
    computeFollow();
}

const std::set<std::string>& FirstFollow::first(const std::string& symbol) const {
    auto found = firstSets_.find(symbol);
    if (found != firstSets_.end()) return found->second;
    static const std::set<std::string> empty;
    return empty;
}

const std::set<std::string>& FirstFollow::follow(const std::string& nonterminal) const {
    auto found = followSets_.find(nonterminal);
    if (found != followSets_.end()) return found->second;
    static const std::set<std::string> empty;
    return empty;
}

std::set<std::string> FirstFollow::firstOfSeq(const std::vector<std::string>& sequence) const {
    std::set<std::string> result;
    bool allNullable = true;
    for (const std::string& symbol : sequence) {
        const std::set<std::string>& symbolFirst = first(symbol);
        for (const std::string& element : symbolFirst)
            if (element != kEps) result.insert(element);
        if (symbolFirst.count(kEps) == 0) { allNullable = false; break; }
    }
    if (allNullable) result.insert(kEps);
    return result;
}

void FirstFollow::computeFirst() {
    for (const std::string& terminal : grammar_.terminals()) firstSets_[terminal].insert(terminal);
    for (const std::string& nonterminal : grammar_.nonterminals()) firstSets_[nonterminal];

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& production : grammar_.productions()) {
            std::set<std::string>& firstSet = firstSets_[production.lhs];
            size_t before = firstSet.size();
            if (production.rhs.empty()) {
                firstSet.insert(kEps);
            } else {
                bool allNullable = true;
                for (const std::string& symbol : production.rhs) {
                    const std::set<std::string>& symbolFirst = firstSets_[symbol];
                    for (const std::string& element : symbolFirst)
                        if (element != kEps) firstSet.insert(element);
                    if (symbolFirst.count(kEps) == 0) { allNullable = false; break; }
                }
                if (allNullable) firstSet.insert(kEps);
            }
            if (firstSet.size() != before) changed = true;
        }
    }
}

void FirstFollow::computeFollow() {
    for (const std::string& nonterminal : grammar_.nonterminals()) followSets_[nonterminal];
    followSets_[grammar_.start()].insert(kEnd);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& production : grammar_.productions()) {
            for (size_t i = 0; i < production.rhs.size(); ++i) {
                const std::string& symbol = production.rhs[i];
                if (!grammar_.isNonterminal(symbol)) continue;

                std::set<std::string>& followSymbol = followSets_[symbol];
                size_t before = followSymbol.size();

                std::vector<std::string> suffix(production.rhs.begin() + i + 1, production.rhs.end());
                std::set<std::string> suffixFirst = firstOfSeq(suffix);
                for (const std::string& element : suffixFirst)
                    if (element != kEps) followSymbol.insert(element);
                if (suffixFirst.count(kEps) > 0) {
                    std::set<std::string> followLhs = followSets_[production.lhs];
                    followSymbol.insert(followLhs.begin(), followLhs.end());
                }
                if (followSymbol.size() != before) changed = true;
            }
        }
    }
}

void FirstFollow::print(std::ostream& out) const {
    size_t width = 0;
    for (const std::string& nonterminal : nonterminalOrder_) width = std::max(width, nonterminal.size());
    for (const std::string& nonterminal : nonterminalOrder_) {
        out << std::left << std::setw(static_cast<int>(width)) << nonterminal
            << "  FIRST  = " << showSet(first(nonterminal)) << "\n"
            << std::string(width, ' ')
            << "  FOLLOW = " << showSet(follow(nonterminal)) << "\n";
    }
}

}
