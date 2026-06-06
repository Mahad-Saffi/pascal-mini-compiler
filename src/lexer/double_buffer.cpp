#include "lexer/double_buffer.hpp"

#include <istream>

namespace mp {

DoubleBuffer::DoubleBuffer(std::istream& input) : input_(input) {}

void DoubleBuffer::ensureLoadedFor(long long absolutePosition) {
    long long blockIndex = absolutePosition / kHalf;
    while (nextBlock_ <= blockIndex && !streamEnded_) {
        int halfIndex = static_cast<int>(nextBlock_ % 2);
        input_.read(buffers_[halfIndex], kHalf);
        int bytesRead = static_cast<int>(input_.gcount());
        fillCount_[halfIndex] = bytesRead;
        buffers_[halfIndex][bytesRead] = kEOF;
        blockInHalf_[halfIndex] = nextBlock_;
        if (bytesRead < kHalf) streamEnded_ = true;
        ++nextBlock_;
    }
}

char DoubleBuffer::characterAt(long long absolutePosition) {
    ensureLoadedFor(absolutePosition);
    long long blockIndex = absolutePosition / kHalf;
    int halfIndex = static_cast<int>(blockIndex % 2);
    if (blockInHalf_[halfIndex] != blockIndex) return kEOF;
    int offset = static_cast<int>(absolutePosition % kHalf);
    if (offset >= fillCount_[halfIndex]) return kEOF;
    return buffers_[halfIndex][offset];
}

char DoubleBuffer::peek(int lookahead) {
    return characterAt(position_ + lookahead);
}

char DoubleBuffer::advance() {
    char character = characterAt(position_);
    if (character == kEOF) return kEOF;
    ++position_;
    if (character == '\n') { ++line_; col_ = 1; }
    else                   { ++col_; }
    return character;
}

}
