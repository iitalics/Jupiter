# lambdas in action

func twice (fn : () -> _) {
	fn();
	fn();
}

func main () {
	let msg = "Foo bar baz.";
	
	func () {
		println(msg);
	}.twice();
}