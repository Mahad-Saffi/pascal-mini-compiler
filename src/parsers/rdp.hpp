#ifndef MINIPASCAL_PARSERS_RDP_HPP
#define MINIPASCAL_PARSERS_RDP_HPP

#include "ast/ast.hpp"
#include "diagnostics.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace mp {

class RecursiveDescentParser {
public:
    RecursiveDescentParser(Lexer& lexer, ErrorReporter& reporter);

    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens_;
    size_t             position_ = 0;
    ErrorReporter&     reporter_;

    const Token& peek(int offset = 0) const;
    const Token& cur() const { return peek(0); }
    bool   check(TokType type) const { return cur().type == type; }
    bool   accept(TokType type);
    bool   expect(TokType type, const char* what);
    Token  advance();
    void   errorHere(const std::string& msg);
    void   recover(std::initializer_list<TokType> sync);
    void   at(Node& node) const;
    static bool startsStatement(TokType type);

    std::unique_ptr<VarRef> makeVarRef(int line, int col, const std::string& name);
    ExprPtr foldBinary(ExprPtr left,
                       const std::function<bool(TokType)>& isOperator,
                       const std::function<ExprPtr()>& nextOperand);

    std::unique_ptr<Program>      program();
    void                          identifierList(std::vector<std::string>& out);
    void                          declarations(std::vector<std::unique_ptr<VarDecl>>& out);
    TypeNode                      type();
    PType                         standardType();
    void                          subprogramDeclarations(std::vector<std::unique_ptr<Subprogram>>& out);
    std::unique_ptr<Subprogram>   subprogramDeclaration();
    void                          arguments(std::vector<Param>& out);
    void                          parameterList(std::vector<Param>& out);
    std::unique_ptr<CompoundStmt> compoundStatement();
    void                          statementList(std::vector<StmtPtr>& out);
    StmtPtr                       statement();
    ExprPtr                       expression();
    ExprPtr                       simpleExpression();
    ExprPtr                       term();
    ExprPtr                       factor();
    void                          expressionList(std::vector<ExprPtr>& out);
};

}

#endif
