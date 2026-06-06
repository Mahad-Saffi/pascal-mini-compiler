#include "semantic/semantic.hpp"

#include "util/text.hpp"

#include <string>

namespace mp {
namespace {

std::string typeText(PType type) { return ptypeName(type); }

bool isRelationalOperator(const std::string& op) {
    return op == "=" || op == "<>" || op == "<" ||
           op == "<=" || op == ">=" || op == ">";
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbols, ErrorReporter& reporter)
    : symbols_(symbols), reporter_(reporter) {}

void SemanticAnalyzer::error(const Node& node, const std::string& msg) {
    reporter_.report(Phase::Semantic, node.line, node.col, msg);
}

bool SemanticAnalyzer::numeric(PType type) {
    return type == PType::Integer || type == PType::Real;
}

bool SemanticAnalyzer::assignable(PType from, PType to) const {
    if (from == PType::Error || to == PType::Error) return true;
    if (to == PType::Real)    return from == PType::Integer || from == PType::Real;
    if (to == PType::Integer) return from == PType::Integer;
    return false;
}

PType SemanticAnalyzer::typeOf(Expr* expression) {
    if (!expression) return PType::Error;
    expression->accept(*this);
    return expression->type;
}

void SemanticAnalyzer::analyze(Program& program) {
    program.accept(*this);
}

void SemanticAnalyzer::visit(Program& node) {
    symbols_.beginScope();

    for (auto& decl : node.decls)      if (decl) decl->accept(*this);
    for (auto& sub : node.subprograms) if (sub) declareSubprogram(*sub);
    for (auto& sub : node.subprograms) if (sub) sub->accept(*this);
    if (node.body) node.body->accept(*this);

    symbols_.endScope();
}

void SemanticAnalyzer::visit(VarDecl& node) {
    const bool isArray = node.type.isArray;
    for (const std::string& name : node.names) {
        Entry entry;
        entry.name = name;
        entry.kind = isArray ? Kind::Array : Kind::Var;
        entry.type = node.type.elem;
        entry.line = node.line;
        if (isArray) entry.array = ArrayInfo{node.type.elem, node.type.lo, node.type.hi};
        if (!symbols_.insert(entry))
            error(node, "duplicate declaration of '" + name + "' in this scope");
    }
}

void SemanticAnalyzer::declareSubprogram(Subprogram& subprogram) {
    Entry entry;
    entry.name = subprogram.name;
    entry.kind = subprogram.isFunction ? Kind::Func : Kind::Proc;
    entry.line = subprogram.line;
    entry.returnType = subprogram.isFunction ? subprogram.returnType : PType::Void;
    for (const Param& param : subprogram.params)
        for (size_t i = 0; i < param.names.size(); ++i)
            entry.params.push_back(param.type.elem);
    if (!symbols_.insert(entry))
        error(subprogram, "duplicate declaration of '" + subprogram.name + "' in this scope");
}

void SemanticAnalyzer::visit(Subprogram& node) {
    symbols_.beginScope();

    for (const Param& param : node.params) {
        const bool isArray = param.type.isArray;
        for (const std::string& name : param.names) {
            Entry entry;
            entry.name = name;
            entry.kind = isArray ? Kind::Array : Kind::Param;
            entry.type = param.type.elem;
            entry.line = node.line;
            if (isArray) entry.array = ArrayInfo{param.type.elem, param.type.lo, param.type.hi};
            if (!symbols_.insert(entry))
                error(node, "duplicate parameter '" + name + "'");
        }
    }
    for (auto& decl : node.decls) if (decl) decl->accept(*this);

    std::string savedFunction = currentFunction_;
    PType       savedReturn   = currentFunctionReturn_;
    currentFunction_       = node.isFunction ? node.name : std::string();
    currentFunctionReturn_ = node.isFunction ? node.returnType : PType::Void;

    if (node.body) node.body->accept(*this);

    currentFunction_       = savedFunction;
    currentFunctionReturn_ = savedReturn;
    symbols_.endScope();
}

void SemanticAnalyzer::visit(CompoundStmt& node) {
    for (auto& stmt : node.stmts) if (stmt) stmt->accept(*this);
}

void SemanticAnalyzer::visit(AssignStmt& node) {
    PType targetType = PType::Error;
    bool  isReturn   = false;
    VarRef* target = node.target.get();

    if (target) {
        if (!currentFunction_.empty() && !target->index && target->name == currentFunction_) {
            isReturn   = true;
            targetType = currentFunctionReturn_;
        } else if (Entry* entry = symbols_.lookup(target->name)) {
            if (entry->kind == Kind::Func || entry->kind == Kind::Proc ||
                entry->kind == Kind::Builtin) {
                error(*target, "cannot assign to " + std::string(kindName(entry->kind)) +
                               " '" + target->name + "'");
            } else if (target->index) {
                if (entry->kind != Kind::Array)
                    error(*target, "'" + target->name + "' is not an array");
                else
                    targetType = entry->array ? entry->array->elem : entry->type;
                PType indexType = typeOf(target->index.get());
                if (indexType != PType::Error && indexType != PType::Integer)
                    error(*target, "array index must be integer, got " + typeText(indexType));
                checkConstIndexBounds(*entry, *target);
            } else if (entry->kind == Kind::Array) {
                error(*target, "cannot assign to entire array '" + target->name + "'");
            } else {
                targetType = entry->type;
            }
        } else {
            error(*target, "use of undeclared identifier '" + target->name + "'");
        }
    }

    PType valueType = typeOf(node.value.get());
    if (!assignable(valueType, targetType))
        error(node, "cannot assign " + typeText(valueType) + " to " +
                    (isReturn ? "the " + typeText(targetType) + " return value"
                              : typeText(targetType) + " target"));
}

void SemanticAnalyzer::visit(CallStmt& node) {
    Entry* entry = symbols_.lookup(node.name);
    if (!entry) {
        error(node, "call to undeclared procedure '" + node.name + "'");
        for (auto& arg : node.args) typeOf(arg.get());
        return;
    }

    if (entry->kind == Kind::Builtin) {
        const bool isRead = equalsIgnoreAsciiCase(node.name, "read") ||
                            equalsIgnoreAsciiCase(node.name, "readln");
        for (auto& arg : node.args) {
            typeOf(arg.get());
            if (isRead && !dynamic_cast<VarRef*>(arg.get()))
                error(*arg, "argument to '" + node.name + "' must be a variable");
        }
        return;
    }
    if (entry->kind != Kind::Func && entry->kind != Kind::Proc) {
        error(node, "'" + node.name + "' is not callable");
        for (auto& arg : node.args) typeOf(arg.get());
        return;
    }
    checkCall(*entry, node.args, node);
}

void SemanticAnalyzer::checkCall(const Entry& callee, std::vector<ExprPtr>& args,
                                 const Node& site) {
    std::vector<PType> argTypes;
    for (auto& arg : args) argTypes.push_back(typeOf(arg.get()));

    if (argTypes.size() != callee.params.size()) {
        error(site, "'" + callee.name + "' expects " +
                    std::to_string(callee.params.size()) + " argument(s), got " +
                    std::to_string(argTypes.size()));
        return;
    }
    for (size_t i = 0; i < argTypes.size(); ++i)
        if (!assignable(argTypes[i], callee.params[i]))
            error(*args[i], "argument " + std::to_string(i + 1) + " to '" +
                            callee.name + "': cannot pass " + typeText(argTypes[i]) +
                            " as " + typeText(callee.params[i]));
}

void SemanticAnalyzer::checkConstIndexBounds(const Entry& arr, const VarRef& target) {
    if (arr.kind != Kind::Array || !arr.array || !target.index) return;
    auto* literal = dynamic_cast<NumLit*>(target.index.get());
    if (!literal || literal->isReal) return;

    long value = static_cast<long>(literal->value);
    if (value < arr.array->lo || value > arr.array->hi)
        reporter_.warn(Phase::Semantic, target.line, target.col,
                       "array index " + std::to_string(value) + " is outside the bounds of '" +
                       target.name + "' [" + std::to_string(arr.array->lo) + ".." +
                       std::to_string(arr.array->hi) + "]");
}

void SemanticAnalyzer::visit(IfStmt& node) {
    PType conditionType = typeOf(node.cond.get());
    if (conditionType != PType::Error && !numeric(conditionType))
        error(node, "if-condition must be a boolean/numeric expression");
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    PType conditionType = typeOf(node.cond.get());
    if (conditionType != PType::Error && !numeric(conditionType))
        error(node, "while-condition must be a boolean/numeric expression");
    if (node.body) node.body->accept(*this);
}

void SemanticAnalyzer::visit(BinOp& node) {
    PType left  = typeOf(node.lhs.get());
    PType right = typeOf(node.rhs.get());
    const std::string& op = node.op;

    auto incompatible = [](PType type, bool wantInt) {
        if (type == PType::Error) return false;
        return wantInt ? (type != PType::Integer) : !numeric(type);
    };

    if (isRelationalOperator(op)) {
        if (incompatible(left, false) || incompatible(right, false))
            error(node, "relational operator '" + op + "' needs numeric operands");
        node.type = PType::Integer;
    } else if (op == "and" || op == "or") {
        if (incompatible(left, true) || incompatible(right, true))
            error(node, "logical operator '" + op + "' needs integer (boolean) operands");
        node.type = PType::Integer;
    } else if (op == "div" || op == "mod") {
        if (incompatible(left, true) || incompatible(right, true))
            error(node, "'" + op + "' requires integer operands");
        node.type = PType::Integer;
    } else if (op == "/") {
        if (incompatible(left, false) || incompatible(right, false))
            error(node, "'/' requires numeric operands");
        node.type = PType::Real;
    } else {
        if (incompatible(left, false) || incompatible(right, false))
            error(node, "'" + op + "' requires numeric operands");
        if (left == PType::Error || right == PType::Error) node.type = PType::Error;
        else node.type = (left == PType::Real || right == PType::Real) ? PType::Real : PType::Integer;
    }
}

void SemanticAnalyzer::visit(UnaryOp& node) {
    PType operandType = typeOf(node.operand.get());
    if (node.op == "not") {
        if (operandType != PType::Error && operandType != PType::Integer)
            error(node, "'not' requires an integer (boolean) operand");
        node.type = PType::Integer;
    } else {
        if (operandType != PType::Error && !numeric(operandType))
            error(node, "unary '" + node.op + "' requires a numeric operand");
        node.type = operandType;
    }
}

void SemanticAnalyzer::visit(NumLit& node) {
    node.type = node.isReal ? PType::Real : PType::Integer;
}

void SemanticAnalyzer::visit(VarRef& node) {
    Entry* entry = symbols_.lookup(node.name);
    if (!entry) {
        error(node, "use of undeclared identifier '" + node.name + "'");
        node.type = PType::Error;
        return;
    }
    switch (entry->kind) {
        case Kind::Var:
        case Kind::Param:
            node.type = entry->type;
            break;
        case Kind::Array:
            error(node, "array '" + node.name + "' cannot be read in an expression");
            node.type = PType::Error;
            break;
        case Kind::Func:
            if (!entry->params.empty())
                error(node, "function '" + node.name + "' expects " +
                            std::to_string(entry->params.size()) + " argument(s)");
            node.type = entry->returnType;
            break;
        case Kind::Proc:
        case Kind::Builtin:
            error(node, "procedure '" + node.name + "' cannot be used in an expression");
            node.type = PType::Error;
            break;
    }
}

void SemanticAnalyzer::visit(CallExpr& node) {
    Entry* entry = symbols_.lookup(node.name);
    if (!entry) {
        error(node, "call to undeclared function '" + node.name + "'");
        for (auto& arg : node.args) typeOf(arg.get());
        node.type = PType::Error;
        return;
    }
    if (entry->kind == Kind::Func) {
        checkCall(*entry, node.args, node);
        node.type = entry->returnType;
        return;
    }
    if (entry->kind == Kind::Proc || entry->kind == Kind::Builtin) {
        error(node, "procedure '" + node.name + "' cannot be used in an expression");
    } else {
        error(node, "'" + node.name + "' is not a function");
    }
    for (auto& arg : node.args) typeOf(arg.get());
    node.type = PType::Error;
}

} // namespace mp
