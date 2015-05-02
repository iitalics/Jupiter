# built in functions:
#           + : (Int, Int) -> Int
#           - : (Int) -> Int
#          == : (Int, Int) -> Bool
#       print : (String) -> ()
#       print : (Int) -> ()
#       print : (Bool) -> ()
#     println : () -> ()


# functions are declared with 'func'

func twice (x) { x + x }


# types are declared after variable names and are optional

func zero? (x : Int) { x == 0 }


# functions are called like in C and other C-like languages

func foo (x) {
	if zero?(x) {
		println("x is zero!");
	} else {
		println("x isn't zero.");
	}
}


# it doesn't matter when you declare a function

func println (x) { print(x); println(); }


# the 'main' function is executed first

func main () {
	foo(twice(0));
	foo(twice(5));
}