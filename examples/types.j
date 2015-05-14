# type declaration examples

func print (lst : [\a]) {
	if lst.cons? {
		let x = lst.hd;
		let xs = lst.tl;

		print(x);
		if xs.cons? {
			print(", ", xs);
		}
	}
}

func :: (x : \a, xs : [\a]) { cons(x, xs) }

func main () {
	let lst = 1 :: 2 :: 3 :: 4 :: nil();

	println(lst);
}