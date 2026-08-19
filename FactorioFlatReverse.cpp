#include <chrono>

template <class A, class B> constexpr bool same = false;
template <class A> constexpr bool same<A, A> = true;

template <class Item, int Amount = 1> struct ingredient {
	using item = Item;
	static constexpr int amount = Amount;
};

template <class Item, int Amount, double Time, class... Ings>
struct recipe {
	using item = Item;
	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };

	// how much of Want one craft consumes; 0 if Want is not an ingredient
	template <class Want>
	static constexpr int uses = ((same<Want, typename Ings::item> ? Ings::amount : 0) + ... + 0);
};

template <class... Rs>
struct recipes {

	template<class... Ings>
	struct orders
	{
		// via has to be written first and has to stay a function. A function
		// body is a complete-class context, so it may name demand before demand
		// is declared; a variable template's initializer may not look forward.
		// The if constexpr is what breaks the self-reference -- a plain ternary
		// would still instantiate demand<Item> while defining it.
		template <class Item, class R>
		static constexpr double via() {
			if constexpr (R::template uses<Item> == 0) return 0;              // not a consumer
			else return demand<typename R::item>                   // how many of R
				* R::template uses<Item> / R::amount;                   // times its rate
		}

		// demand for Item: what was ordered, plus what every recipe that consumes
		// it needs. No inversion -- the fold visits every recipe and the ones that
		// do not use Item contribute nothing.
		//
		// A variable template rather than a function, because a specialization is
		// instantiated once and a constexpr call is not: written as a function
		// this is re-walked once per path from Item up to the orders, which is
		// exponential on a graph where paths multiply.
		template <class Item>
		static constexpr double demand =
			((same<Item, typename Ings::item> ? double(Ings::amount) : 0) + ... + 0)
			+ (via<Item, Rs>() + ... + 0);
	};
};

using factory = recipes<
	recipe<struct crude, 1, 0.0>,
	recipe<struct iron, 1, 3.2>,
	recipe<struct gear, 1, 0.5, ingredient<iron, 2>>,
	recipe<struct belt, 2, 0.5, ingredient<iron, 1>, ingredient<gear, 1>>,
	recipe<struct petro, 45, 5.0, ingredient<crude, 100>>
>::template orders<
	ingredient<belt, 10>,
	ingredient<petro, 90>>;

static_assert(factory::demand<belt> == 10);
static_assert(factory::demand<gear> == 5);
static_assert(factory::demand<iron> == 15);
static_assert(factory::demand<crude> == 200);
