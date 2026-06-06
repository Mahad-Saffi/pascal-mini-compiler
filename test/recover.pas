{ recover.pas - deliberately broken in several independent places to exercise
  panic-mode error recovery in all three parsers. A correct parser must report
  more than the first error (resynchronise and keep checking) and still arrive at
  a REJECT verdict. The cross-parser harness asserts the three verdicts agree.

  Planted syntax errors (each at a clean resynchronisation point):
    1. missing ';' ending the first var-declaration, before the next 'var'
    2. missing right operand of '+'    ->  b := a + ;
    3. missing right operand of 'mod'  ->  c := b mod ;
    4. 'if' with no mandatory 'else'   ->  if a < b then a := b

  Note: the program ends with the mandatory terminating dot ("end."), and arrays
  are write-only, so none of those features appear here. }

program recover(output);

var a, b : integer
var c : integer;

begin
    a := 1;
    b := a + ;
    c := b mod ;
    if a < b then
        a := b
end.
