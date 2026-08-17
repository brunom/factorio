// Flat: the factory inherits its recipes, and each recipe carries its own
// material<Item>, so every item's throughput is a base-class name away.
//
// Nothing nests, so sweep() supplies the order itself: operator| runs its
// right side first, so a right fold over the recipe list runs it back to
// front. The list therefore still reads in dependency order and items are
// still declared inline at their recipe.
//
// Read a recipe as: this item, made <N> at a time, takes <T> seconds, from
// these ingredients. Ore patches get a recipe with no ingredients.
#include <chrono>
#include <cstdio>

template <class Item, int Amount = 1> struct ingredient {
	using item = Item;
	static constexpr int amount = Amount;
};

template <class Item> struct material {
	double throughput = 0;
};

template <class Item, int Amount, double Time, class... Ings>
struct recipe : material<Item> {
	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };

	// this recipe's own throughput is in hand; only the ingredients are
	// looked up, and only in the factory that owns this subobject
	static constexpr void step(auto& f) {
		((f.material<typename Ings::item>::throughput
			+= f.material<Item>::throughput * Ings::amount / amount), ...);
	}
};

// swept is the accumulator: a << swept runs a and hands back a fresh swept.
// In a right fold the inner expression is an operand of the outer call, so it
// is evaluated first -- which walks the pack backwards. The operator cannot
// catch anything it should not, because one operand is always swept.
struct swept {};

template <class A>
constexpr swept operator<<(A a, swept) { a(); return {}; }

template <class... Rs>
struct production : Rs... {
	constexpr void walk() {
		// foldr f z xs: every intermediate value is just swept, so nothing nests
		([this] { Rs::step(*this); } << ... << swept{});
	}
};

using factory = production<
	recipe<struct crude, 1, 0.0>, // ore patch: no ingredients
	recipe<struct iron, 1, 3.2>,
	recipe<struct gear, 1, 0.5, ingredient<iron, 2>>,
	recipe<struct belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>,
	recipe<struct petro, 45, 5.0, ingredient<crude, 100>> // basic oil processing
>;

constexpr factory run() {
	factory f{};
	f.material<belt>::throughput += 10;
	f.material<petro>::throughput += 90;
	f.walk();
	return f;
}

int main() {
	constexpr auto f = run();
	static_assert(f.material<gear>::throughput == 5);
	static_assert(f.material<iron>::throughput == 15);
	static_assert(f.material<crude>::throughput == 200);
	static_assert(f.recipe<belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>::amount == 2);
	std::printf("belt=%g gear=%g iron=%g crude=%g  sizeof=%zu\n",
		f.material<belt>::throughput, f.material<gear>::throughput,
		f.material<iron>::throughput, f.material<crude>::throughput, sizeof(factory));
}