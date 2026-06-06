#include "grammar/slr_table.hpp"

#include <algorithm>
#include <ostream>
#include <string>

namespace mp {

SLRTable::SLRTable(const Grammar& original) {
    augment(original);
    FirstFollow firstFollow(augmented_);
    buildCanonicalCollection();
    buildActionsAndGoto(firstFollow);
}

void SLRTable::augment(const Grammar& original) {
    std::string augmentedSymbol = freshPrimedName(original.start(), [&](const std::string& candidate) {
        return original.nonterminals().count(candidate) > 0;
    });
    augmented_.addProduction(Production{augmentedSymbol, {original.start()}});
    for (const Production& production : original.productions()) augmented_.addProduction(production);
    augmented_.setStart(augmentedSymbol);
    augmentedStart_ = 0;
}

void SLRTable::buildCanonicalCollection() {
    ItemSet start = closure(ItemSet{Item{augmentedStart_, 0}});
    states_.push_back(start);
    std::map<ItemSet, int> stateIndex;
    stateIndex[start] = 0;
    for (int i = 0; i < static_cast<int>(states_.size()); ++i) {
        std::vector<std::string> symbols;
        std::set<std::string> seen;
        for (const Item& item : states_[i]) {
            const Production& production = augmented_.productions()[item.prod];
            if (item.dot < static_cast<int>(production.rhs.size())) {
                const std::string& symbol = production.rhs[item.dot];
                if (seen.insert(symbol).second) symbols.push_back(symbol);
            }
        }
        for (const std::string& symbol : symbols) {
            ItemSet next = goTo(states_[i], symbol);
            if (next.empty()) continue;
            int target;
            auto found = stateIndex.find(next);
            if (found == stateIndex.end()) {
                target = static_cast<int>(states_.size());
                states_.push_back(next);
                stateIndex[next] = target;
            } else {
                target = found->second;
            }
            transitions_[i][symbol] = target;
        }
    }
}

void SLRTable::buildActionsAndGoto(const FirstFollow& firstFollow) {
    std::map<int, std::map<std::string, std::vector<Action>>> proposed;
    auto propose = [&](int state, const std::string& terminal, Action action) {
        std::vector<Action>& actions = proposed[state][terminal];
        for (const Action& existing : actions)
            if (existing.kind == action.kind && existing.value == action.value) return;
        actions.push_back(action);
    };

    for (int i = 0; i < static_cast<int>(states_.size()); ++i) {
        for (const Item& item : states_[i]) {
            const Production& production = augmented_.productions()[item.prod];
            if (item.dot < static_cast<int>(production.rhs.size())) {
                const std::string& symbol = production.rhs[item.dot];
                if (!augmented_.isNonterminal(symbol))
                    propose(i, symbol, Action{ActionKind::Shift, transitions_[i][symbol]});
            } else if (item.prod == augmentedStart_) {
                propose(i, FirstFollow::kEnd, Action{ActionKind::Accept, 0});
            } else {
                for (const std::string& terminal : firstFollow.follow(production.lhs))
                    propose(i, terminal, Action{ActionKind::Reduce, item.prod});
            }
        }
    }

    for (const auto& stateTransitions : transitions_)
        for (const auto& symbolTarget : stateTransitions.second)
            if (augmented_.isNonterminal(symbolTarget.first))
                gotoTable_[stateTransitions.first][symbolTarget.first] = symbolTarget.second;

    for (auto& stateProposals : proposed) {
        int state = stateProposals.first;
        for (auto& terminalActions : stateProposals.second) {
            const std::string& terminal = terminalActions.first;
            std::vector<Action>& actions = terminalActions.second;
            if (actions.size() == 1) actionTable_[state][terminal] = actions[0];
            else                     actionTable_[state][terminal] = resolveConflict(state, terminal, actions);
        }
    }
}

SLRTable::Action SLRTable::resolveConflict(int state, const std::string& terminal,
                                           const std::vector<Action>& actions) {
    bool haveShift = false, haveAccept = false;
    int bestReduce = -1;
    Action chosen;
    for (const Action& action : actions) {
        if (action.kind == ActionKind::Accept) { haveAccept = true; chosen = action; }
        else if (action.kind == ActionKind::Shift) { haveShift = true; if (!haveAccept) chosen = action; }
        else if (action.kind == ActionKind::Reduce)
            if (bestReduce < 0 || action.value < bestReduce) bestReduce = action.value;
    }
    if (!haveAccept && !haveShift && bestReduce >= 0)
        chosen = Action{ActionKind::Reduce, bestReduce};

    std::string kind = haveShift && bestReduce >= 0 ? "shift/reduce"
                     : (!haveShift && !haveAccept ? "reduce/reduce" : "conflict");
    std::string detail;
    for (size_t k = 0; k < actions.size(); ++k) {
        if (k) detail += " vs ";
        detail += actionText(actions[k]);
    }
    conflicts_.push_back(Conflict{state, terminal, kind, detail});
    return chosen;
}

SLRTable::ItemSet SLRTable::closure(ItemSet items) const {
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet pending;
        for (const Item& item : items) {
            const Production& production = augmented_.productions()[item.prod];
            if (item.dot >= static_cast<int>(production.rhs.size())) continue;
            const std::string& symbol = production.rhs[item.dot];
            if (!augmented_.isNonterminal(symbol)) continue;
            for (int productionIndex : augmented_.productionsFor(symbol)) {
                Item candidate{productionIndex, 0};
                if (!items.count(candidate)) pending.insert(candidate);
            }
        }
        for (const Item& candidate : pending)
            if (items.insert(candidate).second) changed = true;
    }
    return items;
}

SLRTable::ItemSet SLRTable::goTo(const ItemSet& items, const std::string& symbol) const {
    ItemSet result;
    for (const Item& item : items) {
        const Production& production = augmented_.productions()[item.prod];
        if (item.dot < static_cast<int>(production.rhs.size()) && production.rhs[item.dot] == symbol)
            result.insert(Item{item.prod, item.dot + 1});
    }
    return closure(result);
}

SLRTable::Action SLRTable::action(int state, const std::string& terminal) const {
    auto stateRow = actionTable_.find(state);
    if (stateRow == actionTable_.end()) return Action{};
    auto entry = stateRow->second.find(terminal);
    if (entry == stateRow->second.end()) return Action{};
    return entry->second;
}

int SLRTable::gotoState(int state, const std::string& nonterminal) const {
    auto stateRow = gotoTable_.find(state);
    if (stateRow == gotoTable_.end()) return -1;
    auto entry = stateRow->second.find(nonterminal);
    if (entry == stateRow->second.end()) return -1;
    return entry->second;
}

std::string SLRTable::itemString(const Item& item) const {
    const Production& production = augmented_.productions()[item.prod];
    std::string text = production.lhs + " ->";
    for (int k = 0; k < static_cast<int>(production.rhs.size()); ++k) {
        if (k == item.dot) text += " .";
        text += " " + production.rhs[k];
    }
    if (item.dot == static_cast<int>(production.rhs.size())) text += " .";
    return text;
}

std::string SLRTable::actionText(const Action& action) const {
    switch (action.kind) {
        case ActionKind::Shift:  return "shift " + std::to_string(action.value);
        case ActionKind::Reduce: return "reduce " + ruleText(augmented_.productions()[action.value]);
        case ActionKind::Accept: return "accept";
        default:                 return "error";
    }
}

void SLRTable::printItemSets(std::ostream& out) const {
    out << "SLR(1) Canonical LR(0) Item Sets (augmented Original Grammar)\n"
        << "  " << states_.size() << " states; augmented start "
        << augmented_.start() << " -> " << augmented_.productions()[augmentedStart_].rhs[0] << "\n\n";
    for (int i = 0; i < static_cast<int>(states_.size()); ++i) {
        out << "State " << i << ":\n";
        for (const Item& item : states_[i])
            out << "    " << itemString(item) << "\n";
        auto stateTransitions = transitions_.find(i);
        if (stateTransitions != transitions_.end())
            for (const auto& symbolTarget : stateTransitions->second)
                out << "      on " << symbolTarget.first << " -> State " << symbolTarget.second << "\n";
        out << "\n";
    }
}

void SLRTable::printTables(std::ostream& out) const {
    out << "SLR(1) ACTION / GOTO tables (" << states_.size() << " states)\n"
        << "  ACTION holds shift/reduce/accept on terminals; GOTO holds state on nonterminals.\n\n";
    for (int i = 0; i < static_cast<int>(states_.size()); ++i) {
        out << "State " << i << ":\n";
        auto stateRow = actionTable_.find(i);
        if (stateRow != actionTable_.end())
            for (const auto& entry : stateRow->second)
                out << "    ACTION[" << entry.first << "] = " << actionText(entry.second) << "\n";
        auto gotoRow = gotoTable_.find(i);
        if (gotoRow != gotoTable_.end())
            for (const auto& gotoEntry : gotoRow->second)
                out << "    GOTO[" << gotoEntry.first << "] = State " << gotoEntry.second << "\n";
        out << "\n";
    }
}

void SLRTable::printConflicts(std::ostream& out) const {
    if (conflicts_.empty()) {
        out << "No SLR(1) conflicts: the Original Grammar is SLR(1).\n";
        return;
    }
    out << conflicts_.size()
        << " SLR(1) conflict(s) (resolved: shift over reduce, lower production over higher):\n";
    for (const Conflict& conflict : conflicts_)
        out << "  state " << conflict.state << ", on '" << conflict.symbol << "': "
            << conflict.kind << " -> " << conflict.detail << "\n";
}

}
