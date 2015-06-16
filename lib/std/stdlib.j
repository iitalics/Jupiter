### jupiter standard library for version 0.0.10


## standard library internal calls

# math & logic functions
func -  (x : Int)            { ^call (Int) -> Int          "juStd_negInt"  (x) }
func +  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_addInt"  (x, y) }
func *  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_mulInt"  (x, y) }
func %  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_modInt"  (x, y) }
func /  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_divInt"  (x, y) }
func <  (x : Int, y : Int)   { ^call (Int, Int) -> Bool    "juStd_ltInt"   (x, y) }
func == (x : Int, y : Int)   { ^call (Int, Int) -> Bool    "juStd_eqInt"   (x, y) }
func succ (x : Int)          { ^call (Int) -> Int          "juStd_succInt" (x) }
func pred (x : Int)          { ^call (Int) -> Int          "juStd_predInt" (x) }
func real (x : Int)          { ^call (Int) -> Real         "juStd_realInt" (x) }
func -  (x : Real)           { ^call (Real) -> Real        "juStd_negReal"  (x) }
func +  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_addReal"  (x, y) }
func *  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_mulReal"  (x, y) }
func /  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_divReal"  (x, y) }
func <  (x : Real, y : Real) { ^call (Real, Real) -> Bool  "juStd_ltReal"   (x, y) }
func == (x : Real, y : Real) { ^call (Real, Real) -> Bool  "juStd_eqReal"   (x, y) }
func succ (x : Real)         { ^call (Real) -> Real        "juStd_succReal" (x) }
func pred (x : Real)         { ^call (Real) -> Real        "juStd_predReal" (x) }
func recip (x : Real)        { ^call (Real) -> Real        "juStd_recipReal" (x) }
func int (x : Real)          { ^call (Real) -> Int         "juStd_intReal" (x) }
func ! (x : Bool)            { ^call (Bool) -> Bool        "juStd_notBool" (x) }
func + (x : Bool, y : Bool)  { ^call (Bool, Bool) -> Bool  "juStd_addBool" (x) }
func * (x : Bool, y : Bool)  { ^call (Bool, Bool) -> Bool  "juStd_mulBool" (x) }

# string functions
func str (x : Int)  { ^call (Int) -> Str  "juStd_strInt"  (x) }
func str (x : Bool) { ^call (Bool) -> Str "juStd_strBool" (x) }
func str (x : Real) { ^call (Real) -> Str "juStd_strReal" (x) }
func len (x : Str)  { ^call (Str) -> Int  "juStd_lenStr"  (x) }
func ++ (x : Str, y : Str) { ^call (Str, Str) -> Str "juStd_appStrStr" (x, y) }
func str (x, y) { str(x) ++ str(y) }

# io functions
func println ()         { ^call () -> ()       "juStd_println"     () }
func print (x : Str)    { ^call (Str) -> ()    "juStd_printStr"    (x) }
func print (x : Int)    { ^call (Int) -> ()    "juStd_printInt"    (x) }
func print (x : Bool)   { ^call (Bool) -> ()   "juStd_printBool"   (x) }
func print (x : Real)   { ^call (Real) -> ()   "juStd_printReal"   (x) }






# types
type List(\a) =
	cons(hd : \a, tl : [\a]), nil()
type Opt(\a) =
	some(val : \a), none()


# list comp
func :: (x : \a, xs : [\a]) { cons(x, xs) }
func foldl (list : [\a], z : \b, fn : (\b, \a) -> \b) {
	if list.nil? then
		z
	else
		foldl(list.tl, fn(z, list.hd), fn)
}
func foldr (list : [\a], z : \b, fn : (\a, \b) -> \b) {
	if list.nil? then
		z
	else
		fn(list.hd, foldr(list.tl, z, fn))
}
func rev (list : [\a]) {
	list.foldl([], \ys, x -> x :: ys)
}
func len (list : [\a]) {
	list.foldl(0, \n, x -> n.succ)
}
func map (list : [\a], fn : (\a) -> \b) {
	if list.nil? then
		[]
	else
		fn(list.hd) :: list.tl.map(fn)
}
func filter (list : [\a], fn : (\a) -> Bool) {
	if list.nil? then
		[]
	else if fn(list.hd) then
		list.hd :: list.tl.filter(fn)
	else
		list.tl.filter(fn)
}


# option monad
func == (a : Opt(\a), b : Opt(\a)) {
	if a.some? then
		if b.some? then
			a.val == b.val
		else
			false
	else
		!(b.some?)
}
func >> (x : Opt(\a), fn : (\a) -> Opt(\b)) {
	if x.some? then
		fn(x.val)
	else
		none()
}
func default (x : Opt(\a), def : \a) {
	if x.some? then x.val else def
}
func default (x : Opt(\a), fn : () -> \a) {
	if x.some? then x.val else fn()
}



# constructor identities
func bool (x : Bool) { x }
func real (x : Real) { x }
func int (x : Int)   { x }
func str (x : Str)   { x }
func list (x : [_])  { x }

# generic io functions
func print (x, y)      { print(x); print(y); }
func print (x, y, z)   { print(x); print(y); print(z); }
func println (x)       { print(x); println(); }
func println (x, y)    { print(x); print(y); println(); }
func println (x, y, z) { print(x); print(y); print(z); println(); }

# generic logic functions
func > (x : \a, y : \a)  { <(y, x) }
func <= (x : \a, y : \a) { if <(x, y) then true else ==(x, y) }
func >= (x : \a, y : \a) { if <(y, x) then true else ==(y, x) }
func != (x : \a, y : \a) { !(==(x, y)) }
func - (x : \a, y : \a)  { +(x, -(y)) }
