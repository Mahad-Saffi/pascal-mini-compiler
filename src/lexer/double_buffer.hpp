#ifndef MINIPASCAL_LEXER_DOUBLE_BUFFER_HPP
#define MINIPASCAL_LEXER_DOUBLE_BUFFER_HPP

#include <iosfwd>

namespace mp {

class DoubleBuffer {
public:
    explicit DoubleBuffer(std::istream& input);

    static constexpr char kEOF = '\0';

    char advance();
    char peek(int lookahead = 0);

    int line() const { return line_; }
    int col()  const { return col_; }

private:
    char characterAt(long long absolutePosition);
    void ensureLoadedFor(long long absolutePosition);

    static constexpr int kHalf = 4096;

    std::istream& input_;
    char      buffers_[2][kHalf + 1];
    int       fillCount_[2] = {0, 0};
    long long blockInHalf_[2] = {-1, -1};
    long long nextBlock_ = 0;
    bool      streamEnded_ = false;
    long long position_ = 0;
    int line_ = 1;
    int col_ = 1;
};

}

#endif
