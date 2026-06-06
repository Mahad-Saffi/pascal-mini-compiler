#ifndef MINIPASCAL_DIAGNOSTICS_HPP
#define MINIPASCAL_DIAGNOSTICS_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace mp {

enum class Phase { Lexical, Syntactic, Semantic };

enum class Severity { Error, Warning };

const char* phaseName(Phase phase);
const char* severityName(Severity severity);

struct Diagnostic {
    Phase       phase;
    Severity    severity = Severity::Error;
    int         line;
    int         col;
    std::string message;
};

class ErrorReporter {
public:
    void report(Phase phase, int line, int col, const std::string& message);
    void warn(Phase phase, int line, int col, const std::string& message);

    bool hasErrors() const;
    bool hasWarnings() const;
    int  count(Phase phase) const;
    int  errorCount() const;
    int  warningCount() const;
    int  total() const;

    const std::vector<Diagnostic>& all() const;

    void printSummary(std::ostream& out) const;

    void clear();

private:
    int countSeverity(Phase phase, Severity severity) const;
    std::vector<Diagnostic> diagnostics_;
};

}

#endif
