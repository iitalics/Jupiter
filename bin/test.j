func println (x) { print(x); println(); }
func - (x, y) { x + -(y) }

func sum (n) {
	n + sum(n - 1)
}

func main () {
	println(sum(10))
}