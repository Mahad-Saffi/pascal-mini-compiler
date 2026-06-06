#include "lexer/token.hpp"

namespace mp {

const char* tokTypeName(TokType type) {
    switch (type) {
        case TokType::Ident:       return "Ident";
        case TokType::IntConst:    return "IntConst";
        case TokType::RealConst:   return "RealConst";
        case TokType::KwProgram:   return "KwProgram";
        case TokType::KwVar:       return "KwVar";
        case TokType::KwArray:     return "KwArray";
        case TokType::KwOf:        return "KwOf";
        case TokType::KwInteger:   return "KwInteger";
        case TokType::KwReal:      return "KwReal";
        case TokType::KwFunction:  return "KwFunction";
        case TokType::KwProcedure: return "KwProcedure";
        case TokType::KwBegin:     return "KwBegin";
        case TokType::KwEnd:       return "KwEnd";
        case TokType::KwIf:        return "KwIf";
        case TokType::KwThen:      return "KwThen";
        case TokType::KwElse:      return "KwElse";
        case TokType::KwWhile:     return "KwWhile";
        case TokType::KwDo:        return "KwDo";
        case TokType::KwNot:       return "KwNot";
        case TokType::KwDiv:       return "KwDiv";
        case TokType::KwMod:       return "KwMod";
        case TokType::KwAnd:       return "KwAnd";
        case TokType::KwOr:        return "KwOr";
        case TokType::Assign:      return "Assign";
        case TokType::Plus:        return "Plus";
        case TokType::Minus:       return "Minus";
        case TokType::Star:        return "Star";
        case TokType::Slash:       return "Slash";
        case TokType::Eq:          return "Eq";
        case TokType::Neq:         return "Neq";
        case TokType::Lt:          return "Lt";
        case TokType::Le:          return "Le";
        case TokType::Ge:          return "Ge";
        case TokType::Gt:          return "Gt";
        case TokType::LParen:      return "LParen";
        case TokType::RParen:      return "RParen";
        case TokType::LBracket:    return "LBracket";
        case TokType::RBracket:    return "RBracket";
        case TokType::Semicolon:   return "Semicolon";
        case TokType::Colon:       return "Colon";
        case TokType::Comma:       return "Comma";
        case TokType::Dot:         return "Dot";
        case TokType::DotDot:      return "DotDot";
        case TokType::EndOfFile:   return "EndOfFile";
    }
    return "?";
}

std::string grammarTerminal(const Token& token) {
    switch (token.type) {
        case TokType::Ident:     return "id";
        case TokType::IntConst:  return "num";
        case TokType::RealConst: return "num";
        case TokType::Assign:    return "assignop";
        case TokType::EndOfFile: return "$";
        default:                 return token.lexeme;
    }
}

}
