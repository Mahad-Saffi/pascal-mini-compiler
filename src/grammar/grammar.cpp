#include "grammar/grammar.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace mp {
namespace {

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) tokens.push_back(token);
    return tokens;
}

}

bool Grammar::looksLikeNonterminal(const std::string& token) {
    if (token.size() < 3) return false;
    if (token.front() != '<' || token.back() != '>') return false;
    std::string inner = token.substr(1, token.size() - 2);
    auto isValidFirst = [](char character) {
        return std::isalpha(static_cast<unsigned char>(character)) || character == '_';
    };
    auto isValidRest = [](char character) {
        return std::isalnum(static_cast<unsigned char>(character)) || character == '_' || character == '\'';
    };
    if (!isValidFirst(inner[0])) return false;
    for (char character : inner)
        if (!isValidRest(character)) return false;
    return true;
}

void Grammar::classify(const std::string& symbol) {
    if (looksLikeNonterminal(symbol)) nonterminals_.insert(symbol);
    else                              terminals_.insert(symbol);
}

void Grammar::addProduction(const Production& production) {
    nonterminals_.insert(production.lhs);
    for (const std::string& symbol : production.rhs) classify(symbol);
    productions_.push_back(production);
}

std::vector<int> Grammar::productionsFor(const std::string& nonterminal) const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(productions_.size()); ++i)
        if (productions_[i].lhs == nonterminal) indices.push_back(i);
    return indices;
}

std::vector<std::string> Grammar::nonterminalsInOrder() const {
    std::vector<std::string> order;
    std::set<std::string> seen;
    for (const Production& production : productions_)
        if (seen.insert(production.lhs).second) order.push_back(production.lhs);
    return order;
}

Grammar Grammar::loadBNF(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open grammar file: " + path);

    Grammar grammar;
    std::string currentLhs;
    std::vector<std::string> currentRhs;

    auto flush = [&]() {
        if (currentLhs.empty()) return;
        std::vector<std::string> alternative;
        auto emitAlternative = [&]() {
            if (alternative.empty()) return;
            Production production;
            production.lhs = currentLhs;
            if (!(alternative.size() == 1 && alternative[0] == "epsilon")) production.rhs = alternative;
            grammar.addProduction(production);
            alternative.clear();
        };
        for (const std::string& token : currentRhs) {
            if (token == "|") emitAlternative();
            else              alternative.push_back(token);
        }
        emitAlternative();
        currentRhs.clear();
        currentLhs.clear();
    };

    std::string line;
    while (std::getline(input, line)) {
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos) line.erase(commentPos);
        std::vector<std::string> tokens = splitWhitespace(line);
        if (tokens.empty()) continue;

        if (tokens.size() >= 2 && tokens[1] == "->") {
            flush();
            currentLhs = tokens[0];
            currentRhs.assign(tokens.begin() + 2, tokens.end());
        } else {
            currentRhs.insert(currentRhs.end(), tokens.begin(), tokens.end());
        }
    }
    flush();

    if (grammar.productions_.empty()) throw std::runtime_error("empty grammar: " + path);
    grammar.start_ = grammar.productions_.front().lhs;
    return grammar;
}

std::string formatRhs(const std::vector<std::string>& rhs) {
    if (rhs.empty()) return "ε";
    std::string out;
    for (size_t i = 0; i < rhs.size(); ++i) {
        if (i) out += ' ';
        out += rhs[i];
    }
    return out;
}

std::string ruleText(const Production& production) {
    return production.lhs + " -> " + formatRhs(production.rhs);
}

std::string freshPrimedName(const std::string& nonterminal,
                            const std::function<bool(const std::string&)>& isTaken) {
    std::string base = nonterminal.substr(0, nonterminal.size() - 1);
    std::string candidate;
    do {
        base += "'";
        candidate = base + ">";
    } while (isTaken(candidate));
    return candidate;
}

void Grammar::print(std::ostream& out) const {
    std::vector<std::string> order = nonterminalsInOrder();

    size_t width = 0;
    for (const std::string& nonterminal : order)
        width = std::max(width, nonterminal.size());

    for (const std::string& nonterminal : order) {
        std::vector<int> indices = productionsFor(nonterminal);
        out << std::left << std::setw(static_cast<int>(width)) << nonterminal << " -> ";
        for (size_t i = 0; i < indices.size(); ++i) {
            if (i) out << "\n" << std::string(width, ' ') << "  | ";
            out << formatRhs(productions_[indices[i]].rhs);
        }
        out << "\n";
    }
}

}
