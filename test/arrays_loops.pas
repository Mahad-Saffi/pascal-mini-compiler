{ Arrays + loops + nested procedure. Valid program.
  Exercises: array declaration, array-element assignment (arrays are
  write-only in this subset - they can never be read in an expression),
  while loops, nested compound statements, if/else (mandatory else),
  a procedure with a parameter, and access to a global from inside it.
  Each declaration group repeats 'var', as the grammar requires. }

program arrayfill (output);

var
   a : array [ 1 .. 10 ] of integer;

var
   i : integer;

procedure init (n : integer);

var
   k : integer;

begin
   k := n;
   while k <= 10 do
   begin
      if k <= 5 then
         a[k] := k
      else
         a[k] := 0;
      k := k + 1
   end
end;

begin
   i := 1;
   while i <= 10 do
   begin
      a[i] := 0;
      i := i + 1
   end;
   init(1)
end.
