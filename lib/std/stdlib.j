### jupiter standard library for version 0.0.10


## standard library internal calls

# math & logic functions
func !  (x : Bool)         { ^call (Bool) -> Bool     "juStd_notBool" (x) }
func -  (x : Int)          { ^call (Int) -> Int       "juStd_negInt"  (x) }
func +  (x : Int, y : Int) { ^call (Int, Int) -> Int  "juStd_addInt"  (x, y) }
func *  (x : Int, y : Int) { ^call (Int, Int) -> Int  "juStd_mulInt"  (x, y) }
func /  (x : Int, y : Int) { ^call (Int, Int) -> Int  "juStd_divInt"  (x, y) }
func <  (x : Int, y : Int) { ^call (Int, Int) -> Bool "juStd_ltInt"   (x, y) }
func == (x : Int, y : Int) { ^call (Int, Int) -> Bool "juStd_eqInt"   (x, y) }
func succ (x : Int)        { ^call (Int) -> Int       "juStd_succInt" (x) }
func pred (x : Int)        { ^call (Int) -> Int       "juStd_predInt" (x) }

# string functions
func str (x : Int)  { ^call (Int) -> Str  "juStd_strInt"  (x) }
func str (x : Bool) { ^call (Bool) -> Str "juStd_strBool" (x) }
func str (x : Real) { ^call (Real) -> Str "juStd_strReal" (x) }
func len (x : Str)  { ^call (Str) -> Int  "juStd_lenStr"  (x) }
func ++ (x : Str, y : Str) { ^call (Str, Str) -> Str "juStd_appStrStr" (x, y) }

# io functions
func println ()         { ^call () -> ()       "juStd_println"     () }
func print (x : Str)    { ^call (Str) -> ()    "juStd_printStr" (x) }
func print (x : Int)    { ^call (Int) -> ()    "juStd_printInt"    (x) }
func print (x : Real)   { ^call (Real) -> ()   "juStd_printReal"   (x) }
func print (x : Bool)   { ^call (Bool) -> ()   "juStd_printBool"   (x) }




## generic utilities

# types
type [\a] =
	cons(hd : \a, tl : [\a]), nil()
type ?(\a) =
	some(val : \a), none()

# list
func len (x : [_]) {
	# TODO: tail calls
	if nil?(x) then
		0
	else
		1 + len(tl(x))
}

# string
func str (x : Str) { x }
func str (x, y) { str(x) ++ str(y) }

# io
func print (x, y)      { print(x); print(y); }
func print (x, y, z)   { print(x); print(y); print(z); }
func println (x)       { print(x); println(); }
func println (x, y)    { print(x); print(y); println(); }
func println (x, y, z) { print(x); print(y); print(z); println(); }

# logic
func > (x, y)  { <(y, x) }
func <= (x, y) { if <(x, y) then true else ==(x, y) }
func >= (x, y) { if <(y, x) then true else ==(y, x) }
func != (x, y) { !(==(x, y)) }
func - (x, y)  { +(x, -(y)) }
