#include "lexer/lexer.hpp"

#include "util/text.hpp"

#include <istream>
#include <string>
#include <unordered_map>

namespace mp {
namespace {

bool isLetter(char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}
bool isDigit(char character) { return character >= '0' && character <= '9'; }

const std::unordered_map<std::string, TokType>& keywords() {
    static const std::unordered_map<std::string, TokType> table = {
        {"program", TokType::KwProgram},   {"var", TokType::KwVar},
        {"array", TokType::KwArray},       {"of", TokType::KwOf},
        {"integer", TokType::KwInteger},   {"real", TokType::KwReal},
        {"function", TokType::KwFunction}, {"procedure", TokType::KwProcedure},
        {"begin", TokType::KwBegin},       {"end", TokType::KwEnd},
        {"if", TokType::KwIf},             {"then", TokType::KwThen},
        {"else", TokType::KwElse},         {"while", TokType::KwWhile},
        {"do", TokType::KwDo},             {"not", TokType::KwNot},
        {"div", TokType::KwDiv},           {"mod", TokType::KwMod},
        {"and", TokType::KwAnd},           {"or", TokType::KwOr},
    };
    return table;
}

std::string printable(char character) {
    unsigned char byte = static_cast<unsigned char>(character);
    if (byte >= 32 && byte < 127) return std::string(1, character);
    static const char* hexDigits = "0123456789ABCDEF";
    std::string escaped = "\\x";
    escaped += hexDigits[(byte >> 4) & 0xF];
    escaped += hexDigits[byte & 0xF];
    return escaped;
}

}

Lexer::Lexer(std::istream& input, ErrorReporter& reporter)
    : buffer_(input), reporter_(reporter) {}

void Lexer::skipTrivia() {
    for (;;) {
        char character = buffer_.peek();
        if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            buffer_.advance();
            continue;
        }
        if (character == '{') {
            int startLine = buffer_.line(), startCol = buffer_.col();
            buffer_.advance();
            bool closed = false;
            for (;;) {
                char commentChar = buffer_.peek();
                if (commentChar == DoubleBuffer::kEOF) break;
                buffer_.advance();
                if (commentChar == '}') { closed = true; break; }
            }
            if (!closed)
                reporter_.report(Phase::Lexical, startLine, startCol,
                                 "unterminated comment '{' (reached end of file)");
            continue;
        }
        break;
    }
}

Token Lexer::lexWord(int line, int col) {
    std::string text;
    while (isLetter(buffer_.peek()) || isDigit(buffer_.peek()))
        text.push_back(buffer_.advance());

    auto match = keywords().find(toAsciiLower(text));
    if (match != keywords().end()) return Token{match->second, toAsciiLower(text), line, col};
    return Token{TokType::Ident, text, line, col};
}

Token Lexer::lexNumber(int line, int col) {
    std::string text;
    bool isReal = false;

    while (isDigit(buffer_.peek())) text.push_back(buffer_.advance());

    if (buffer_.peek() == '.' && isDigit(buffer_.peek(1))) {
        isReal = true;
        text.push_back(buffer_.advance());
        while (isDigit(buffer_.peek())) text.push_back(buffer_.advance());
    }

    char exponentChar = buffer_.peek();
    if (exponentChar == 'e' || exponentChar == 'E') {
        char afterExponent = buffer_.peek(1);
        bool wellFormed = isDigit(afterExponent) ||
                          ((afterExponent == '+' || afterExponent == '-') && isDigit(buffer_.peek(2)));
        if (wellFormed) {
            isReal = true;
            text.push_back(buffer_.advance());
            if (buffer_.peek() == '+' || buffer_.peek() == '-')
                text.push_back(buffer_.advance());
            while (isDigit(buffer_.peek())) text.push_back(buffer_.advance());
        }
    }

    return Token{isReal ? TokType::RealConst : TokType::IntConst, text, line, col};
}

Token Lexer::next() {
    for (;;) {
        skipTrivia();
        int line = buffer_.line(), col = buffer_.col();
        char character = buffer_.peek();

        if (character == DoubleBuffer::kEOF) return Token{TokType::EndOfFile, "", line, col};
        if (isLetter(character)) return lexWord(line, col);
        if (isDigit(character))  return lexNumber(line, col);

        switch (character) {
            case ':':
                buffer_.advance();
                if (buffer_.peek() == '=') { buffer_.advance(); return Token{TokType::Assign, ":=", line, col}; }
                return Token{TokType::Colon, ":", line, col};
            case '<':
                buffer_.advance();
                if (buffer_.peek() == '=') { buffer_.advance(); return Token{TokType::Le, "<=", line, col}; }
                if (buffer_.peek() == '>') { buffer_.advance(); return Token{TokType::Neq, "<>", line, col}; }
                return Token{TokType::Lt, "<", line, col};
            case '>':
                buffer_.advance();
                if (buffer_.peek() == '=') { buffer_.advance(); return Token{TokType::Ge, ">=", line, col}; }
                return Token{TokType::Gt, ">", line, col};
            case '.':
                buffer_.advance();
                if (buffer_.peek() == '.') { buffer_.advance(); return Token{TokType::DotDot, "..", line, col}; }
                return Token{TokType::Dot, ".", line, col};
            case '+': buffer_.advance(); return Token{TokType::Plus, "+", line, col};
            case '-': buffer_.advance(); return Token{TokType::Minus, "-", line, col};
            case '*': buffer_.advance(); return Token{TokType::Star, "*", line, col};
            case '/': buffer_.advance(); return Token{TokType::Slash, "/", line, col};
            case '=': buffer_.advance(); return Token{TokType::Eq, "=", line, col};
            case '(': buffer_.advance(); return Token{TokType::LParen, "(", line, col};
            case ')': buffer_.advance(); return Token{TokType::RParen, ")", line, col};
            case '[': buffer_.advance(); return Token{TokType::LBracket, "[", line, col};
            case ']': buffer_.advance(); return Token{TokType::RBracket, "]", line, col};
            case ';': buffer_.advance(); return Token{TokType::Semicolon, ";", line, col};
            case ',': buffer_.advance(); return Token{TokType::Comma, ",", line, col};
            default:
                buffer_.advance();
                reporter_.report(Phase::Lexical, line, col,
                                 "illegal character '" + printable(character) + "'");
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        Token token = next();
        tokens.push_back(token);
        if (token.type == TokType::EndOfFile) break;
    }
    return tokens;
}

}
