#include "util/text.hpp"

namespace mp {

char foldAsciiLower(char character) {
    if (character >= 'A' && character <= 'Z')
        return static_cast<char>(character - 'A' + 'a');
    return character;
}

std::string toAsciiLower(const std::string& text) {
    std::string lowered = text;
    for (char& character : lowered) character = foldAsciiLower(character);
    return lowered;
}

bool equalsIgnoreAsciiCase(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t position = 0; position < left.size(); ++position)
        if (foldAsciiLower(left[position]) != foldAsciiLower(right[position]))
            return false;
    return true;
}

std::string joinStrings(const std::vector<std::string>& items, const char* separator) {
    std::string joined;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index) joined += separator;
        joined += items[index];
    }
    return joined;
}

std::string clipColumn(const std::string& text, std::size_t width, bool keepRight) {
    if (text.size() <= width) return text;
    if (width <= 3) return text.substr(0, width);
    return keepRight ? "..." + text.substr(text.size() - (width - 3))
                     : text.substr(0, width - 3) + "...";
}

}
