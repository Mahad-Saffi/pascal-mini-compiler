#include "diagnostics.hpp"

#include <algorithm>
#include <ostream>

namespace mp {

const char* phaseName(Phase phase) {
    switch (phase) {
        case Phase::Lexical:   return "lexical";
        case Phase::Syntactic: return "syntactic";
        case Phase::Semantic:  return "semantic";
    }
    return "unknown";
}

const char* severityName(Severity severity) {
    switch (severity) {
        case Severity::Error:   return "error";
        case Severity::Warning: return "warning";
    }
    return "error";
}

void ErrorReporter::report(Phase phase, int line, int col, const std::string& message) {
    diagnostics_.push_back(Diagnostic{phase, Severity::Error, line, col, message});
}

void ErrorReporter::warn(Phase phase, int line, int col, const std::string& message) {
    diagnostics_.push_back(Diagnostic{phase, Severity::Warning, line, col, message});
}

int ErrorReporter::countSeverity(Phase phase, Severity severity) const {
    int matches = 0;
    for (const Diagnostic& diagnostic : diagnostics_)
        if (diagnostic.phase == phase && diagnostic.severity == severity) ++matches;
    return matches;
}

bool ErrorReporter::hasErrors() const {
    return errorCount() > 0;
}

bool ErrorReporter::hasWarnings() const {
    return warningCount() > 0;
}

int ErrorReporter::count(Phase phase) const {
    return countSeverity(phase, Severity::Error);
}

int ErrorReporter::errorCount() const {
    return countSeverity(Phase::Lexical,   Severity::Error) +
           countSeverity(Phase::Syntactic, Severity::Error) +
           countSeverity(Phase::Semantic,  Severity::Error);
}

int ErrorReporter::warningCount() const {
    return countSeverity(Phase::Lexical,   Severity::Warning) +
           countSeverity(Phase::Syntactic, Severity::Warning) +
           countSeverity(Phase::Semantic,  Severity::Warning);
}

int ErrorReporter::total() const {
    return static_cast<int>(diagnostics_.size());
}

const std::vector<Diagnostic>& ErrorReporter::all() const {
    return diagnostics_;
}

void ErrorReporter::printSummary(std::ostream& out) const {
    const int errorTotal = errorCount();
    const int warningTotal = warningCount();

    if (errorTotal == 0) out << "No errors.\n";
    else
        out << "Errors: " << errorTotal
            << " (lexical "    << countSeverity(Phase::Lexical,   Severity::Error)
            << ", syntactic "  << countSeverity(Phase::Syntactic, Severity::Error)
            << ", semantic "   << countSeverity(Phase::Semantic,  Severity::Error) << ")\n";

    if (warningTotal > 0)
        out << "Warnings: " << warningTotal
            << " (lexical "    << countSeverity(Phase::Lexical,   Severity::Warning)
            << ", syntactic "  << countSeverity(Phase::Syntactic, Severity::Warning)
            << ", semantic "   << countSeverity(Phase::Semantic,  Severity::Warning) << ")\n";

    if (diagnostics_.empty()) return;

    std::vector<const Diagnostic*> ordered;
    ordered.reserve(diagnostics_.size());
    for (const Diagnostic& diagnostic : diagnostics_) ordered.push_back(&diagnostic);

    std::stable_sort(ordered.begin(), ordered.end(),
        [](const Diagnostic* left, const Diagnostic* right) {
            if (left->line != right->line) return left->line < right->line;
            return left->col < right->col;
        });

    for (const Diagnostic* diagnostic : ordered) {
        out << "  [" << phaseName(diagnostic->phase);
        if (diagnostic->severity == Severity::Warning) out << " warning";
        out << "] line " << diagnostic->line << ", col " << diagnostic->col << ": "
            << diagnostic->message << '\n';
    }
}

void ErrorReporter::clear() {
    diagnostics_.clear();
}

}
