#ifndef MINIPASCAL_SYMTAB_SYMBOL_TABLE_HPP
#define MINIPASCAL_SYMTAB_SYMBOL_TABLE_HPP

#include "types.hpp"

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mp {

enum class Kind { Var, Param, Array, Func, Proc, Builtin };

const char* kindName(Kind kind);

struct ArrayInfo {
    PType elem = PType::Integer;
    int   lo = 0, hi = 0;
};

struct Entry {
    std::string              name;
    Kind                     kind = Kind::Var;
    PType                    type = PType::Error;
    int                      scopeLevel = 0;
    int                      line = 0;
    std::optional<ArrayInfo> array;
    std::vector<PType>       params;
    PType                    returnType = PType::Void;
    Entry*                   next = nullptr;
};

class SymbolTable {
public:
    SymbolTable();

    void  beginScope();
    void  endScope();
    int   currentLevel() const;

    Entry* insert(const Entry& proto);
    Entry* lookup(const std::string& name) const;
    Entry* lookupCurrent(const std::string& name) const;

    bool   remove(const std::string& name);

    void  printAll(std::ostream& out) const;

private:
    static constexpr int kBuckets = 211;
    static unsigned djb2(const std::string& text);

    struct Scope {
        int                                 level = 0;
        int                                 parent = -1;
        std::vector<std::unique_ptr<Entry>> entries;
        std::vector<Entry*>                 buckets;
        Scope() : buckets(kBuckets, nullptr) {}
    };

    Entry* findIn(const Scope& scope, const std::string& name) const;
    void   addBuiltins();

    std::vector<std::unique_ptr<Scope>> scopes_;
    int                                 current_ = -1;
};

} // namespace mp

#endif // MINIPASCAL_SYMTAB_SYMBOL_TABLE_HPP
