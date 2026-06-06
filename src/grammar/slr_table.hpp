#ifndef MINIPASCAL_GRAMMAR_SLR_TABLE_HPP
#define MINIPASCAL_GRAMMAR_SLR_TABLE_HPP

#include "grammar/first_follow.hpp"
#include "grammar/grammar.hpp"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mp {

class SLRTable {
public:
    explicit SLRTable(const Grammar& original);

    enum class ActionKind { Error, Shift, Reduce, Accept };
    struct Action {
        ActionKind kind = ActionKind::Error;
        int        value = 0;
    };

    Action action(int state, const std::string& terminal) const;
    int    gotoState(int state, const std::string& nonterminal) const;
    int    numStates() const { return static_cast<int>(states_.size()); }

    const Production& production(int index) const { return augmented_.productions()[index]; }

    struct Conflict {
        int         state = 0;
        std::string symbol;
        std::string kind;
        std::string detail;
    };
    bool hasConflicts() const { return !conflicts_.empty(); }
    const std::vector<Conflict>& conflicts() const { return conflicts_; }

    void printItemSets(std::ostream& out) const;
    void printTables(std::ostream& out) const;
    void printConflicts(std::ostream& out) const;

private:
    struct Item {
        int prod;
        int dot;
        bool operator<(const Item& other) const {
            return prod != other.prod ? prod < other.prod : dot < other.dot;
        }
        bool operator==(const Item& other) const { return prod == other.prod && dot == other.dot; }
    };
    using ItemSet = std::set<Item>;

    void   augment(const Grammar& original);
    void   buildCanonicalCollection();
    void   buildActionsAndGoto(const FirstFollow& firstFollow);
    Action resolveConflict(int state, const std::string& terminal, const std::vector<Action>& actions);

    ItemSet     closure(ItemSet items) const;
    ItemSet     goTo(const ItemSet& items, const std::string& symbol) const;
    std::string itemString(const Item& item) const;
    std::string actionText(const Action& action) const;

    Grammar                                      augmented_;
    int                                          augmentedStart_ = 0;
    std::vector<ItemSet>                         states_;
    std::map<int, std::map<std::string, int>>    transitions_;
    std::map<int, std::map<std::string, Action>> actionTable_;
    std::map<int, std::map<std::string, int>>    gotoTable_;
    std::vector<Conflict>                        conflicts_;
};

}

#endif
