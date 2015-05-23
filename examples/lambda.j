# lambdas in action!

func each (lst : [\a], cb : (\a) -> _) {
	if lst.cons? {
		cb(lst.hd);
		lst.tl.each(cb);
	}
}

func print (lst : [\a]) {
	print("[");

	if !(lst.nil?) {
		print(lst.hd);
		lst.tl.each(\x ->
			print(", ", x))
	}

	print("]");
}

func :: (x, xs) { cons(x, xs) }

func main () {
	let lst = 1 :: 2 :: 3 :: 4 :: 5 :: nil();

	println("lst = ", lst);
}
