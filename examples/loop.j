# loops

func main () {
	let i = 1;

	loop i <= 10 {
		println("i = ", i);
		i = i.succ;
	}
}