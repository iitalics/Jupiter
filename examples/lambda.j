# lambdas in action

func twice (fn : () -> _) { fn(); fn(); }

func main () {
	func () { 
		println("Hello, world!");
	}.twice();
}