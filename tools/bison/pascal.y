/* Bison grammar for the Subset of Pascal - the parser half of the cross-check
 * oracle. It is a production-for-production transcription of the Original Grammar
 * in docs/grammar/original.bnf (the same left-recursive grammar our hand-built
 * SLR(1) parser uses). Bison builds an LALR(1) automaton from it; running the
 * generated parser on a .pas file yields an accept/reject verdict that we compare
 * against our SLR(1) parser's verdict (see tools/bison/cross_check.sh).
 *
 * Why this is a meaningful check: LALR(1) is at least as powerful as SLR(1), so a
 * grammar that is conflict-free under our SLR construction must also be conflict-
 * free for Bison, and both must accept/reject exactly the same strings. Any
 * disagreement would expose a bug in our table construction - that is the bonus
 * deliverable "hand-built SLR table cross-checked against Bison's output".
 *
 * There are no semantic actions and no AST: the verdict is yyparse()'s return code.
 */
%{
#include <stdio.h>
#include <stdlib.h>
int  yylex(void);
void yyerror(const char* s);
%}

/* Token classes the scanner returns; single-character tokens ( ) [ ] ; , : . and
 * the operators + - * / < > = are passed through as their own char codes. */
%token PROGRAM VAR ARRAY OF INTEGER REAL FUNCTION PROCEDURE
%token BEGINT ENDT IFT THEN ELSE WHILE DO NOT OR DIV MOD AND
%token ID NUM ASSIGNOP DOTDOT LE GE NE

%start program
%%

program
    : PROGRAM ID '(' identifier_list ')' ';' declarations subprogram_declarations compound_statement '.'
    ;

identifier_list
    : ID
    | identifier_list ',' ID
    ;

declarations
    : declarations VAR identifier_list ':' type ';'
    | /* epsilon */
    ;

type
    : standard_type
    | ARRAY '[' NUM DOTDOT NUM ']' OF standard_type
    ;

standard_type
    : INTEGER
    | REAL
    ;

subprogram_declarations
    : subprogram_declarations subprogram_declaration ';'
    | /* epsilon */
    ;

subprogram_declaration
    : subprogram_head declarations compound_statement
    ;

subprogram_head
    : FUNCTION ID arguments ':' standard_type ';'
    | PROCEDURE ID arguments ';'
    ;

arguments
    : '(' parameter_list ')'
    | /* epsilon */
    ;

parameter_list
    : identifier_list ':' type
    | parameter_list ';' identifier_list ':' type
    ;

compound_statement
    : BEGINT optional_statements ENDT
    ;

optional_statements
    : statement_list
    | /* epsilon */
    ;

statement_list
    : statement
    | statement_list ';' statement
    ;

statement
    : variable ASSIGNOP expression
    | procedure_statement
    | compound_statement
    | IFT expression THEN statement ELSE statement
    | WHILE expression DO statement
    ;

variable
    : ID
    | ID '[' expression ']'
    ;

procedure_statement
    : ID
    | ID '(' expression_list ')'
    ;

expression_list
    : expression
    | expression_list ',' expression
    ;

expression
    : simple_expression
    | simple_expression relop simple_expression
    ;

simple_expression
    : term
    | sign term
    | simple_expression addop term
    ;

term
    : factor
    | term mulop factor
    ;

factor
    : ID
    | ID '(' expression_list ')'
    | NUM
    | '(' expression ')'
    | NOT factor
    ;

sign  : '+' | '-' ;
addop : '+' | '-' | OR ;
mulop : '*' | '/' | DIV | MOD | AND ;
relop : '=' | NE | '<' | LE | GE | '>' ;

%%

/* Quiet by default: the verdict is the exit code, and the cross-check script wants
 * a single ACCEPT/REJECT line, not a wall of parser diagnostics. Set ORACLE_VERBOSE
 * in the environment to see Bison's error message for debugging. */
void yyerror(const char* s) {
    if (getenv("ORACLE_VERBOSE")) fprintf(stderr, "syntax error: %s\n", s);
}

int main(int argc, char** argv) {
    extern FILE* yyin;
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { fprintf(stderr, "oracle: cannot open %s\n", argv[1]); return 2; }
    }
    int rc = yyparse();
    printf(rc == 0 ? "ACCEPT\n" : "REJECT\n");
    return rc == 0 ? 0 : 1;
}
