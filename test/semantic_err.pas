{ Syntactically valid - all three parsers accept it - but it violates
  several static-semantic rules, to drive the semantic analyser.
  Injected errors:
    1. duplicate declaration of 'x' in the same scope
    2. real -> integer assignment (x := r)
    3. use of an undeclared identifier (undeclared)
    4. call-arity mismatch: p takes 1 argument, called with 2
    5. wrong argument type: real passed where integer is expected }

program semanticerr (output);

var
   x : integer;

var
   x : real;

var
   r : real;

procedure p (n : integer);

begin
   n := n + 1
end;

begin
   x := r;
   x := undeclared;
   p(1, 2);
   p(r)
end.
