### jupiter standard library for version 0.0.10


## standard library internal calls

# math & logic functions
pub func -  (x : Int)            { ^call (Int) -> Int          "juStd_negInt"  (x) }
pub func +  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_addInt"  (x, y) }
pub func *  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_mulInt"  (x, y) }
pub func %  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_modInt"  (x, y) }
pub func /  (x : Int, y : Int)   { ^call (Int, Int) -> Int     "juStd_divInt"  (x, y) }
pub func <  (x : Int, y : Int)   { ^call (Int, Int) -> Bool    "juStd_ltInt"   (x, y) }
pub func == (x : Int, y : Int)   { ^call (Int, Int) -> Bool    "juStd_eqInt"   (x, y) }
pub func succ (x : Int)          { ^call (Int) -> Int          "juStd_succInt" (x) }
pub func pred (x : Int)          { ^call (Int) -> Int          "juStd_predInt" (x) }
pub func real (x : Int)          { ^call (Int) -> Real         "juStd_realInt" (x) }
pub func -  (x : Real)           { ^call (Real) -> Real        "juStd_negReal"  (x) }
pub func +  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_addReal"  (x, y) }
pub func *  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_mulReal"  (x, y) }
pub func /  (x : Real, y : Real) { ^call (Real, Real) -> Real  "juStd_divReal"  (x, y) }
pub func <  (x : Real, y : Real) { ^call (Real, Real) -> Bool  "juStd_ltReal"   (x, y) }
pub func == (x : Real, y : Real) { ^call (Real, Real) -> Bool  "juStd_eqReal"   (x, y) }
pub func succ (x : Real)         { ^call (Real) -> Real        "juStd_succReal" (x) }
pub func pred (x : Real)         { ^call (Real) -> Real        "juStd_predReal" (x) }
pub func recip (x : Real)        { ^call (Real) -> Real        "juStd_recipReal" (x) }
pub func int (x : Real)          { ^call (Real) -> Int         "juStd_intReal" (x) }
pub func ! (x : Bool)            { ^call (Bool) -> Bool        "juStd_notBool" (x) }
pub func + (x : Bool, y : Bool)  { ^call (Bool, Bool) -> Bool  "juStd_addBool" (x) }
pub func * (x : Bool, y : Bool)  { ^call (Bool, Bool) -> Bool  "juStd_mulBool" (x) }

# string functions
pub func str (x : Int)  { ^call (Int) -> Str  "juStd_strInt"  (x) }
pub func str (x : Bool) { ^call (Bool) -> Str "juStd_strBool" (x) }
pub func str (x : Real) { ^call (Real) -> Str "juStd_strReal" (x) }
pub func len (x : Str)  { ^call (Str) -> Int  "juStd_lenStr"  (x) }
pub func ++ (x : Str, y : Str) { ^call (Str, Str) -> Str "juStd_appStrStr" (x, y) }
pub func str (x, y) { str(x) ++ str(y) }

# io functions
pub func println ()         { ^call () -> ()       "juStd_println"     () }
pub func print (x : Str)    { ^call (Str) -> ()    "juStd_printStr"    (x) }
pub func print (x : Int)    { ^call (Int) -> ()    "juStd_printInt"    (x) }
pub func print (x : Bool)   { ^call (Bool) -> ()   "juStd_printBool"   (x) }
pub func print (x : Real)   { ^call (Real) -> ()   "juStd_printReal"   (x) }






# types
type List(\a) =
	cons(hd : \a, tl : [\a]), nil()
type Opt(\a) =
	some(val : \a), none()


# list comp
pub func :: (x : \a, xs : [\a]) { cons(x, xs) }
pub func foldl (list : [\a], z : \b, fn : (\b, \a) -> \b) {
	if list.nil? then
		z
	else
		foldl(list.tl, fn(z, list.hd), fn)
}
pub func foldr (list : [\a], z : \b, fn : (\a, \b) -> \b) {
	if list.nil? then
		z
	else
		fn(list.hd, foldr(list.tl, z, fn))
}
pub func rev (list : [\a]) {
	list.foldl([], \ys, x -> x :: ys)
}
pub func len (list : [\a]) {
	list.foldl(0, \n, x -> n.succ)
}
pub func map (list : [\a], fn : (\a) -> \b) {
	if list.nil? then
		[]
	else
		fn(list.hd) :: list.tl.map(fn)
}
pub func filter (list : [\a], fn : (\a) -> Bool) {
	if list.nil? then
		[]
	else if fn(list.hd) then
		list.hd :: list.tl.filter(fn)
	else
		list.tl.filter(fn)
}


# option monad
pub func == (a : Opt(\a), b : Opt(\a)) {
	if a.some? then
		if b.some? then
			a.val == b.val
		else
			false
	else
		!(b.some?)
}
pub func >> (x : Opt(\a), fn : (\a) -> Opt(\b)) {
	if x.some? then
		fn(x.val)
	else
		none()
}
pub func default (x : Opt(\a), def : \a) {
	if x.some? then x.val else def
}
pub func default (x : Opt(\a), fn : () -> \a) {
	if x.some? then x.val else fn()
}



# constructor identities
pub func bool (x : Bool) { x }
pub func real (x : Real) { x }
pub func int (x : Int)   { x }
pub func str (x : Str)   { x }
pub func list (x : [_])  { x }

# generic io functions
pub func print (x, y)      { print(x); print(y); }
pub func print (x, y, z)   { print(x); print(y); print(z); }
pub func println (x)       { print(x); println(); }
pub func println (x, y)    { print(x); print(y); println(); }
pub func println (x, y, z) { print(x); print(y); print(z); println(); }

# generic logic functions
pub func > (x : \a, y : \a)  { <(y, x) }
pub func <= (x : \a, y : \a) { if <(x, y) then true else ==(x, y) }
pub func >= (x : \a, y : \a) { if <(y, x) then true else ==(y, x) }
pub func != (x : \a, y : \a) { !(==(x, y)) }
pub func - (x : \a, y : \a)  { +(x, -(y)) }
