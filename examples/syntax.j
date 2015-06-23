# showcases most all features of the syntax
#  useful for codegen testing as well as 
#  testing syntax highlighting
import std/stdlib

pub func main () {
	# variables
	# types are optional in most cases
	let w : Str = "Hello, world";
	let x = 3;      # x : Int
	let y = 4.0;    # y : Real
	let z = true;   # z : Bool

	# function calls
	# print(x) is overloaded with basic types (Str, Int, etc...)
	# println(x, y) calls print(x); print(y)
	# each individual overload is determined at compile time
	println(w);
	println("x = ", x);
	println("y = ", y);

	# conditions
	if z {
		println("z is true!");
	}

	# loops
	print(x);
	loop x < 6 {
		x = x.succ;
		print(", ", x);
	}
	println();

	# lambdas
	let fn1 = func () { println("function one!"); };
	let fn2 = \x : Int -> println("function two: ", x);
	
	fn1.twice();
	fn2.twice(3);

	# using the custom types below
	let pt = point(4, 5);
	println("pt = ", pt.str);
	println(pair(3, true).str);

	let n = intNum(3);
	println(n.str);
	n = realNum(4.0);
	println(n.str);
}



#  function types are declared as
#    (args...) -> ret
#   or
#    Fn(args..., ret)
#
#  list types are declared as
#    [type]
#   or
#    List(type)
#
#  tuple types are declared as
#    (types...)
#   or
#    Tuple(types...)
#
#  poly types are declared as
#    \<name>

# user defined type
# generates function 'point' : (Int, Int) -> Point
type Point = point(x : Int, y : Int)

# user defined types can be generic
type Pair(\a, \b) = pair(left : \a, right : \b)

# all types are actually sum types, for instance
type Number =
	intNum(intVal : Int),
	realNum(realVal : Real)


# overloading the 'str' function
func str (pt : Point) {
	pt.x.str ++ ", " ++ pt.y.str
}
func str (p : Pair(\a, \b)) {
	p.left.str ++ "/" ++ p.right.str
}
func str (num : Number) {
	if num.intNum? {
		"int: " ++ num.intVal.str
	} else {
		"real: " ++ num.realVal.str
	}
}

func twice (fn : () -> _) {
	fn();
	fn();
}
# poly type '\a' for generic functions
func twice (fn : (\a) -> _, arg : \a) {
	fn(arg);
	fn(arg);
}