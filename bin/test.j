# uses internal operations ^make and ^get 
#  to artificially create tuples


func println (x) { print(x); println() }
func println (x, y) { print(x); print(y); println() }

func pair (x : \a, y : \b) {
	^make (\a, \b) -> (\a, \b) "pair"
		(x, y)
}
func fst (x : (\a, \b)) { ^get \a 0 x }
func snd (x : (\a, \b)) { ^get \b 1 x }


func main () {
	let obj = pair(3, "Hello");

	println(fst(obj));
	println(snd(obj));
}