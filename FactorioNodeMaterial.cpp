// A factory is a chain of recipes, listed in dependency order. Each recipe
// becomes a node that inherits the node before it, so by the time a recipe is
// written its ingredients are already bases and can be named directly.
//
// material<It> is the node itself, so throughput, amount and time all come
// from one type and an item's numbers can be asked for by name with no
// object: factory::material<iron>::time. Reaching a node is one deduction
// step through made(), not a walk up the chain.
//
// It also keeps the order honest -- a node for a later item is not a base
// yet, so a recipe cannot name an item that comes after it.
//
// The price is that node must be a top-level template, because deduction
// cannot see through recipe<...>::node. The parameter list is spelled in
// node's declaration, node's definition, twice in made(), and three times in
// chain's step; all of those have to stay in sync by hand. FactorioFreeMaterial
// keeps them down to one at the cost of the lookup.
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

// what you write: pure description
template <class Item, int Amount, double Time, class... Ings> struct recipe {};

template <class Base, class Item, int Amount, double Time, class... Ings> struct node;

// declared only: names the node for Item among the bases of what is passed
template <class Item, class Base, int Amount, double Time, class... Ings>
node<Base, Item, Amount, Time, Ings...> made(node<Base, Item, Amount, Time, Ings...>*);

template <class Base, class Item, int Amount, double Time, class... Ings>
struct node : Base {
	double throughput = 0;
	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };

	// only items listed earlier (and this one) are bases here
	template <class It> using material = decltype(made<It>(static_cast<node*>(nullptr)));

	constexpr void sweep() {
		((material<typename Ings::item>::throughput += throughput * Ings::amount / amount), ...);
		Base::sweep();
	}
};

template <class Base, class... Rs> struct chain { using type = Base; };
template <class Base, class Item, int Amount, double Time, class... Ings, class... Rs>
struct chain<Base, recipe<Item, Amount, Time, Ings...>, Rs...>
	: chain<node<Base, Item, Amount, Time, Ings...>, Rs...> {};

using factory = chain<
	root,
	recipe<struct crude, 1, 0.0>, // ore patch: no ingredients
	recipe<struct iron, 1, 3.2>,
	recipe<struct gear, 1, 0.5, ingredient<iron, 2>>,
	recipe<struct belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>,
	recipe<struct petro, 45, 5.0, ingredient<crude, 100>> // basic oil processing
>::type;

static_assert(factory::material<belt>::amount == 2);
static_assert(factory::material<iron>::time == std::chrono::duration<double>{3.2});

constexpr factory run() {
	factory f{};
	f.material<belt>::throughput += 10;
	f.material<petro>::throughput += 90;
	f.sweep();
	return f;
}

void Nodemain() {
	constexpr auto f = run();
	static_assert(f.material<gear>::throughput == 5);
	static_assert(f.material<iron>::throughput == 15);
	static_assert(f.material<crude>::throughput == 200);
	std::printf("belt=%g gear=%g iron=%g crude=%g  sizeof=%zu\n",
		f.material<belt>::throughput, f.material<gear>::throughput,
		f.material<iron>::throughput, f.material<crude>::throughput, sizeof(factory));
}
