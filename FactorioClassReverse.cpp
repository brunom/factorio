// The material is the recipe: struct belt is both the item and the way to make
// it. Written as a struct rather than an alias, so that two items with the same
// wiki numbers stay two items -- and so a material is complete the moment it is
// written, which is what makes a cycle unwritable.
//
// Read a material as: this item, made <N> at a time, takes <T> seconds, from
// these inputs. The numbers are kept as the wiki writes them and the division
// happens where it is used. An ore patch has no inputs.
//
// A material only knows how to combine its own inputs. What is being counted,
// and where the ladder stops, is a counter handed to it -- so the question
// reads left to right: count iron per belt, count iron to gear per belt.
#include <chrono>
#include <concepts>

// the answer is carried by the type, not by the value: two paths to the same
// question are the same specialization, so the compiler computes it once.
// Returning a plain double instead compiles the same and is exponential.
template <double Value> struct Const {
	static constexpr double value = Value;
	constexpr operator double() const { return Value; }
};

template <class Material, int Amount = 1> struct input {
	using material = Material;
	static constexpr int amount = Amount;
};

template <int Amount, double Time, class... Input>
struct material {
	// sizeof demands a complete type, so an input must already be
	// defined -- which makes a cycle unwritable, without inheriting anything
	static_assert(((sizeof(typename Input::material) > 0) && ...),
		"an input is not defined yet -- materials cannot form a cycle");

	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };

	// the + 0.0 is load-bearing, not just a terminator for ore patches: any
	// branch that dead-ends without meeting the counter's stop expands to
	// nothing, and nothing has to read as zero rather than as an error
	static constexpr auto of(auto counter) {
		return Const<((Input::amount * counter.per(typename Input::material{}).value
		               / Amount) + ... + 0.0)>{};
	}
};

// the stop case is a plain function: BaseInput is fixed by the enclosing
// template, so it is only a parameter type. That is the whole reason these
// nest and a specialization could not -- overloading can name the enclosing
// argument, pattern matching cannot reach it. It is also what lets to<> live
// inside count<> and still stop on both.
template <class BaseInput> struct count {
	static constexpr Const<1.0> per(BaseInput) { return {}; }
	static constexpr auto per(auto output) { return output.of(count{}); }

	// the same question, restricted to what arrives by way of Through
	template <class Through> struct to {
		static constexpr auto per(Through) { return Const<count::per(Through{}).value>{}; }

		// constrained so it stays a distinct declaration when Through is BaseInput
		template <std::same_as<BaseInput> B> requires (!std::same_as<B, Through>)
		static constexpr Const<0.0> per(B) { return {}; }

		static constexpr auto per(auto output) { return output.of(to{}); }
	};
};

struct copper : material<1, 3.2> {};
struct iron : material<1, 3.2> {};
struct gear : material<1, 0.5, input<iron, 2>> {};
struct red_science : material<1, 5.0, input<copper>, input<gear>> {};
struct belt : material<2, 0.5, input<gear>, input<iron>> {};

static_assert(count<copper>::per(red_science{}) == 1);
static_assert(count<iron>::per(red_science{}) == 2);
static_assert(count<iron>::per(copper{}) == 0);

// belt is made 2 at a time from 1 gear and 1 iron, so 1.5 iron each, of
// which 1 arrives through gear
static_assert(count<iron>::per(belt{}) == 1.5);
static_assert(count<iron>::to<gear>::per(belt{}) == 1.0);
static_assert(count<iron>::to<iron>::per(belt{}) == 1.5);
static_assert(count<iron>::to<copper>::per(belt{}) == 0);
