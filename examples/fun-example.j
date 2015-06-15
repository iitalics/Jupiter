# loops

type Weapon = weapon(name : Str, dmg : Int, price : Int)

func print (weap : Weapon) {
	println("weapon '", weap.name, "'");
	println("   dmg: ", weap.dmg);
	println(" price: $", weap.price);
}

func max (lst : [\a], pred : (\a, \a) -> Bool) {
	let best = lst.hd;

	for x : lst.tl {
		if pred(best, x) {
			best = x;
		}
	}

	best
}

func main () {
	let store = [
		weapon("Wand",   65, 100),
		weapon("Axe",    45, 90),
		weapon("Dagger", 70, 85),
		weapon("Sword",  65, 85),
	];

	println("WEAPONS:");
	for w : store {
		println("-----------");
		w.print();
	}
	println();

	println("MOST EXPENSIVE:");
	store.max(func (a, b) { a.price < b.price }).print();
	println();

	println("MOST DAMAGE:");
	store.max(func (a, b) { a.dmg < b.dmg }).print();
	println();
}