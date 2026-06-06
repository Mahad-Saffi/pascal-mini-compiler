{ Deliberately malformed - drives parser error recovery and the
  end-of-run error summary. Each parser should report several distinct
  syntax errors (not just the first) and resynchronise after each.
  Injected errors:
    1. missing ';' after the var declaration (before 'begin')
    2. missing ';' between the first two statements
    3. '+' with no right operand
    4. an 'if' with no 'else' (else is mandatory in this subset) }

program syntaxerr (output);

var
   x, y : integer

begin
   x := 1
   y := x + ;
   if x > y then
      x := y;
   write(x)
end.
