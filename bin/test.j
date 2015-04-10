func foldl (f : (\a, \b) -> \a, z : \a, a : [\b]) {
	if nil?(a) {
		z
	} else {
		foldl(f, f(z, hd(a)), tl(a))
	}
}