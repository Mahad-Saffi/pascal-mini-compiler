#include "ast/ast.hpp"

#include <string>

namespace mp {

void NumLit::accept(Visitor& v)       { v.visit(*this); }
void VarRef::accept(Visitor& v)       { v.visit(*this); }
void CallExpr::accept(Visitor& v)     { v.visit(*this); }
void BinOp::accept(Visitor& v)        { v.visit(*this); }
void UnaryOp::accept(Visitor& v)      { v.visit(*this); }
void CompoundStmt::accept(Visitor& v) { v.visit(*this); }
void AssignStmt::accept(Visitor& v)   { v.visit(*this); }
void CallStmt::accept(Visitor& v)     { v.visit(*this); }
void IfStmt::accept(Visitor& v)       { v.visit(*this); }
void WhileStmt::accept(Visitor& v)    { v.visit(*this); }
void VarDecl::accept(Visitor& v)      { v.visit(*this); }
void Subprogram::accept(Visitor& v)   { v.visit(*this); }
void Program::accept(Visitor& v)      { v.visit(*this); }

std::string typeName(const TypeNode& type) {
    if (!type.isArray) return ptypeName(type.elem);
    return "array [" + std::to_string(type.lo) + ".." + std::to_string(type.hi) +
           "] of " + ptypeName(type.elem);
}

} // namespace mp
