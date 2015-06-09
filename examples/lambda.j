# lambdas in action!

func repeat (fn, n) {
	if n > 0 {
		fn();
		fn.repeat(n - 1);
	}
}

func main () {
	let x = 1;

	func () {
		println("x = ", x);
		x = x.succ;
	}.repeat(10);
}
