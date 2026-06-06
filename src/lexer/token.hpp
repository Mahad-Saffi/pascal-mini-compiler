#ifndef MINIPASCAL_LEXER_TOKEN_HPP
#define MINIPASCAL_LEXER_TOKEN_HPP

#include <string>

namespace mp {

enum class TokType {
    Ident, IntConst, RealConst,
    KwProgram, KwVar, KwArray, KwOf, KwInteger, KwReal,
    KwFunction, KwProcedure, KwBegin, KwEnd,
    KwIf, KwThen, KwElse, KwWhile, KwDo,
    KwNot, KwDiv, KwMod, KwAnd, KwOr,
    Assign,
    Plus, Minus, Star, Slash,
    Eq, Neq, Lt, Le, Ge, Gt,
    LParen, RParen, LBracket, RBracket,
    Semicolon, Colon, Comma, Dot, DotDot,
    EndOfFile
};

struct Token {
    TokType     type;
    std::string lexeme;
    int         line;
    int         col;
};

const char* tokTypeName(TokType type);

std::string grammarTerminal(const Token& token);

}

#endif
