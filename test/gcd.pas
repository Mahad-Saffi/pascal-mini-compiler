{ Euclid's GCD - the Appendix A example. Valid program.
  Exercises: program header params, var declaration, while, if/else
  (mandatory else), read/write built-ins, recursion-free iteration.
  The program ends with the mandatory terminating dot ("end."), as in
  standard Pascal and Appendix A Fig A.1. }

program gcd (input, output);

var
   x, y : integer;

begin
   read(x);
   read(y);
   while x <> y do
      if x > y then
         x := x - y
      else
         y := y - x;
   write(x)
end.
