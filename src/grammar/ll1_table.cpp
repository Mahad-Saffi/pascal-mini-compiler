#include "grammar/ll1_table.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <set>

namespace mp {

LL1Table::LL1Table(const Grammar& grammar, const FirstFollow& firstFollow)
    : grammar_(grammar), nonterminalOrder_(grammar.nonterminalsInOrder()) {
    const std::vector<Production>& productions = grammar_.productions();
    for (int i = 0; i < static_cast<int>(productions.size()); ++i) {
        const Production& production = productions[i];
        std::set<std::string> firstOfRhs = firstFollow.firstOfSeq(production.rhs);
        bool nullable = firstOfRhs.count(FirstFollow::kEps) > 0;
        for (const std::string& terminal : firstOfRhs)
            if (terminal != FirstFollow::kEps) table_[production.lhs][terminal].push_back(i);
        if (nullable)
            for (const std::string& terminal : firstFollow.follow(production.lhs))
                table_[production.lhs][terminal].push_back(i);
    }

    for (const std::string& nonterminal : nonterminalOrder_)
        for (const auto& cellPair : table_[nonterminal])
            if (cellPair.second.size() >= 2)
                conflicts_.push_back(Conflict{nonterminal, cellPair.first, cellPair.second});
}

int LL1Table::cell(const std::string& nonterminal, const std::string& term) const {
    auto row = table_.find(nonterminal);
    if (row == table_.end()) return -1;
    auto found = row->second.find(term);
    if (found == row->second.end() || found->second.empty()) return -1;
    return found->second.front();
}

void LL1Table::print(std::ostream& out) const {
    out << "LL(1) Predictive Parsing Table (Transformed Grammar)\n"
        << "  each row is a nonterminal; \"on a => alpha\" means M[A, a] = A -> alpha.\n"
        << "  a cell with more than one production is a CONFLICT (not LL(1)).\n\n";

    const std::vector<Production>& productions = grammar_.productions();
    for (const std::string& nonterminal : nonterminalOrder_) {
        const auto& row = table_.at(nonterminal);
        out << nonterminal << "\n";
        size_t width = 0;
        for (const auto& entry : row) width = std::max(width, entry.first.size());
        for (const auto& entry : row) {
            bool conflict = entry.second.size() >= 2;
            for (int idx : entry.second) {
                out << "    on " << std::left << std::setw(static_cast<int>(width)) << entry.first
                    << "  =>  " << formatRhs(productions[idx].rhs);
                if (conflict) out << "   [CONFLICT]";
                out << "\n";
            }
        }
    }
}

void LL1Table::printConflicts(std::ostream& out) const {
    if (conflicts_.empty()) {
        out << "No LL(1) conflicts: the Transformed Grammar is LL(1).\n";
        return;
    }
    const std::vector<Production>& productions = grammar_.productions();
    out << conflicts_.size() << " LL(1) conflict(s) (grammar is not LL(1)):\n";
    for (const Conflict& conflict : conflicts_) {
        out << "  M[" << conflict.nonterminal << ", " << conflict.lookahead << "] has "
            << conflict.productions.size() << " productions:\n";
        for (int idx : conflict.productions)
            out << "      " << conflict.nonterminal << " -> " << formatRhs(productions[idx].rhs) << "\n";
    }
}

}
