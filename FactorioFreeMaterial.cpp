// A factory is a chain of recipes, listed in dependency order. Each recipe
// becomes a node that inherits the node before it, so by the time a recipe is
// written its ingredients are already bases and can be named directly.
//
// material<Item> holds one item's throughput and nothing else, so reaching it
// is a base-class name rather than a lookup: no deduction, no type walk. It is
// also what keeps the order honest — material<belt> is not a base until belt
// has been listed, so a recipe cannot name an item that comes after it.
//
// Read a recipe as: this item, made <N> at a time, takes <T> seconds, from
// these ingredients. The wiki numbers are kept as written and the division
// happens where it is used. Items are declared inline at their recipe; ore
// patches get a recipe with no ingredients.
//
// sweep() runs each recipe exactly once, latest first, so an item's throughput
// is final before the recipe that makes it is scaled.
#include <chrono>
#include <cstdio>

struct root {
	constexpr void sweep() {}
};

template <class Item, int Amount = 1> struct ingredient {
	using item = Item;
	static constexpr int amount = Amount;
};

template <class Item> struct material {
	double throughput = 0;
};

// what you write, and the node it becomes: one parameter list serves both
template <class Item, int Amount, double Time, class... Ings>
struct recipe {
	template <class Base>
	struct node : Base, material<Item> {
		static constexpr int amount = Amount;
		static constexpr std::chrono::duration<double> time{ Time };

		constexpr void sweep() {
			((material<typename Ings::item>::throughput
				+= material<Item>::throughput * Ings::amount / amount), ...);
			Base::sweep();
		}
	};
};

template <class Base, class... Rs> struct chain { using type = Base; };
template <class Base, class R, class... Rs>
struct chain<Base, R, Rs...> : chain<typename R::template node<Base>, Rs...> {};

using factory = chain<
	root,
	recipe<struct crude, 1, 0.0>, // ore patch: no ingredients
	recipe<struct iron, 1, 3.2>,
	recipe<struct gear, 1, 0.5, ingredient<iron, 2>>,
	recipe<struct belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>,
	recipe<struct petro, 45, 5.0, ingredient<crude, 100>> // basic oil processing
>::type;

constexpr factory run() {
	factory f{};
	f.material<belt>::throughput += 10;
	f.material<petro>::throughput += 90;
	f.sweep();
	return f;
}

void Freemain() {
	constexpr auto f = run();
	static_assert(f.material<gear>::throughput == 5);
	static_assert(f.material<iron>::throughput == 15);
	static_assert(f.material<crude>::throughput == 200);
	std::printf("belt=%g gear=%g iron=%g crude=%g\n",
		f.material<belt>::throughput, f.material<gear>::throughput,
		f.material<iron>::throughput, f.material<crude>::throughput);
}