func foldl (f : (\a, \b) -> \a, z : \a, a : [\b]) { z }

func what (x, y) { x }

func main () {
	foldl(what, nil(), nil())
}