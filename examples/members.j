# example showing the "member" expression




# { foo.bar }         eqv.   { bar(foo) }
# { foo.bar(x, y) }   eqv.   { bar(foo, x, y) }


func main () {
	let foo = cons(2, nil());

	println(foo.hd);
}