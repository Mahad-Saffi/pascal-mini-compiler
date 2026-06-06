#include "ast/ast.hpp"
#include "diagnostics.hpp"
#include "grammar/first_follow.hpp"
#include "grammar/grammar.hpp"
#include "grammar/ll1_table.hpp"
#include "grammar/slr_table.hpp"
#include "grammar/transform.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "parsers/lr.hpp"
#include "parsers/predictive.hpp"
#include "parsers/rdp.hpp"
#include "semantic/semantic.hpp"
#include "symtab/symbol_table.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* kVersion = "0.7.0 (P7 AST DOT export, output/ artifacts, report-table "
                       "dumps, Bison/Flex SLR cross-check)";
const char* kDefaultBnf = "docs/grammar/original.bnf";

void printBanner() {
    std::cout << "Mini Pascal Compiler " << kVersion << "\n"
              << "CS-471L final project - unified front end for the Subset of Pascal.\n\n";
}

void printUsage() {
    std::cout << "Usage:\n"
              << "  minipascal                    show this overview\n"
              << "  minipascal lex <file>         tokenize a .pas file and print the token stream\n"
              << "  minipascal rdp <file>         recursive-descent parse a .pas file and print its AST\n"
              << "  minipascal dot <file>         emit the AST as a Graphviz DOT digraph (for `dot -Tpng`)\n"
              << "  minipascal ll1 <file>         LL(1) predictive (table-driven) parse + trace\n"
              << "  minipascal lr <file>          SLR(1) shift-reduce parse + trace\n"
              << "  minipascal symtab <file>      build and dump the symbol table for a .pas file\n"
              << "  minipascal semant <file>      semantic-analyze a .pas file (types + symbol table)\n"
              << "  minipascal all <file>         run every parser + semantics on one file\n"
              << "  minipascal test <file...>     cross-parser harness: assert RDP/LL(1)/LR agree\n"
              << "  minipascal outputs <file...>  write per-module artifacts to output/ (--out <dir>)\n"
              << "  minipascal grammar [bnf]      print the Original and Transformed grammars\n"
              << "  minipascal first-follow [bnf] print FIRST/FOLLOW for the Transformed grammar\n"
              << "  minipascal ll1-table [bnf]    print the LL(1) table and any conflicts\n"
              << "  minipascal slr-table [bnf]    print SLR(1) item sets, ACTION/GOTO, conflicts\n"
              << "  minipascal report-tables      dump all grammar/parse tables to docs/report/ (--out <dir>)\n\n"
              << "  Run with no arguments for an interactive menu.\n"
              << "  [bnf] defaults to " << kDefaultBnf << "\n";
}

void printTokenTable(const std::string& path, const std::vector<mp::Token>& tokens,
                     std::ostream& out) {
    out << "Token stream for " << path << ":\n\n";
    out << "  " << std::left
        << std::setw(5) << "LINE" << std::setw(5) << "COL"
        << std::setw(13) << "TYPE" << std::setw(16) << "LEXEME"
        << "TERMINAL\n";
    out << "  " << std::string(56, '-') << "\n";

    for (const mp::Token& token : tokens) {
        bool eof = token.type == mp::TokType::EndOfFile;
        out << "  " << std::right << std::setw(3) << token.line << "  "
            << std::setw(3) << token.col << "  " << std::left
            << std::setw(13) << mp::tokTypeName(token.type)
            << std::setw(16) << (eof ? "<eof>" : token.lexeme)
            << mp::grammarTerminal(token) << "\n";
    }

    int realTokens = tokens.empty() ? 0 : static_cast<int>(tokens.size()) - 1;
    out << "\n" << realTokens << " token(s) before end of file.\n\n";
}

bool openSource(const std::string& path, std::ifstream& in) {
    in.open(path, std::ios::binary);
    if (in) return true;
    std::cerr << "error: cannot open '" << path << "'\n";
    return false;
}

const char* requireFileArg(int argc, char** argv, const char* command,
                           const char* hint = "") {
    if (argc >= 3) return argv[2];
    std::cerr << "error: '" << command << "' needs a file argument" << hint << "\n";
    return nullptr;
}

bool parseOutputArgs(int argc, char** argv, std::string& outDir,
                     std::vector<std::string>* files) {
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--out") {
            if (i + 1 >= argc) {
                std::cerr << "error: --out needs a directory\n";
                return false;
            }
            outDir = argv[++i];
        } else if (files) {
            files->push_back(arg);
        }
    }
    return true;
}

int runLex(const std::string& path) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;

    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    std::vector<mp::Token> tokens = lexer.tokenize();

    printTokenTable(path, tokens, std::cout);
    reporter.printSummary(std::cout);
    return reporter.hasErrors() ? 1 : 0;
}

int runRdp(const std::string& path) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;

    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    mp::RecursiveDescentParser parser(lexer, reporter);
    std::unique_ptr<mp::Program> program = parser.parse();

    std::cout << "Recursive-descent parse of " << path << ":\n\n";
    if (program) {
        std::cout << "Abstract syntax tree:\n\n";
        mp::printAst(*program, std::cout);
        std::cout << "\n";
    } else {
        std::cout << "No AST produced.\n\n";
    }
    reporter.printSummary(std::cout);
    return reporter.hasErrors() ? 1 : 0;
}

int runDot(const std::string& path) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;

    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    mp::RecursiveDescentParser parser(lexer, reporter);
    std::unique_ptr<mp::Program> program = parser.parse();

    if (!program) {
        std::cerr << "error: no AST produced - parsing failed before DOT export\n";
        reporter.printSummary(std::cerr);
        return 1;
    }

    mp::SymbolTable symbols;
    mp::SemanticAnalyzer analyzer(symbols, reporter);
    analyzer.analyze(*program);

    mp::printAstDot(*program, std::cout);
    reporter.printSummary(std::cerr);
    return reporter.hasErrors() ? 1 : 0;
}

int runSemanticPipeline(const std::string& path, bool showAst, bool showTable) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;

    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    mp::RecursiveDescentParser parser(lexer, reporter);
    std::unique_ptr<mp::Program> program = parser.parse();

    if (!program) {
        std::cout << "No AST produced - parsing failed before analysis.\n\n";
        reporter.printSummary(std::cout);
        return reporter.hasErrors() ? 1 : 0;
    }

    mp::SymbolTable symbols;
    mp::SemanticAnalyzer analyzer(symbols, reporter);
    analyzer.analyze(*program);

    if (showAst) {
        std::cout << "Abstract syntax tree:\n\n";
        mp::printAst(*program, std::cout);
        std::cout << "\n";
    }
    if (showTable) {
        symbols.printAll(std::cout);
        std::cout << "\n";
    }
    reporter.printSummary(std::cout);
    return reporter.hasErrors() ? 1 : 0;
}

int runSymtab(const std::string& path) {
    std::cout << "Symbol table for " << path << ":\n\n";
    return runSemanticPipeline(path, false, true);
}

int runSemant(const std::string& path) {
    std::cout << "Semantic analysis of " << path << ":\n\n";
    return runSemanticPipeline(path, true, true);
}

int runPredictive(const std::string& path) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;
    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    std::vector<mp::Token> tokens = lexer.tokenize();
    try {
        mp::Grammar    original    = mp::Grammar::loadBNF(kDefaultBnf);
        mp::Grammar    transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        mp::LL1Table   ll1(transformed, firstFollow);
        std::cout << "Predictive LL(1) parse of " << path << ":\n\n";
        mp::PredictiveParser parser(tokens, transformed, ll1, reporter);
        parser.parse(std::cout);
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    reporter.printSummary(std::cout);
    return reporter.hasErrors() ? 1 : 0;
}

int runLR(const std::string& path) {
    std::ifstream in;
    if (!openSource(path, in)) return 2;
    mp::ErrorReporter reporter;
    mp::Lexer lexer(in, reporter);
    std::vector<mp::Token> tokens = lexer.tokenize();
    try {
        mp::Grammar  original = mp::Grammar::loadBNF(kDefaultBnf);
        mp::SLRTable slr(original);
        std::cout << "LR(SLR(1)) parse of " << path << ":\n\n";
        mp::LRParser parser(tokens, original, slr, reporter);
        parser.parse(std::cout);
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    reporter.printSummary(std::cout);
    return reporter.hasErrors() ? 1 : 0;
}

struct Verdicts {
    bool rdp = false, predictive = false, lr = false;
    bool agree() const { return rdp == predictive && predictive == lr; }
};

Verdicts crossCheck(const std::string& path, const mp::Grammar& transformed,
                    const mp::LL1Table& ll1, const mp::Grammar& original,
                    const mp::SLRTable& slr) {
    Verdicts verdicts;
    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        mp::RecursiveDescentParser parser(lexer, reporter);
        std::unique_ptr<mp::Program> program = parser.parse();
        verdicts.rdp = program != nullptr && reporter.count(mp::Phase::Syntactic) == 0;
    }
    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        std::vector<mp::Token> tokens = lexer.tokenize();
        std::ostringstream sink;
        {
            mp::ErrorReporter predictiveReporter;
            mp::PredictiveParser predictiveParser(tokens, transformed, ll1, predictiveReporter);
            verdicts.predictive = predictiveParser.parse(sink);
        }
        {
            mp::ErrorReporter lrReporter;
            mp::LRParser lrParser(tokens, original, slr, lrReporter);
            verdicts.lr = lrParser.parse(sink);
        }
    }
    return verdicts;
}

const char* verdictWord(bool accept) { return accept ? "accept" : "reject"; }

int runAll(const std::string& path) {
    std::ifstream probe;
    if (!openSource(path, probe)) return 2;
    probe.close();

    bool agree = true;
    try {
        mp::Grammar    original    = mp::Grammar::loadBNF(kDefaultBnf);
        mp::Grammar    transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        mp::LL1Table   ll1(transformed, firstFollow);
        mp::SLRTable   slr(original);

        Verdicts verdicts = crossCheck(path, transformed, ll1, original, slr);
        agree = verdicts.agree();

        std::cout << "Cross-parser agreement for " << path << ":\n\n"
                  << "  " << std::left << std::setw(12) << "RDP"
                  << std::setw(14) << "PREDICTIVE" << std::setw(8) << "LR" << "VERDICT\n"
                  << "  " << std::string(42, '-') << "\n"
                  << "  " << std::left << std::setw(12) << verdictWord(verdicts.rdp)
                  << std::setw(14) << verdictWord(verdicts.predictive)
                  << std::setw(8) << verdictWord(verdicts.lr)
                  << (agree ? "all three agree" : "*** DISAGREEMENT ***") << "\n\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }

    int result = runSemanticPipeline(path, true, true);
    return (agree && result == 0) ? 0 : 1;
}

int runHarness(const std::vector<std::string>& files) {
    if (files.empty()) {
        std::cerr << "error: 'test' needs at least one .pas file\n";
        return 2;
    }
    try {
        mp::Grammar    original    = mp::Grammar::loadBNF(kDefaultBnf);
        mp::Grammar    transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        mp::LL1Table   ll1(transformed, firstFollow);
        mp::SLRTable   slr(original);

        std::cout << "Cross-parser harness (RDP vs Predictive vs LR) over "
                  << files.size() << " sample(s):\n\n"
                  << "  " << std::left << std::setw(32) << "FILE"
                  << std::setw(10) << "RDP" << std::setw(12) << "PREDICTIVE"
                  << std::setw(8) << "LR" << "AGREE\n"
                  << "  " << std::string(68, '-') << "\n";

        int disagreements = 0;
        for (const std::string& file : files) {
            Verdicts verdicts = crossCheck(file, transformed, ll1, original, slr);
            if (!verdicts.agree()) ++disagreements;
            std::string name = file.size() > 31 ? "..." + file.substr(file.size() - 28) : file;
            std::cout << "  " << std::left << std::setw(32) << name
                      << std::setw(10) << verdictWord(verdicts.rdp)
                      << std::setw(12) << verdictWord(verdicts.predictive)
                      << std::setw(8) << verdictWord(verdicts.lr)
                      << (verdicts.agree() ? "yes" : "NO") << "\n";
        }
        std::cout << "\n";
        if (disagreements == 0)
            std::cout << "All parsers agree on every sample. Harness PASSED.\n";
        else
            std::cout << disagreements << " disagreement(s) found. Harness FAILED.\n";
        return disagreements == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}

namespace fs = std::filesystem;

std::string stemOf(const std::string& path) {
    std::string stem = fs::path(path).stem().string();
    return stem.empty() ? "out" : stem;
}

int writeOutputsFor(const std::string& path, const std::string& outDir,
                    const mp::Grammar& transformed, const mp::LL1Table& ll1,
                    const mp::Grammar& original, const mp::SLRTable& slr) {
    const std::string base = (fs::path(outDir) / stemOf(path)).string();
    int failed = 0;
    auto open = [&](const char* suffix, std::ofstream& file) -> bool {
        file.open(base + suffix, std::ios::binary);
        if (!file) { std::cerr << "  ! cannot write " << base << suffix << "\n"; ++failed; }
        return static_cast<bool>(file);
    };

    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        std::vector<mp::Token> tokens = lexer.tokenize();
        std::ofstream tokensFile;
        if (open(".tokens.txt", tokensFile)) { printTokenTable(path, tokens, tokensFile); reporter.printSummary(tokensFile); }
    }

    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        mp::RecursiveDescentParser parser(lexer, reporter);
        std::unique_ptr<mp::Program> program = parser.parse();

        if (program) {
            mp::SymbolTable symbols;
            mp::SemanticAnalyzer analyzer(symbols, reporter);
            analyzer.analyze(*program);

            std::ofstream astText;
            if (open(".ast.txt", astText))    { astText << "AST for " << path << ":\n\n"; mp::printAst(*program, astText); }
            std::ofstream astDot;
            if (open(".ast.dot", astDot))    mp::printAstDot(*program, astDot);
            std::ofstream symtabFile;
            if (open(".symtab.txt", symtabFile)) { symtabFile << "Symbol table for " << path << ":\n\n"; symbols.printAll(symtabFile); }
        } else {
            std::ofstream astText;
            if (open(".ast.txt", astText))    astText << "No AST produced - parsing failed for " << path << ".\n";
            std::ofstream astDot;
            if (open(".ast.dot", astDot))    astDot << "// no AST produced for " << path << "\n";
            std::ofstream symtabFile;
            if (open(".symtab.txt", symtabFile)) symtabFile << "No symbol table - parsing failed for " << path << ".\n";
        }
        std::ofstream semantFile;
        if (open(".semant.txt", semantFile))  { semantFile << "Semantic analysis of " << path << ":\n\n"; reporter.printSummary(semantFile); }
        std::ofstream errorsFile;
        if (open(".errors.txt", errorsFile))  { errorsFile << "Consolidated diagnostics for " << path << ":\n\n"; reporter.printSummary(errorsFile); }
    }

    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        std::vector<mp::Token> tokens = lexer.tokenize();
        std::ofstream traceFile;
        if (open(".ll1.trace.txt", traceFile)) {
            traceFile << "Predictive LL(1) parse of " << path << ":\n\n";
            mp::PredictiveParser predictiveParser(tokens, transformed, ll1, reporter);
            predictiveParser.parse(traceFile);
            traceFile << "\n";
            reporter.printSummary(traceFile);
        }
    }

    {
        std::ifstream in(path, std::ios::binary);
        mp::ErrorReporter reporter;
        mp::Lexer lexer(in, reporter);
        std::vector<mp::Token> tokens = lexer.tokenize();
        std::ofstream traceFile;
        if (open(".lr.trace.txt", traceFile)) {
            traceFile << "SLR(1) parse of " << path << ":\n\n";
            mp::LRParser lrParser(tokens, original, slr, reporter);
            lrParser.parse(traceFile);
            traceFile << "\n";
            reporter.printSummary(traceFile);
        }
    }
    return failed;
}

int runOutputs(const std::vector<std::string>& files, const std::string& outDir) {
    if (files.empty()) {
        std::cerr << "error: 'outputs' needs at least one .pas file\n";
        return 2;
    }
    std::error_code ec;
    fs::create_directories(outDir, ec);
    if (ec) {
        std::cerr << "error: cannot create output directory '" << outDir << "': "
                  << ec.message() << "\n";
        return 2;
    }
    try {
        mp::Grammar    original    = mp::Grammar::loadBNF(kDefaultBnf);
        mp::Grammar    transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        mp::LL1Table   ll1(transformed, firstFollow);
        mp::SLRTable   slr(original);

        std::cout << "Writing per-module artifacts to " << outDir << "/ for "
                  << files.size() << " sample(s):\n\n";
        int failed = 0;
        for (const std::string& file : files) {
            std::ifstream probe(file, std::ios::binary);
            if (!probe) { std::cerr << "  ! cannot open '" << file << "', skipping\n"; ++failed; continue; }
            probe.close();
            std::cout << "  " << file << " -> " << stemOf(file)
                      << ".{tokens,ast.txt,ast.dot,ll1.trace,lr.trace,symtab,semant,errors}.txt\n";
            failed += writeOutputsFor(file, outDir, transformed, ll1, original, slr);
        }
        std::cout << "\n"
                  << (failed == 0 ? "All artifacts written."
                                  : "Some artifacts failed; see messages above.")
                  << "\n";
        return failed == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}

int runGrammar(const std::string& bnfPath, std::ostream& out) {
    try {
        mp::Grammar original = mp::Grammar::loadBNF(bnfPath);
        out << "Original Grammar (" << bnfPath << "):\n"
            << "  start symbol : " << original.start() << "\n"
            << "  " << original.nonterminals().size() << " nonterminals, "
            << original.terminals().size() << " terminals, "
            << original.productions().size() << " productions\n\n";
        original.print(out);

        mp::Grammar transformed = mp::toTransformed(original);
        out << "\n\nTransformed Grammar (left recursion removed, left factored):\n"
            << "  " << transformed.nonterminals().size() << " nonterminals, "
            << transformed.terminals().size() << " terminals, "
            << transformed.productions().size() << " productions\n\n";
        transformed.print(out);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}

int runFirstFollow(const std::string& bnfPath, std::ostream& out) {
    try {
        mp::Grammar original = mp::Grammar::loadBNF(bnfPath);
        mp::Grammar transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        out << "FIRST / FOLLOW for the Transformed Grammar (" << bnfPath << "):\n"
            << "  (epsilon shown as " << "\xCE\xB5" << ", end-of-input as $)\n\n";
        firstFollow.print(out);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}

int runLL1(const std::string& bnfPath, std::ostream& out) {
    try {
        mp::Grammar original = mp::Grammar::loadBNF(bnfPath);
        mp::Grammar transformed = mp::toTransformed(original);
        mp::FirstFollow firstFollow(transformed);
        mp::LL1Table table(transformed, firstFollow);
        table.print(out);
        out << "\n";
        table.printConflicts(out);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}

int runSLR(const std::string& bnfPath, std::ostream& out) {
    try {
        mp::Grammar original = mp::Grammar::loadBNF(bnfPath);
        mp::SLRTable slr(original);
        slr.printItemSets(out);
        out << "\n";
        slr.printTables(out);
        out << "\n";
        slr.printConflicts(out);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}

int runReportTables(const std::string& outDir) {
    std::error_code ec;
    fs::create_directories(outDir, ec);
    if (ec) {
        std::cerr << "error: cannot create report directory '" << outDir << "': "
                  << ec.message() << "\n";
        return 2;
    }
    int failed = 0;
    auto dump = [&](const char* name, int (*fn)(const std::string&, std::ostream&)) {
        std::string path = (fs::path(outDir) / name).string();
        std::ofstream file(path, std::ios::binary);
        if (!file) { std::cerr << "  ! cannot write " << path << "\n"; ++failed; return; }
        if (fn(kDefaultBnf, file) != 0) { ++failed; return; }
        std::cout << "  " << name << "\n";
    };

    std::cout << "Writing grammar / parse-table dumps to " << outDir << "/:\n\n";
    dump("grammar.txt",      runGrammar);
    dump("first_follow.txt", runFirstFollow);
    dump("ll1_table.txt",    runLL1);
    dump("slr_table.txt",    runSLR);
    std::cout << "\n"
              << (failed == 0 ? "All tables written."
                              : "Some tables failed; see messages above.")
              << "\n";
    return failed == 0 ? 0 : 1;
}

bool readLine(std::string& line) {
    if (!std::getline(std::cin, line)) return false;
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end = line.find_last_not_of(" \t\r\n");
    line = (start == std::string::npos) ? "" : line.substr(start, end - start + 1);
    return true;
}

void printMenu(const std::string& srcPath) {
    std::cout << "\n=== Mini Pascal interactive menu ===\n"
              << "current source file: " << srcPath << "\n\n"
              << "   1  Lex             tokenize and print the token stream\n"
              << "   2  Parse + AST     recursive-descent parse and print the AST\n"
              << "   3  Predictive      LL(1) table-driven parse + trace\n"
              << "   4  LR parse        SLR(1) shift-reduce parse + trace\n"
              << "   5  Symbol table    build and dump the symbol table\n"
              << "   6  Semantic        type / semantic analysis\n"
              << "   7  Run all stages  parsers agree + AST + symbol table + semantics\n"
              << "   8  Grammar tables  dump grammar / FIRST-FOLLOW / LL(1) / SLR\n"
              << "   9  AST (DOT)       emit the AST as a Graphviz DOT digraph\n"
              << "  10  Generate output write per-module artifacts to output/\n"
              << "  11  Report tables   dump grammar / parse tables to docs/report/\n"
              << "  12  Change file     set the source .pas file\n"
              << "   0  Quit\n\n"
              << "choose> ";
}

void runGrammarDump() {
    std::cout << "\nGrammar / parse-table dump (" << kDefaultBnf << "):\n\n"
              << "  1  Original + Transformed grammar\n"
              << "  2  FIRST / FOLLOW sets\n"
              << "  3  LL(1) predictive table + conflicts\n"
              << "  4  SLR(1) item sets + ACTION/GOTO + conflicts\n"
              << "  0  Back\n\n"
              << "choose> ";
    std::string line;
    if (!readLine(line)) { std::cout << "\n"; return; }
    if      (line == "1") runGrammar(kDefaultBnf, std::cout);
    else if (line == "2") runFirstFollow(kDefaultBnf, std::cout);
    else if (line == "3") runLL1(kDefaultBnf, std::cout);
    else if (line == "4") runSLR(kDefaultBnf, std::cout);
    else if (line == "0" || line.empty()) return;
    else std::cout << "unknown choice '" << line << "'\n";
}

int runMenu() {
    std::string srcPath = "test/gcd.pas";
    for (;;) {
        printMenu(srcPath);

        std::string line;
        if (!readLine(line)) { std::cout << "\n"; break; }
        if (line.empty()) continue;

        if (line == "0") break;
        else if (line == "1") runLex(srcPath);
        else if (line == "2") runRdp(srcPath);
        else if (line == "3") runPredictive(srcPath);
        else if (line == "4") runLR(srcPath);
        else if (line == "5") runSymtab(srcPath);
        else if (line == "6") runSemant(srcPath);
        else if (line == "7") runAll(srcPath);
        else if (line == "8") runGrammarDump();
        else if (line == "9") runDot(srcPath);
        else if (line == "10") runOutputs({srcPath}, "output");
        else if (line == "11") runReportTables("docs/report");
        else if (line == "12") {
            std::cout << "new source file path> ";
            std::string newPath;
            if (!readLine(newPath)) { std::cout << "\n"; break; }
            if (!newPath.empty()) srcPath = newPath;
            else std::cout << "(unchanged)\n";
        }
        else
            std::cout << "unknown choice '" << line << "'\n";
    }
    std::cout << "bye.\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        printBanner();
        return runMenu();
    }

    std::string command = argv[1];
    if (command == "lex") {
        const char* file = requireFileArg(argc, argv, "lex");
        return file ? runLex(file) : 2;
    }
    if (command == "rdp") {
        const char* file = requireFileArg(argc, argv, "rdp");
        return file ? runRdp(file) : 2;
    }
    if (command == "dot") {
        const char* file = requireFileArg(argc, argv, "dot");
        return file ? runDot(file) : 2;
    }
    if (command == "symtab") {
        const char* file = requireFileArg(argc, argv, "symtab");
        return file ? runSymtab(file) : 2;
    }
    if (command == "semant") {
        const char* file = requireFileArg(argc, argv, "semant");
        return file ? runSemant(file) : 2;
    }
    if (command == "ll1") {
        const char* file = requireFileArg(argc, argv, "ll1", " (use 'll1-table' for the table)");
        return file ? runPredictive(file) : 2;
    }
    if (command == "lr") {
        const char* file = requireFileArg(argc, argv, "lr", " (use 'slr-table' for the table)");
        return file ? runLR(file) : 2;
    }
    if (command == "all") {
        const char* file = requireFileArg(argc, argv, "all");
        return file ? runAll(file) : 2;
    }
    if (command == "test") {
        std::vector<std::string> files(argv + 2, argv + argc);
        return runHarness(files);
    }
    if (command == "outputs") {
        std::vector<std::string> files;
        std::string outDir = "output";
        if (!parseOutputArgs(argc, argv, outDir, &files)) return 2;
        return runOutputs(files, outDir);
    }
    if (command == "grammar") {
        return runGrammar(argc >= 3 ? argv[2] : kDefaultBnf, std::cout);
    }
    if (command == "first-follow") {
        return runFirstFollow(argc >= 3 ? argv[2] : kDefaultBnf, std::cout);
    }
    if (command == "ll1-table") {
        return runLL1(argc >= 3 ? argv[2] : kDefaultBnf, std::cout);
    }
    if (command == "slr-table") {
        return runSLR(argc >= 3 ? argv[2] : kDefaultBnf, std::cout);
    }
    if (command == "report-tables") {
        std::string outDir = "docs/report";
        if (!parseOutputArgs(argc, argv, outDir, nullptr)) return 2;
        return runReportTables(outDir);
    }
    if (command == "help" || command == "-h" || command == "--help") {
        printBanner();
        printUsage();
        return 0;
    }

    printBanner();
    std::cout << "Unknown command '" << command << "'.\n\n";
    printUsage();
    return 2;
}
