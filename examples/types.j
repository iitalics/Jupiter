# type declaration examples

type [\a] =
	cons(hd : \a, tl : List(\a)),
	nil()

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
func println (x) { print(x); println(); }


func main () {
	let lst = 1 :: 2 :: 3 :: 4 :: nil();

	println(lst);
}