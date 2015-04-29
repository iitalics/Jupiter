# uses internal operations ^make and ^get 
#  to artificially create lists

func println (x) { print(x); println() }
func println (x, y) { print(x); print(y); println() }

func :: (hd : \t, tl : List(\t)) {
	^make (Bool, \t, List(\t)) -> List(\t) "::"
		(true, hd, tl)
}
func nil () {
	^make (Bool) -> List(_) "nil"
		(false)
}
func ::? (x : List(\t)) { ^get Bool 0 x }
func hd (x : List(\t))  { ^get \t 1 x }
func tl (x : List(\t))  { ^get List(\t) 2 x }


func print (a : List(\t)) {
	print("[");
	if ::?(a) {
		print(hd(a));
		printList(tl(a));
	};
	print("]");
}
func printList (a) {
	if ::?(a) {
		print(", ");
		print(hd(a));
		printList(tl(a));
	}
}

func main () {
	let lst = { 1 :: 2 :: 3 :: 4 :: nil() };

	println("test list: ", lst);
}