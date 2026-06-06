#ifndef MINIPASCAL_SEMANTIC_SEMANTIC_HPP
#define MINIPASCAL_SEMANTIC_SEMANTIC_HPP

#include "ast/ast.hpp"
#include "diagnostics.hpp"
#include "symtab/symbol_table.hpp"

#include <string>
#include <vector>

namespace mp {

class SemanticAnalyzer : public Visitor {
public:
    SemanticAnalyzer(SymbolTable& symbols, ErrorReporter& reporter);

    void analyze(Program& program);

    void visit(Program&)      override;
    void visit(VarDecl&)      override;
    void visit(Subprogram&)   override;
    void visit(CompoundStmt&) override;
    void visit(AssignStmt&)   override;
    void visit(CallStmt&)     override;
    void visit(IfStmt&)       override;
    void visit(WhileStmt&)    override;
    void visit(BinOp&)        override;
    void visit(UnaryOp&)      override;
    void visit(NumLit&)       override;
    void visit(VarRef&)       override;
    void visit(CallExpr&)     override;

private:
    SymbolTable&   symbols_;
    ErrorReporter& reporter_;

    std::string currentFunction_;
    PType       currentFunctionReturn_ = PType::Void;

    void  error(const Node& node, const std::string& msg);
    PType typeOf(Expr* expression);

    void  declareSubprogram(Subprogram& subprogram);
    void  checkCall(const Entry& callee, std::vector<ExprPtr>& args, const Node& site);
    void  checkConstIndexBounds(const Entry& arr, const VarRef& target);

    bool        assignable(PType from, PType to) const;
    static bool numeric(PType type);
};

} // namespace mp

#endif // MINIPASCAL_SEMANTIC_SEMANTIC_HPP
