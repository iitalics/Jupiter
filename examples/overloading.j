# example of overload resolution
import std/stdlib

func dontDoThis (x : Int, y : \a) {
	println(":(");
}
func dontDoThis (x : \a, y : Int) {
	println(":)");
}

func thing (x : [\a]) {
	println("List of anything!");
}
func thing (x : [Int]) {
	println("Just list of Int");
}

pub func main () {
	# dontDoThis(0, 0);      ambiguous call

	thing(["Not an Int!"]);
	thing([1, 2]);
}