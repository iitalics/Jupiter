# type declaration examples

# simple type
type Point =
	point(x : Int, y : Int)

# generates functions
#	point : (Int, Int) -> Point
#	point? : (Point) -> Bool
#	x : (Point) -> Int
#	y : (Point) -> Int


# generic type
type List(\a) =
	cons(hd : \a, tl : List(\a)),
	nil()

# generates functions
#	cons : (\a, List(\a)) -> List(\a)
#	nil : () -> List(\a)
#	cons? : (List(\a)) -> Bool
#	nil? : (List(\a)) -> Bool
#	hd : (List(\a)) -> \a
#	tl : (List(\a)) -> List(\a)


func println (x) { print(x); println() }

func main () {
	let lst = cons(3, nil());

	println(hd(lst));
}