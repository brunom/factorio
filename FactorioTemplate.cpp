// A recipe holds everything about one item and is a base of the node for it,
// so material<Item> is just a name for that base: amount, yield and time all
// arrive through one :: path. Only amount is a field, so an item costs 8
// bytes. factory holds the chain rather than deriving from it, so nothing
// needs its own type while it is still incomplete.
//
// Read a recipe as: this item, made <N> at a time, takes <T> seconds, from
// these ingredients. The wiki numbers are kept as written. Items are declared
// inline at their recipe; raw materials get a recipe with no ingredients.
//
// sweep() runs each recipe exactly once, latest first, so an item's amount is
// final before the recipe that makes it is scaled. A recipe can only name
// items listed before it, because only those are bases at that recipe's depth
// in the chain — a forward reference does not compile.
#pragma once
#include <chrono>

struct root {
	constexpr void sweep() {}
};

template <class Item, int N = 1> struct ingredient {
	using item = Item;
	static constexpr int amount = N;
};

template <class Item, int Amount, double Time, class... Ings>
struct recipe {
	double throughput = 0;
	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };
};

// declared only: names the recipe for Item among the bases of what is passed
template <class Item, int Y, double T, class... I>
recipe<Item, Y, T, I...> made(recipe<Item, Y, T, I...>*);

template <class Base, class R> struct node;

template <class Base, class Item, int Amount, double Time, class... Ings>
struct node<Base, recipe<Item, Amount, Time, Ings...>>
	: Base, recipe<Item, Amount, Time, Ings...> {
	// only items listed earlier (and this one) are bases here
	template <class It> using material = decltype(made<It>(static_cast<node*>(nullptr)));

	// every recipe in the chain is a base here, so names must be qualified
	using self = recipe<Item, Amount, Time, Ings...>;

	constexpr void sweep() {
		((material<typename Ings::item>::throughput += self::throughput * Ings::amount / self::amount), ...);
		Base::sweep();
	}
};

template <class Base, class... Rs> struct chain { using type = Base; };
template <class Base, class R, class... Rs>
struct chain<Base, R, Rs...> : chain<node<Base, R>, Rs...> {};

using factory = chain<
	root,
	recipe<struct crude, 1, 0.0>, // ore patch: no ingredients
	recipe<struct iron, 1, 3.2>,
	recipe<struct gear, 1, 0.5, ingredient<iron, 2>>,
	recipe<struct belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>,
	recipe<struct petro, 45, 5.0, ingredient<crude, 100>> // basic oil processing
>::type;

// queries that need no object
static_assert(factory::material<belt>::amount == 2);
static_assert(factory::material<iron>::time == std::chrono::duration<double>{3.2});

constexpr factory run() {
	factory f{};
	f.material<belt>::throughput += 10;
	f.material<petro>::throughput += 90;
	f.sweep();
	return f;
}

int main() {
	constexpr auto f = run();
	static_assert(f.material<gear>::throughput == 5);
	static_assert(f.material<iron>::throughput == 15);
	static_assert(f.material<crude>::throughput == 200);
	std::printf("belt=%g gear=%g iron=%g crude=%g\n",
		f.material<belt>::throughput, f.material<gear>::throughput,
		f.material<iron>::throughput, f.material<crude>::throughput);
}
