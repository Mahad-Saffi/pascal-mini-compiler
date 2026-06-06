#ifndef MINIPASCAL_AST_AST_HPP
#define MINIPASCAL_AST_AST_HPP

#include "types.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace mp {

struct Visitor;

struct Node {
    int line = 0;
    int col  = 0;
    virtual ~Node() = default;
    virtual void accept(Visitor&) = 0;
};

struct Expr : Node {
    PType type = PType::Error;
};
using ExprPtr = std::unique_ptr<Expr>;

struct NumLit : Expr {
    std::string lexeme;
    double      value = 0.0;
    bool        isReal = false;
    void accept(Visitor&) override;
};

struct VarRef : Expr {
    std::string name;
    ExprPtr     index;
    void accept(Visitor&) override;
};

struct CallExpr : Expr {
    std::string          name;
    std::vector<ExprPtr> args;
    void accept(Visitor&) override;
};

struct BinOp : Expr {
    std::string op;
    ExprPtr     lhs, rhs;
    void accept(Visitor&) override;
};

struct UnaryOp : Expr {
    std::string op;
    ExprPtr     operand;
    void accept(Visitor&) override;
};

struct Stmt : Node {};
using StmtPtr = std::unique_ptr<Stmt>;

struct CompoundStmt : Stmt {
    std::vector<StmtPtr> stmts;
    void accept(Visitor&) override;
};

struct AssignStmt : Stmt {
    std::unique_ptr<VarRef> target;
    ExprPtr                 value;
    void accept(Visitor&) override;
};

struct CallStmt : Stmt {
    std::string          name;
    std::vector<ExprPtr> args;
    void accept(Visitor&) override;
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr thenBranch, elseBranch;
    void accept(Visitor&) override;
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtPtr body;
    void accept(Visitor&) override;
};

struct TypeNode {
    bool  isArray = false;
    PType elem = PType::Integer;
    int   lo = 0, hi = 0;
};

struct VarDecl : Node {
    std::vector<std::string> names;
    TypeNode                 type;
    void accept(Visitor&) override;
};

struct Param {
    std::vector<std::string> names;
    TypeNode                 type;
};

struct Subprogram : Node {
    bool                                  isFunction = false;
    std::string                           name;
    std::vector<Param>                    params;
    PType                                 returnType = PType::Void;
    std::vector<std::unique_ptr<VarDecl>> decls;
    std::unique_ptr<CompoundStmt>         body;
    void accept(Visitor&) override;
};

struct Program : Node {
    std::string                              name;
    std::vector<std::string>                 params;
    std::vector<std::unique_ptr<VarDecl>>    decls;
    std::vector<std::unique_ptr<Subprogram>> subprograms;
    std::unique_ptr<CompoundStmt>            body;
    void accept(Visitor&) override;
};

struct Visitor {
    virtual ~Visitor() = default;
    virtual void visit(Program&)      = 0;
    virtual void visit(VarDecl&)      = 0;
    virtual void visit(Subprogram&)   = 0;
    virtual void visit(CompoundStmt&) = 0;
    virtual void visit(AssignStmt&)   = 0;
    virtual void visit(CallStmt&)     = 0;
    virtual void visit(IfStmt&)       = 0;
    virtual void visit(WhileStmt&)    = 0;
    virtual void visit(BinOp&)        = 0;
    virtual void visit(UnaryOp&)      = 0;
    virtual void visit(NumLit&)       = 0;
    virtual void visit(VarRef&)       = 0;
    virtual void visit(CallExpr&)     = 0;
};

std::string typeName(const TypeNode& type);

void printAst(Program& program, std::ostream& out);
void printAstDot(Program& program, std::ostream& out);

} // namespace mp

#endif // MINIPASCAL_AST_AST_HPP
