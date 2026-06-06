#include "ast/ast.hpp"

#include "util/text.hpp"

#include <ostream>
#include <string>

namespace mp {
namespace {

class AstPrinter : public Visitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void visit(Program& node) override {
        emit("Program \"" + node.name + "\"" +
             (node.params.empty() ? "" : " (" + joinStrings(node.params, ", ") + ")"));
        Indent indent(*this);
        if (!node.decls.empty()) {
            emit("declarations:");
            Indent listIndent(*this);
            for (auto& decl : node.decls) child(decl.get());
        }
        if (!node.subprograms.empty()) {
            emit("subprograms:");
            Indent listIndent(*this);
            for (auto& sub : node.subprograms) child(sub.get());
        }
        labelled("body:", node.body.get());
    }

    void visit(VarDecl& node) override {
        emit("VarDecl " + joinStrings(node.names, ", ") + " : " + typeName(node.type));
    }

    void visit(Subprogram& node) override {
        emit((node.isFunction ? "Function \"" : "Procedure \"") + node.name + "\"");
        Indent indent(*this);
        for (const Param& param : node.params)
            emit("param " + joinStrings(param.names, ", ") + " : " + typeName(param.type));
        if (node.isFunction) emit(std::string("returns: ") + ptypeName(node.returnType));
        if (!node.decls.empty()) {
            emit("declarations:");
            Indent listIndent(*this);
            for (auto& decl : node.decls) child(decl.get());
        }
        labelled("body:", node.body.get());
    }

    void visit(CompoundStmt& node) override {
        emit("Compound (" + std::to_string(node.stmts.size()) + " stmt" +
             (node.stmts.size() == 1 ? "" : "s") + ")");
        Indent indent(*this);
        for (auto& stmt : node.stmts) child(stmt.get());
    }

    void visit(AssignStmt& node) override {
        emit("Assign");
        Indent indent(*this);
        labelled("target:", node.target.get());
        labelled("value:", node.value.get());
    }

    void visit(CallStmt& node) override {
        emit("Call \"" + node.name + "\"" + (node.args.empty() ? " (no args)" : ""));
        Indent indent(*this);
        for (auto& arg : node.args) child(arg.get());
    }

    void visit(IfStmt& node) override {
        emit("If");
        Indent indent(*this);
        labelled("cond:", node.cond.get());
        labelled("then:", node.thenBranch.get());
        labelled("else:", node.elseBranch.get());
    }

    void visit(WhileStmt& node) override {
        emit("While");
        Indent indent(*this);
        labelled("cond:", node.cond.get());
        labelled("body:", node.body.get());
    }

    void visit(BinOp& node) override {
        emit("BinOp \"" + node.op + "\"");
        Indent indent(*this);
        child(node.lhs.get());
        child(node.rhs.get());
    }

    void visit(UnaryOp& node) override {
        emit("Unary \"" + node.op + "\"");
        Indent indent(*this);
        child(node.operand.get());
    }

    void visit(NumLit& node) override {
        emit(std::string("Num ") + node.lexeme + (node.isReal ? " (real)" : " (int)"));
    }

    void visit(VarRef& node) override {
        if (node.index) {
            emit("ArrayRef \"" + node.name + "\"");
            Indent indent(*this);
            labelled("index:", node.index.get());
        } else {
            emit("Var \"" + node.name + "\"");
        }
    }

    void visit(CallExpr& node) override {
        emit("FuncCall \"" + node.name + "\"");
        Indent indent(*this);
        for (auto& arg : node.args) child(arg.get());
    }

private:
    struct Indent {
        AstPrinter& printer;
        explicit Indent(AstPrinter& printer) : printer(printer) { ++printer.depth_; }
        ~Indent() { --printer.depth_; }
    };

    void emit(const std::string& text) { out_ << std::string(depth_ * 2, ' ') << text << "\n"; }

    void child(Node* node) {
        if (node) node->accept(*this);
        else      emit("<error>");
    }

    void labelled(const char* label, Node* node) {
        emit(label);
        Indent indent(*this);
        child(node);
    }

    std::ostream& out_;
    int           depth_ = 0;
};

} // namespace

void printAst(Program& program, std::ostream& out) {
    AstPrinter printer(out);
    program.accept(printer);
}

} // namespace mp
