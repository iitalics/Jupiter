# type declaration examples

func print (lst : [\a]) {
	if cons?(lst) {
		let x = hd(lst);
		let xs = tl(lst);

		print(x);
		if cons?(xs) {
			print(", ");
			print(xs);
		}
	}
}

func :: (x : \a, xs : [\a]) { cons(x, xs) }

func main () {
	let lst = 1 :: 2 :: 3 :: 4 :: nil();

	println(lst);
}