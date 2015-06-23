# obligatory fizzbuzz
import std/stdlib

pub func main () {
	for i : 1 -> 101 {
		let num = true;

		if i % 3 == 0 {
			print("Fizz");
			num = false;
		}
		if i % 5 == 0 {
			print("Buzz");
			num = false;
		}

		if num {
			print(i);
		}
		println();
	}
}