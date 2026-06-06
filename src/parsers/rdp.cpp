#include "parsers/rdp.hpp"

#include <cstdlib>
#include <utility>

namespace mp {
namespace {

bool isRelop(TokType type) {
    return type == TokType::Eq || type == TokType::Neq || type == TokType::Lt ||
           type == TokType::Le || type == TokType::Ge || type == TokType::Gt;
}
bool isAddop(TokType type) {
    return type == TokType::Plus || type == TokType::Minus || type == TokType::KwOr;
}
bool isMulop(TokType type) {
    return type == TokType::Star || type == TokType::Slash || type == TokType::KwDiv ||
           type == TokType::KwMod || type == TokType::KwAnd;
}

int toInt(const std::string& text) {
    return static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
}
double toDouble(const std::string& text) {
    return std::strtod(text.c_str(), nullptr);
}

}

RecursiveDescentParser::RecursiveDescentParser(Lexer& lexer, ErrorReporter& reporter)
    : tokens_(lexer.tokenize()), reporter_(reporter) {}

const Token& RecursiveDescentParser::peek(int offset) const {
    size_t i = position_ + static_cast<size_t>(offset);
    if (i >= tokens_.size()) return tokens_.back();
    return tokens_[i];
}

Token RecursiveDescentParser::advance() {
    Token token = cur();
    if (position_ + 1 < tokens_.size()) ++position_;
    return token;
}

bool RecursiveDescentParser::accept(TokType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

void RecursiveDescentParser::errorHere(const std::string& msg) {
    const Token& token = cur();
    reporter_.report(Phase::Syntactic, token.line, token.col, msg);
}

bool RecursiveDescentParser::expect(TokType type, const char* what) {
    if (check(type)) { advance(); return true; }
    errorHere(std::string("expected ") + what);
    return false;
}

void RecursiveDescentParser::recover(std::initializer_list<TokType> sync) {
    while (!check(TokType::EndOfFile)) {
        for (TokType syncToken : sync)
            if (check(syncToken)) return;
        advance();
    }
}

void RecursiveDescentParser::at(Node& node) const {
    node.line = cur().line;
    node.col  = cur().col;
}

bool RecursiveDescentParser::startsStatement(TokType type) {
    return type == TokType::KwBegin || type == TokType::KwIf ||
           type == TokType::KwWhile || type == TokType::Ident;
}

std::unique_ptr<VarRef> RecursiveDescentParser::makeVarRef(int line, int col, const std::string& name) {
    auto var = std::make_unique<VarRef>();
    var->line = line;
    var->col  = col;
    var->name = name;
    return var;
}

ExprPtr RecursiveDescentParser::foldBinary(ExprPtr left,
        const std::function<bool(TokType)>& isOperator,
        const std::function<ExprPtr()>& nextOperand) {
    while (isOperator(cur().type)) {
        auto binary = std::make_unique<BinOp>();
        at(*binary);
        binary->op  = advance().lexeme;
        binary->lhs = std::move(left);
        binary->rhs = nextOperand();
        left = std::move(binary);
    }
    return left;
}

std::unique_ptr<Program> RecursiveDescentParser::parse() {
    return program();
}

std::unique_ptr<Program> RecursiveDescentParser::program() {
    auto programNode = std::make_unique<Program>();
    at(*programNode);

    expect(TokType::KwProgram, "'program'");
    if (check(TokType::Ident)) programNode->name = advance().lexeme;
    else                       errorHere("expected the program name");

    expect(TokType::LParen, "'('");
    identifierList(programNode->params);
    expect(TokType::RParen, "')'");
    expect(TokType::Semicolon, "';'");

    declarations(programNode->decls);
    subprogramDeclarations(programNode->subprograms);
    programNode->body = compoundStatement();
    expect(TokType::Dot, "'.' after the final 'end'");

    if (!check(TokType::EndOfFile))
        errorHere("expected end of file after the final 'end.'");
    return programNode;
}

void RecursiveDescentParser::identifierList(std::vector<std::string>& out) {
    if (check(TokType::Ident)) out.push_back(advance().lexeme);
    else { errorHere("expected an identifier"); return; }

    while (accept(TokType::Comma)) {
        if (check(TokType::Ident)) out.push_back(advance().lexeme);
        else { errorHere("expected an identifier after ','"); break; }
    }
}

void RecursiveDescentParser::declarations(std::vector<std::unique_ptr<VarDecl>>& out) {
    while (check(TokType::KwVar)) {
        auto varDecl = std::make_unique<VarDecl>();
        at(*varDecl);
        advance();
        identifierList(varDecl->names);
        expect(TokType::Colon, "':'");
        varDecl->type = type();
        expect(TokType::Semicolon, "';'");
        out.push_back(std::move(varDecl));
    }
}

TypeNode RecursiveDescentParser::type() {
    TypeNode typeNode;
    if (accept(TokType::KwArray)) {
        typeNode.isArray = true;
        expect(TokType::LBracket, "'['");
        if (check(TokType::IntConst)) typeNode.lo = toInt(advance().lexeme);
        else                          errorHere("expected an integer lower bound");
        expect(TokType::DotDot, "'..'");
        if (check(TokType::IntConst)) typeNode.hi = toInt(advance().lexeme);
        else                          errorHere("expected an integer upper bound");
        expect(TokType::RBracket, "']'");
        expect(TokType::KwOf, "'of'");
        typeNode.elem = standardType();
    } else {
        typeNode.elem = standardType();
    }
    return typeNode;
}

PType RecursiveDescentParser::standardType() {
    if (accept(TokType::KwInteger)) return PType::Integer;
    if (accept(TokType::KwReal))    return PType::Real;
    errorHere("expected 'integer' or 'real'");
    return PType::Error;
}

void RecursiveDescentParser::subprogramDeclarations(
        std::vector<std::unique_ptr<Subprogram>>& out) {
    while (check(TokType::KwFunction) || check(TokType::KwProcedure)) {
        if (auto subprogram = subprogramDeclaration()) out.push_back(std::move(subprogram));
        expect(TokType::Semicolon, "';'");
    }
}

std::unique_ptr<Subprogram> RecursiveDescentParser::subprogramDeclaration() {
    auto subprogram = std::make_unique<Subprogram>();
    at(*subprogram);
    subprogram->isFunction = check(TokType::KwFunction);
    advance();

    if (check(TokType::Ident)) subprogram->name = advance().lexeme;
    else                       errorHere("expected the subprogram name");

    arguments(subprogram->params);
    if (subprogram->isFunction) {
        expect(TokType::Colon, "':'");
        subprogram->returnType = standardType();
    }
    expect(TokType::Semicolon, "';'");

    declarations(subprogram->decls);
    subprogram->body = compoundStatement();
    return subprogram;
}

void RecursiveDescentParser::arguments(std::vector<Param>& out) {
    if (accept(TokType::LParen)) {
        parameterList(out);
        expect(TokType::RParen, "')'");
    }
}

void RecursiveDescentParser::parameterList(std::vector<Param>& out) {
    do {
        Param param;
        identifierList(param.names);
        expect(TokType::Colon, "':'");
        param.type = type();
        out.push_back(std::move(param));
    } while (accept(TokType::Semicolon));
}

std::unique_ptr<CompoundStmt> RecursiveDescentParser::compoundStatement() {
    auto compound = std::make_unique<CompoundStmt>();
    at(*compound);
    expect(TokType::KwBegin, "'begin'");
    if (!check(TokType::KwEnd) && !check(TokType::EndOfFile))
        statementList(compound->stmts);
    expect(TokType::KwEnd, "'end'");
    return compound;
}

void RecursiveDescentParser::statementList(std::vector<StmtPtr>& out) {
    if (auto stmt = statement()) out.push_back(std::move(stmt));

    for (;;) {
        if (accept(TokType::Semicolon)) {
            if (startsStatement(cur().type)) {
                if (auto stmt = statement()) out.push_back(std::move(stmt));
            } else {
                break;
            }
        } else if (startsStatement(cur().type)) {
            errorHere("expected ';' between statements");
            if (auto stmt = statement()) out.push_back(std::move(stmt));
        } else {
            break;
        }
    }
}

StmtPtr RecursiveDescentParser::statement() {
    switch (cur().type) {
        case TokType::KwBegin:
            return compoundStatement();

        case TokType::KwIf: {
            auto stmt = std::make_unique<IfStmt>();
            at(*stmt);
            advance();
            stmt->cond = expression();
            expect(TokType::KwThen, "'then'");
            stmt->thenBranch = statement();
            if (expect(TokType::KwElse, "'else' (mandatory in this subset)"))
                stmt->elseBranch = statement();
            return stmt;
        }

        case TokType::KwWhile: {
            auto stmt = std::make_unique<WhileStmt>();
            at(*stmt);
            advance();
            stmt->cond = expression();
            expect(TokType::KwDo, "'do'");
            stmt->body = statement();
            return stmt;
        }

        case TokType::Ident: {
            int line = cur().line;
            int col  = cur().col;
            std::string name  = cur().lexeme;
            TokType     ahead = peek(1).type;

            if (ahead == TokType::LBracket) {
                auto target = makeVarRef(line, col, name);
                advance();
                advance();
                target->index = expression();
                expect(TokType::RBracket, "']'");

                auto stmt = std::make_unique<AssignStmt>();
                stmt->line = line;
                stmt->col  = col;
                stmt->target = std::move(target);
                expect(TokType::Assign, "':='");
                stmt->value = expression();
                return stmt;
            }
            if (ahead == TokType::Assign) {
                auto target = makeVarRef(line, col, name);
                advance();
                advance();

                auto stmt = std::make_unique<AssignStmt>();
                stmt->line = line;
                stmt->col  = col;
                stmt->target = std::move(target);
                stmt->value = expression();
                return stmt;
            }
            if (ahead == TokType::LParen) {
                auto stmt = std::make_unique<CallStmt>();
                stmt->line = line;
                stmt->col  = col;
                stmt->name = name;
                advance();
                advance();
                expressionList(stmt->args);
                expect(TokType::RParen, "')'");
                return stmt;
            }
            auto stmt = std::make_unique<CallStmt>();
            stmt->line = line;
            stmt->col  = col;
            stmt->name = name;
            advance();
            return stmt;
        }

        default:
            errorHere("expected a statement");
            advance();
            return nullptr;
    }
}

ExprPtr RecursiveDescentParser::expression() {
    ExprPtr lhs = simpleExpression();
    if (isRelop(cur().type)) {
        auto binary = std::make_unique<BinOp>();
        at(*binary);
        binary->op  = advance().lexeme;
        binary->lhs = std::move(lhs);
        binary->rhs = simpleExpression();
        return binary;
    }
    return lhs;
}

ExprPtr RecursiveDescentParser::simpleExpression() {
    ExprPtr operand;
    if (check(TokType::Plus) || check(TokType::Minus)) {
        auto unary = std::make_unique<UnaryOp>();
        at(*unary);
        unary->op = advance().lexeme;
        unary->operand = term();
        operand = std::move(unary);
    } else {
        operand = term();
    }
    return foldBinary(std::move(operand), isAddop, [this] { return term(); });
}

ExprPtr RecursiveDescentParser::term() {
    return foldBinary(factor(), isMulop, [this] { return factor(); });
}

ExprPtr RecursiveDescentParser::factor() {
    switch (cur().type) {
        case TokType::Ident: {
            int line = cur().line;
            int col  = cur().col;
            std::string name = advance().lexeme;
            if (accept(TokType::LParen)) {
                auto call = std::make_unique<CallExpr>();
                call->line = line;
                call->col  = col;
                call->name = name;
                expressionList(call->args);
                expect(TokType::RParen, "')'");
                return call;
            }
            return makeVarRef(line, col, name);
        }

        case TokType::IntConst:
        case TokType::RealConst: {
            auto num = std::make_unique<NumLit>();
            at(*num);
            num->isReal = check(TokType::RealConst);
            num->lexeme = cur().lexeme;
            num->value  = toDouble(cur().lexeme);
            advance();
            return num;
        }

        case TokType::LParen: {
            advance();
            ExprPtr expr = expression();
            expect(TokType::RParen, "')'");
            return expr;
        }

        case TokType::KwNot: {
            auto unary = std::make_unique<UnaryOp>();
            at(*unary);
            unary->op = advance().lexeme;
            unary->operand = factor();
            return unary;
        }

        default:
            errorHere("expected an expression");
            return nullptr;
    }
}

void RecursiveDescentParser::expressionList(std::vector<ExprPtr>& out) {
    if (auto expr = expression()) out.push_back(std::move(expr));
    while (accept(TokType::Comma)) {
        if (auto expr = expression()) out.push_back(std::move(expr));
    }
}

}
