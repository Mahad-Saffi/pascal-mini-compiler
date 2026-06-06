#include "ast/ast.hpp"

#include "util/text.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace mp {
namespace {

std::string dotEscape(const std::string& text) {
    std::string out;
    for (char character : text) {
        if (character == '"' || character == '\\') { out += '\\'; out += character; }
        else if (character == '\n')                { out += "\\n"; }
        else                                       { out += character; }
    }
    return out;
}

std::string typeSuffix(PType type) {
    if (type == PType::Integer) return "\n: integer";
    if (type == PType::Real)    return "\n: real";
    return "";
}

class AstDot : public Visitor {
public:
    explicit AstDot(std::ostream& out) : out_(out) {}

    void visit(Program& node) override {
        int id = makeNode("Program: " + node.name +
                          (node.params.empty() ? std::string()
                                               : " (" + joinStrings(node.params, ", ") + ")"),
                          "box3d");
        for (auto& decl : node.decls)      edge(id, emit(decl.get()), "decl");
        for (auto& sub : node.subprograms) edge(id, emit(sub.get()), "subprogram");
        edge(id, emit(node.body.get()), "body");
        last_ = id;
    }

    void visit(VarDecl& node) override {
        last_ = makeNode("VarDecl\n" + joinStrings(node.names, ", ") + " : " + typeName(node.type), "box");
    }

    void visit(Subprogram& node) override {
        int id = makeNode((node.isFunction ? "Function: " : "Procedure: ") + node.name, "box3d");
        for (const Param& param : node.params)
            edge(id, makeNode("param\n" + joinStrings(param.names, ", ") + " : " + typeName(param.type), "box"),
                 "param");
        if (node.isFunction)
            edge(id, makeNode(std::string("returns\n") + ptypeName(node.returnType), "box"), "");
        for (auto& decl : node.decls) edge(id, emit(decl.get()), "decl");
        edge(id, emit(node.body.get()), "body");
        last_ = id;
    }

    void visit(CompoundStmt& node) override {
        int id = makeNode("Compound (" + std::to_string(node.stmts.size()) + ")", "box");
        for (auto& stmt : node.stmts) edge(id, emit(stmt.get()), "");
        last_ = id;
    }

    void visit(AssignStmt& node) override {
        int id = makeNode("Assign :=", "box");
        edge(id, emit(node.target.get()), "target");
        edge(id, emit(node.value.get()), "value");
        last_ = id;
    }

    void visit(CallStmt& node) override {
        int id = makeNode("Call: " + node.name, "box");
        for (auto& arg : node.args) edge(id, emit(arg.get()), "arg");
        last_ = id;
    }

    void visit(IfStmt& node) override {
        int id = makeNode("If", "box");
        edge(id, emit(node.cond.get()), "cond");
        edge(id, emit(node.thenBranch.get()), "then");
        edge(id, emit(node.elseBranch.get()), "else");
        last_ = id;
    }

    void visit(WhileStmt& node) override {
        int id = makeNode("While", "box");
        edge(id, emit(node.cond.get()), "cond");
        edge(id, emit(node.body.get()), "body");
        last_ = id;
    }

    void visit(BinOp& node) override {
        int id = makeNode("BinOp " + node.op + typeSuffix(node.type), "ellipse");
        edge(id, emit(node.lhs.get()), "lhs");
        edge(id, emit(node.rhs.get()), "rhs");
        last_ = id;
    }

    void visit(UnaryOp& node) override {
        int id = makeNode("Unary " + node.op + typeSuffix(node.type), "ellipse");
        edge(id, emit(node.operand.get()), "");
        last_ = id;
    }

    void visit(NumLit& node) override {
        last_ = makeNode("Num " + node.lexeme + (node.isReal ? " (real)" : " (int)"), "ellipse");
    }

    void visit(VarRef& node) override {
        if (node.index) {
            int id = makeNode("ArrayRef " + node.name + typeSuffix(node.type), "ellipse");
            edge(id, emit(node.index.get()), "index");
            last_ = id;
        } else {
            last_ = makeNode("Var " + node.name + typeSuffix(node.type), "ellipse");
        }
    }

    void visit(CallExpr& node) override {
        int id = makeNode("FuncCall " + node.name + typeSuffix(node.type), "ellipse");
        for (auto& arg : node.args) edge(id, emit(arg.get()), "arg");
        last_ = id;
    }

private:
    int makeNode(const std::string& label, const char* shape) {
        int id = counter_++;
        out_ << "  n" << id << " [label=\"" << dotEscape(label)
             << "\", shape=" << shape << "];\n";
        return id;
    }

    void edge(int from, int to, const std::string& label) {
        out_ << "  n" << from << " -> n" << to;
        if (!label.empty()) out_ << " [label=\"" << dotEscape(label) << "\"]";
        out_ << ";\n";
    }

    int emit(Node* node) {
        if (!node) { last_ = makeNode("<error>", "box"); return last_; }
        node->accept(*this);
        return last_;
    }

    std::ostream& out_;
    int           counter_ = 0;
    int           last_ = -1;
};

} // namespace

void printAstDot(Program& program, std::ostream& out) {
    out << "digraph AST {\n"
        << "  graph [ordering=out];\n"
        << "  node [fontname=\"monospace\", fontsize=10];\n"
        << "  edge [fontname=\"monospace\", fontsize=8];\n";
    AstDot dot(out);
    program.accept(dot);
    out << "}\n";
}

} // namespace mp
