# simple recursion example

func println (x) { print(x); println(); }
func println (x, y) { print(x); print(y); println(); }
func - (x, y) { x + -(y) }

func sum (n) {
	if n < 1 then
		0
	else
		n + sum(n - 1)
}

func main () {
	println("please don't free ", "me!");
	println("phew, they're gone!");
}