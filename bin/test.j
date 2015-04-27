# simple recursion example

func println (x) { print(x); println(); }
func - (x, y) { x + -(y) }

func sum (n) {
	if n < 1 then
		0
	else
		n + sum(n - 1)
}

func main () {
	println(sum(10)) # -> 55
}