# loops
import std/stdlib

pub func main () {
	# "while" loop
	let going = true;
	let y = 0;
	loop going {
		println("y = ", y);
		y = y.succ;

		if y >= 10 {
			going = false;
		}
	}

	# "for range" loop
	for x : 0 -> 10 {
		println("x = ", x);
	}

	# "for each" loop
	let lst = [6, 3, 2];
	for t : lst {
		println("t = ", t);
	}
}