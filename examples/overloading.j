# example of overload resolution


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

func main () {
	# dontDoThis(0, 0);      ambiguous call

	thing(cons("Not an Int!", nil()));
	thing(cons(1, cons(2, nil())));
}