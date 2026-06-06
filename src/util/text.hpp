#ifndef MINIPASCAL_UTIL_TEXT_HPP
#define MINIPASCAL_UTIL_TEXT_HPP

#include <string>
#include <vector>

namespace mp {

char foldAsciiLower(char character);
std::string toAsciiLower(const std::string& text);
bool equalsIgnoreAsciiCase(const std::string& left, const std::string& right);

std::string joinStrings(const std::vector<std::string>& items, const char* separator);
std::string clipColumn(const std::string& text, std::size_t width, bool keepRight);

}

#endif
