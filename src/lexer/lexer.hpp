#ifndef MINIPASCAL_LEXER_LEXER_HPP
#define MINIPASCAL_LEXER_LEXER_HPP

#include "diagnostics.hpp"
#include "lexer/double_buffer.hpp"
#include "lexer/token.hpp"

#include <iosfwd>
#include <vector>

namespace mp {

class Lexer {
public:
    Lexer(std::istream& input, ErrorReporter& reporter);

    Token next();
    std::vector<Token> tokenize();

private:
    void  skipTrivia();
    Token lexWord(int line, int col);
    Token lexNumber(int line, int col);

    DoubleBuffer   buffer_;
    ErrorReporter& reporter_;
};

}

#endif
