#include "symtab/symbol_table.hpp"

#include "util/text.hpp"

#include <ostream>
#include <string>

namespace mp {
namespace {

std::string describe(const Entry& entry) {
    switch (entry.kind) {
        case Kind::Array: {
            const ArrayInfo& info = entry.array ? *entry.array : ArrayInfo{};
            return "array [" + std::to_string(info.lo) + ".." + std::to_string(info.hi) +
                   "] of " + ptypeName(info.elem);
        }
        case Kind::Func:
        case Kind::Proc: {
            std::string text = (entry.kind == Kind::Func ? "function(" : "procedure(");
            for (size_t i = 0; i < entry.params.size(); ++i) {
                if (i) text += ", ";
                text += ptypeName(entry.params[i]);
            }
            text += ")";
            if (entry.kind == Kind::Func) text += " : " + std::string(ptypeName(entry.returnType));
            return text;
        }
        case Kind::Builtin:
            return "builtin procedure (variadic)";
        case Kind::Var:
        case Kind::Param:
        default:
            return ptypeName(entry.type);
    }
}

} // namespace

const char* kindName(Kind kind) {
    switch (kind) {
        case Kind::Var:     return "var";
        case Kind::Param:   return "param";
        case Kind::Array:   return "array";
        case Kind::Func:    return "function";
        case Kind::Proc:    return "procedure";
        case Kind::Builtin: return "builtin";
    }
    return "?";
}

unsigned SymbolTable::djb2(const std::string& text) {
    unsigned hash = 5381;
    for (char character : text)
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(foldAsciiLower(character));
    return hash;
}

SymbolTable::SymbolTable() {
    beginScope();
    addBuiltins();
}

void SymbolTable::beginScope() {
    auto scope = std::make_unique<Scope>();
    scope->level  = (current_ == -1) ? 0 : scopes_[current_]->level + 1;
    scope->parent = current_;
    scopes_.push_back(std::move(scope));
    current_ = static_cast<int>(scopes_.size()) - 1;
}

void SymbolTable::endScope() {
    if (current_ != -1) current_ = scopes_[current_]->parent;
}

int SymbolTable::currentLevel() const {
    return current_ == -1 ? -1 : scopes_[current_]->level;
}

Entry* SymbolTable::findIn(const Scope& scope, const std::string& name) const {
    for (Entry* entry = scope.buckets[djb2(name) % kBuckets]; entry; entry = entry->next)
        if (equalsIgnoreAsciiCase(entry->name, name)) return entry;
    return nullptr;
}

Entry* SymbolTable::insert(const Entry& proto) {
    Scope& scope = *scopes_[current_];
    if (findIn(scope, proto.name)) return nullptr;

    auto owned = std::make_unique<Entry>(proto);
    Entry* entry = owned.get();
    entry->scopeLevel = scope.level;

    unsigned bucket = djb2(entry->name) % kBuckets;
    entry->next = scope.buckets[bucket];
    scope.buckets[bucket] = entry;
    scope.entries.push_back(std::move(owned));
    return entry;
}

Entry* SymbolTable::lookup(const std::string& name) const {
    for (int scopeIndex = current_; scopeIndex != -1; scopeIndex = scopes_[scopeIndex]->parent)
        if (Entry* entry = findIn(*scopes_[scopeIndex], name)) return entry;
    return nullptr;
}

Entry* SymbolTable::lookupCurrent(const std::string& name) const {
    return findIn(*scopes_[current_], name);
}

bool SymbolTable::remove(const std::string& name) {
    if (current_ == -1) return false;
    Scope& scope = *scopes_[current_];

    unsigned bucket   = djb2(name) % kBuckets;
    Entry*   previous = nullptr;
    Entry*   entry    = scope.buckets[bucket];
    while (entry && !equalsIgnoreAsciiCase(entry->name, name)) {
        previous = entry;
        entry    = entry->next;
    }
    if (!entry) return false;

    if (previous) previous->next = entry->next;
    else          scope.buckets[bucket] = entry->next;

    for (auto it = scope.entries.begin(); it != scope.entries.end(); ++it) {
        if (it->get() == entry) { scope.entries.erase(it); break; }
    }
    return true;
}

void SymbolTable::addBuiltins() {
    for (const char* name : {"read", "readln", "write", "writeln"}) {
        Entry entry;
        entry.name = name;
        entry.kind = Kind::Builtin;
        entry.type = PType::Void;
        insert(entry);
    }
}

void SymbolTable::printAll(std::ostream& out) const {
    out << "Symbol table (" << scopes_.size() << " scope"
        << (scopes_.size() == 1 ? "" : "s") << "):\n";

    for (const auto& scopePtr : scopes_) {
        const Scope& scope = *scopePtr;
        std::string pad(static_cast<size_t>(scope.level) * 2, ' ');
        out << pad << "scope " << scope.level
            << (scope.level == 0 ? " (global)" : "") << ":\n";

        if (scope.entries.empty()) {
            out << pad << "  (empty)\n";
            continue;
        }
        for (const auto& entryPtr : scope.entries) {
            const Entry& entry = *entryPtr;
            out << pad << "  " << entry.name
                << "  [" << kindName(entry.kind) << "] " << describe(entry);
            if (entry.line > 0) out << "  (line " << entry.line << ")";
            out << "\n";
        }
    }
}

} // namespace mp
