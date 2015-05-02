# built in functions:
#           + : (Int, Int) -> Int       add
#           * : (Int, Int) -> Int       multiply
#           / : (Int, Int) -> Int       divide
#           - : (Int) -> Int            negate
#          == : (Int, Int) -> Bool      test equality
#       print : (String) -> ()          print out
#       print : (Int) -> ()             ...
#       print : (Bool) -> ()            ...
#     println : () -> ()                print a line break


# functions are declared with 'func'

func twice (x) { x + x }


# types are declared after variable names and are optional
# although this is the only type declaration you will see here,
#  every expression and variable has a type like in C or Java,
#  it is just automatically deduced by the compiler

func zero? (x : Int) { x == 0 }


# functions are called like in C and other C-like languages

func foo (x) {
	if zero?(x) {
		println("x is zero!");
	} else {
		println("x isn't zero.");
	}
}


# it doesn't matter the order that you declare functions

func println (x) { print(x); println(); }
func println (x, y) { print(x); print(y); println(); }
func - (x, y) { x + -(y) }


# declare local variables with 'let'

func sum1 (n) {
	let m = n + 1;
	m * n / 2;
}


# 'if' can either take the form
#   if <condition> { <code> } else { <code> }
# or
#   if <condition> then <expression> else <expression>

func sum2 (n) {
	if n == 0 then
		0
	else
		n + sum2(n - 1)
}


# the 'main' function is executed first
# 'if' is an expression, not a statement, so it
#  can exists where any expression does

func main () {
	let max = 10;
	let result1 = sum1(max);
	let result2 = sum2(max);

	println(
		if result1 == result2 then
			"it works!"
		else
			"it doesn't work :c"); 
}