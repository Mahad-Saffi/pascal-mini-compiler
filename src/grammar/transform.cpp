#include "grammar/transform.hpp"

#include "grammar/grammar.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace mp {
namespace {

struct Builder {
    std::vector<std::string> order;
    std::map<std::string, std::vector<std::vector<std::string>>> alternatives;

    void addNonterminal(const std::string& nonterminal) {
        if (!alternatives.count(nonterminal)) {
            order.push_back(nonterminal);
            alternatives[nonterminal] = {};
        }
    }
};

Builder toBuilder(const Grammar& grammar) {
    Builder builder;
    for (const Production& production : grammar.productions()) {
        builder.addNonterminal(production.lhs);
        builder.alternatives[production.lhs].push_back(production.rhs);
    }
    return builder;
}

Grammar fromBuilder(const Builder& builder, const std::string& start) {
    Grammar grammar;
    for (const std::string& nonterminal : builder.order)
        for (const auto& rhs : builder.alternatives.at(nonterminal))
            grammar.addProduction(Production{nonterminal, rhs});
    grammar.setStart(start);
    return grammar;
}

std::string makeFreshNonterminal(const std::string& nonterminal, std::set<std::string>& used) {
    std::string fresh = freshPrimedName(nonterminal, [&](const std::string& candidate) {
        return used.count(candidate) > 0;
    });
    used.insert(fresh);
    return fresh;
}

}

Grammar removeLeftRecursion(const Grammar& grammar) {
    Builder source = toBuilder(grammar);
    std::set<std::string> used(grammar.nonterminals().begin(), grammar.nonterminals().end());
    Builder result;

    for (const std::string& nonterminal : source.order) {
        std::vector<std::vector<std::string>> recursiveTails, nonRecursive;
        for (const auto& rhs : source.alternatives.at(nonterminal)) {
            if (!rhs.empty() && rhs[0] == nonterminal)
                recursiveTails.emplace_back(rhs.begin() + 1, rhs.end());
            else
                nonRecursive.push_back(rhs);
        }

        if (recursiveTails.empty()) {
            result.addNonterminal(nonterminal);
            result.alternatives[nonterminal] = source.alternatives.at(nonterminal);
            continue;
        }

        std::string primed = makeFreshNonterminal(nonterminal, used);

        std::vector<std::vector<std::string>> baseProductions;
        if (nonRecursive.empty()) {
            baseProductions.push_back({primed});
        } else {
            for (auto body : nonRecursive) {
                body.push_back(primed);
                baseProductions.push_back(body);
            }
        }

        std::vector<std::vector<std::string>> primedProductions;
        for (auto tail : recursiveTails) {
            tail.push_back(primed);
            primedProductions.push_back(tail);
        }
        primedProductions.push_back({});

        result.addNonterminal(nonterminal);
        result.alternatives[nonterminal] = baseProductions;
        result.addNonterminal(primed);
        result.alternatives[primed] = primedProductions;
    }

    return fromBuilder(result, grammar.start());
}

Grammar leftFactor(const Grammar& grammar) {
    Builder builder = toBuilder(grammar);
    std::set<std::string> used(builder.order.begin(), builder.order.end());

    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::string> snapshot = builder.order;

        for (const std::string& nonterminal : snapshot) {
            auto& alternatives = builder.alternatives.at(nonterminal);
            if (alternatives.size() < 2) continue;

            std::map<std::string, std::vector<int>> byFirstSymbol;
            for (int i = 0; i < static_cast<int>(alternatives.size()); ++i)
                if (!alternatives[i].empty()) byFirstSymbol[alternatives[i][0]].push_back(i);

            int sharedIndex = -1;
            std::string sharedSymbol;
            for (int i = 0; i < static_cast<int>(alternatives.size()) && sharedIndex < 0; ++i) {
                if (alternatives[i].empty()) continue;
                const std::string& symbol = alternatives[i][0];
                if (byFirstSymbol[symbol].size() >= 2) { sharedIndex = i; sharedSymbol = symbol; }
            }
            if (sharedIndex < 0) continue;

            const std::vector<int>& group = byFirstSymbol[sharedSymbol];

            int prefixLength = 0;
            for (;;) {
                bool matched = true;
                std::string symbol;
                for (size_t j = 0; j < group.size(); ++j) {
                    const auto& alternative = alternatives[group[j]];
                    if (static_cast<int>(alternative.size()) <= prefixLength) { matched = false; break; }
                    if (j == 0) symbol = alternative[prefixLength];
                    else if (alternative[prefixLength] != symbol) { matched = false; break; }
                }
                if (!matched) break;
                ++prefixLength;
            }

            std::vector<std::string> prefix(alternatives[group[0]].begin(),
                                            alternatives[group[0]].begin() + prefixLength);
            std::string primed = makeFreshNonterminal(nonterminal, used);

            std::vector<std::vector<std::string>> suffixes;
            for (int idx : group)
                suffixes.emplace_back(alternatives[idx].begin() + prefixLength, alternatives[idx].end());

            std::set<int> groupIndices(group.begin(), group.end());
            std::vector<std::vector<std::string>> rebuilt;
            bool inserted = false;
            for (int i = 0; i < static_cast<int>(alternatives.size()); ++i) {
                if (!groupIndices.count(i)) { rebuilt.push_back(alternatives[i]); continue; }
                if (inserted) continue;
                std::vector<std::string> factored = prefix;
                factored.push_back(primed);
                rebuilt.push_back(factored);
                inserted = true;
            }

            alternatives = rebuilt;
            builder.alternatives[primed] = suffixes;
            auto position = std::find(builder.order.begin(), builder.order.end(), nonterminal);
            builder.order.insert(position + 1, primed);

            changed = true;
            break;
        }
    }

    return fromBuilder(builder, grammar.start());
}

Grammar toTransformed(const Grammar& grammar) {
    return leftFactor(removeLeftRecursion(grammar));
}

}
