func println (x) { print(x); println(); }

func sum (n) {
	1 + sum(n + 1)
}

func main () {
	println(sum(10))
}