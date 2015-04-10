func foldl (f : (\u, \t) -> \u, z : \u, a : [\t]) {
	if nil?(a) {
		z
	} else {
		foldl(f, f(z, hd(a)), tl(a))
	}
}