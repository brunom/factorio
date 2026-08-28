// FactorioLambda with the chain call taken out.
//
// That file is the runtime shape in miniature: a local per item, a lambda per
// recipe that charges what it takes, and each one calling the one before it --
//
//     double gear = 0;
//     auto sweep1 = [&] { iron += gear; sweep0(); };
//     double belt = 0;
//     auto sweep2 = [&] { gear += belt; iron += belt; sweep1(); };
//
// so the sweep runs latest first and an item's rate is final before the recipe
// that makes it is scaled. FactorioNodeMaterial walks the same order through
// Base::sweep(), FactorioClassReverse through the recursion in count_v. All
// three write the link to the step before by hand.
//
// None of them has to. Locals in a block are destroyed in reverse order of
// construction, and a recipe cannot be written until its ingredients have
// been, so the order they are written in is a topological sort and destruction
// is that order backwards, which is the one the demand has to travel in. An
// item's destructor cannot run until every recipe above it has already run and
// charged it. Name lookup in a block is what checks that: an ingredient that
// has not been written yet is a build error, aliases included.
//
// A recipe is its name and what one of it takes, and what it takes is a sum of
// products -- spelled as one:
//
//     auto offshore_pump = item("offshore_pump");
//     auto steam = recipe("steam", (boiler + fuel*0.45 + water*6.0) / 60);
//
// prod is a leaf -- how much of what -- and sum<Left, Right> is two of them.
// The `/` is a scaling rather than a node, so it needs no type of its own. The
// two of them together are named once, as a concept:
//
//     template <class T> concept input =
//         std::is_same_v<T, prod> ||
//         (std::is_same_v<T, sum<decltype(T::left), decltype(T::right)>>
//          && T::an_input);
//
// and the family is closed. Not by being declared -- a declared membership is
// an invitation, and any later class can accept it by carrying the same member
// -- but by identity: to be an input you have to BE a prod, or be the very sum
// built out of your own two member types, which only a sum is. A concept cannot
// be specialized and there is no table beside the classes, so there is nothing
// left to add yourself to. sum's `an_input` is `input<Left> && input<Right>`,
// so the invariant rides along behind the identity check, where declaring it
// gains a stranger nothing.
//
// An item is not one of them. An `item` is a name and a rate, and a
// `recipe<Ins>` is an item with what one of it takes on top -- so an offshore
// pump, which takes nothing, is written as the item and not as a recipe with an
// empty one. There is no empty one, and no type for it.
//
// An item becomes an expression only by being converted to a prod. That is the
// one door in, it is the only place a reference is taken, and it is why the
// deduced half of each operator, which takes an `input`, will not look at a
// bare item at all.
//
// `boiler` on its own is one boiler-second, `fuel*0.45` is 0.45 coal, and the
// `/60` is the sixty steam a craft makes -- said once over the whole recipe
// rather than once per input, which is why an item has no count of its own to
// carry and the sweep has nothing to divide. The sweep is these two lines and
// nothing else:
//
//     void sum ::operator+=(double rate) { left += rate; right += rate; }
//     void prod::operator+=(double rate) { what += rate * amount; }
//
// The report rides on them one call further down: `what += ...` arrives at
// item::operator+=, which is where a link is both added and written.
//
// The parentheses have to close before the `/`. Written
// `(boiler + fuel*0.45 + water*6.0/60)` it compiles and divides the water
// alone.
//
// A machine is an item like any other, because a machine makes something too:
// it makes crafting. Its speed is the divisor on the LINK into it and not on
// its own inputs. The wiki's 75 kW is per machine-second; a recipe asks for
// seconds of crafting; speed is the rate between the two clocks. Divide the
// machine's own inputs by it and the kilowatts still come out right --
// 77.5/0.50 is what the two machines draw -- but the line then counts seconds
// of crafting, and a furnace at 2.00 shows twice its count. Divide the link
// and the line is a machine count, which is also the only place a leaf can
// say its speed at all: a burner drill has no inputs to divide it into. A
// machine at 1.00 needs no term of its own: its node already is one.
//
// The `/` is a rescaling of the terms inside it and not a node of its own, but
// it is written once at the end of the recipe rather than folded into each
// term, so the wiki's own numbers stay where they are written: a two-at-a-time
// craft still says 10 rather than the 5 it works out to. The tree is walked at
// compile time and every leaf's amount divided on the way through. That
// arithmetic order moves no printed figure -- all five products come out
// identical to the version that kept a divisor node.
//
// The left side is a tree and not a list. `a + b + c` is a pair of pairs, one
// type per shape, so the sweep is a recursive walk over types the compiler can
// see through rather than a loop over an array of links. Nothing is
// type-erased, nothing allocates, and a recipe's inputs live in the recipe.
//
// ONE RULE, AND IT IS A BUILD ERROR. An item is a node in a graph with a
// destructor that charges its inputs, so a copy of one is always a mistake, and
// item says so: `item(const item&) = delete`. Every version of this file has
// been bitten by the same mistake -- a parameter that took an item by value
// instead of converting it to a term, so the copy went into the tree, charged
// its own inputs, printed its own line, and left the real one at nothing. Once
// it was `operator+(auto left, in right)`; once a deduction guide a constructor
// wrote for free, which gave four ore patches their own private drill, 20 and
// 13 and 21 and 50.5 where the one drill should have said 104.5; once a
// deduced `Ins`. Each time it compiled, and each time every material figure was
// still correct, which is exactly as much warning as it gave. Those same four
// figures are printed now, as the drill links under stone, coal, copper ore and
// iron ore -- so the bug would show as four totals where there is one, which is
// the same arithmetic in the wrong column.
//
// Deleting the copy is worth more than everything that was tried against those
// in turn -- concrete parameters, a `not an item` concept, four deduction
// guides, std::type_identity_t -- because it is the mistake itself that is now
// impossible, rather than each of the ways of writing it. All of that is gone.
//
// It used to cost a `*1` on a recipe with a single input:
//
//     auto iron_ore = recipe("iron_ore", drill*1);
//
// because with no operator in the expression to convert it, a bare item was
// deduced and copied, and the delete turned that into an error at the line that
// wrote it. But an error is not the right answer to a recipe that is written
// correctly, and recipe gets it for one line. Its constructor takes `Ins`
// itself, so for a recipe<prod> the parameter reads `prod`, concretely, and a
// bare item converts on the way in exactly as it does at an operator. The
// constructor writes the deduced guide for itself; the concrete one is the
// single guide left in the file. The `*1` is gone, and of the four guides that
// once forbade things, one that permits is what is left.
//
// So the delete stands for `auto dup = iron_plate;`, which is the mistake
// itself and has no correct spelling.
//
// THE LIFETIME IS NOT DELETED, IT IS UNREACHABLE. An expression keeps a
// reference to an item, so a reference to a temporary would outlive what it
// points at -- and the only way an item enters an expression is
//
//     prod(item& what, double amount = 1.0)
//
// A temporary does not bind to a non-const lvalue reference, so there is no
// such expression to build. Nothing is refused by name and there is no list of
// refusals to have got wrong: the one door in is the wrong shape for it.
//
// That is why item does not inherit from an expression here. It did, for a
// while, and it bought something real -- an item WAS a sum, deduction found it
// by walking bases, and each operator could be written once instead of twice,
// three functions where there are now six. But a by-value parameter then sliced
// the item's base subobject out of a temporary, and stopping that took a
// deleted constructor, and a deleted constructor is a list. `sum(item&&)`
// alone missed const rvalues -- that compiled, and the sanitizer called it a
// stack-use-after-scope. `sum(const item&&)` beside it shut that spelling and
// left the same question open about the next. Constraining it instead
// (`template <a_temporary T> sum(T&&) = delete`) answered the question without
// a list, and was still a rule about what may not happen rather than about what
// may. Three operators are not worth a guard whose completeness is an argument.
//
// Putting the operators in a CRTP base does not get them back either. An empty
// base cannot be sliced, which is the appeal, but it forces the operands to be
// taken as `const expr<L>&`, and const is exactly what the reference bind uses
// to tell a temporary from an item.
//
// THREE RULES THAT ARE STILL THE READER'S:
//
//   * Overload resolution applies a user conversion; template argument
//     deduction never does. That is the whole of why each operator is written
//     twice: the concrete half names `prod`, so a bare item is converted into
//     one on the way in, and the deduced half names `input`, so it takes a tree
//     and a conversion is never considered for it. Deduction does walk the
//     argument's base classes for one that matches the pattern, and that door
//     was open for a while -- an item inherited from a sum and every operator
//     collapsed to one. It is deliberately shut again, and the compiler says so
//     in as many words: `'item' is not derived from 'sum<A, B>'`.
//     DeductionRepro.cpp is that sentence in thirty lines.
//   * prod holds an item by reference and every node above it holds what is
//     under it by value. That split is the lifetime rule and the whole of the
//     safety: an item outlives every term about it, and an expression is a
//     prvalue that would not, so only the leaf may point at anything. The
//     reference is non-const, so `item("gone")*2.0` does not bind and does not
//     compile.
//   * A member operator does not convert its left operand -- `a + b` looks for
//     operator+ on a's own class and its bases and at namespace scope, and will
//     not convert a to find one. So members are not an option here at all: a
//     bare item is exactly what has to be converted, and a member is the one
//     kind of function that will not do it. They were an option in the version
//     where an item inherited from a sum, and were measured there -- identical
//     figures, but the temporary guard then needed ref-qualifiers on both
//     classes, ten declarations against four, because a `const &` accepts
//     rvalues and a plain `&` rejects `(a + b)*2` along with them.
//
// WHAT IS PRINTED, and where from. Two kinds of line, and only one of them
// comes from a destructor.
//
//   * A link -- what one recipe takes of one thing a second -- is written by
//     item::operator+= as the charge arrives, indented, once per asker.
//   * A total is written by ~item, when there is nothing left to arrive.
//     ~recipe is the sweep and a base is destroyed after the rest of the
//     object, so by then every input has been charged and has said so.
//
// A block therefore reads its inputs first and its own total last, and that is
// not a choice: nothing can print a heading for charges its own members have
// not made yet. Products come first overall, because that is the order things
// are destroyed in; read upwards for the order a factory is built.
//
// Both lines are the item's and neither is the term's, which is what a recipe
// column cost: an amount belongs to a term and an item has several, so no one
// row can name a single machine and a single count. What the item does get is
// the number the term charged it, and that number is the link -- so a share is
// said once per asker and a total once, and the item with no recipe at all, an
// offshore pump, still says its total. Summing the links printed under a name
// reproduces that name's own total line, which is the listing checking itself.
//
// WHAT THIS DOES NOT DO:
//
//   * Nothing can be asked after a run, so the wiki's numbers cannot be
//     static_asserts. They are in the comment at the end.
//   * ostream::operator<< may throw and a destructor may not. A total is
//     written from ~item and a link from a charge a destructor made, so either
//     way a failure to write would call std::terminate rather than propagate.
//   * The sweep and the report are one walk, so there is no quiet mode: a
//     charge cannot be made without a line being written.
//   * One graph answers for one product, and the graph is a block rather than
//     a type, so a second product is a second copy of it.
//
// What a local cannot be is a reference to the recipe rather than the recipe.
// An item has to *be* the local, because being one is what puts its destructor
// in the teardown at all, and the teardown is the algorithm. As a member of a
// class it could not even be that: a temporary bound to a reference member
// lives only to the end of the constructor, so the sweep fired there, before
// anything was seeded. clang refuses it, gcc warns under -Wextra, and the
// sanitizer calls it a stack-use-after-return.
//
// There is no chrono here, and no time field for it to type. A recipe's time is
// on the machine's own input line, where it shares a field with an ingredient's
// count, and that sharing is what lets one shape serve both: an amount is
// always measured in the unit of the term it multiplies, and a machine's term is
// a second of crafting -- the item under it counts machines, and the speed on the
// term is what stands between the two. Typing it a duration would assert
// otherwise. Nothing else is a candidate either -- chrono measures durations
// and has no frequency, so a rate has no chrono spelling at all.
//
// A line is written to a stream and not through a format string. A format
// string is checked against its arguments at compile time, which is worth
// something, but it puts every width and precision in one list and every value
// it is about in another, and reading it means counting along both.
//
// Recipes are vanilla Factorio 2.0 (no Space Age), no modules anywhere. Where
// 1.1 differs it is noted at the recipe.
#include <iomanip>
#include <iostream>
#include <type_traits>

// ---------------------------------------------------------------------------
// What has been researched. Nothing else is a dial: no modules, and a machine
// is picked by naming it further down.
//
// Mining productivity is free ore rather than a faster drill, but with no
// productivity modules to slow anything down the two are the same number here,
// so it multiplies the drill's speed.
// ---------------------------------------------------------------------------

constexpr double mining_productivity = 0.00;   // +0.10 a level, no cap
constexpr double lab_research_speed  = 0.00;   // +2.50 with all six levels

// ---------------------------------------------------------------------------
// What one craft takes is an expression, and there are three shapes of one: a
// product -- how much of what -- a sum of two, and one over how many a craft
// makes. Each knows how to take a rate, and that is all any of them is for.
//
// prod is not a template, which is what makes every operator below a plain
// function with a concrete parameter. A bare item converts to a prod on the
// way in, and a conversion is something overload resolution will do where
// template argument deduction never would. That is the whole reason there is
// no box, no member operators and no concepts here: `boiler + fuel*0.45` works
// because `operator+(prod, prod)` says prod and means it.
//
// It is also the whole of the lifetime rule. prod holds an item by non-const
// lvalue reference, so it can only be made from one that has a name:
// `item("gone")*2.0` will not bind and does not compile. Everything else is
// held by value, and there is nothing else to get wrong.
// ---------------------------------------------------------------------------

struct item;

struct prod;
template <class Left, class Right> struct sum;

// Membership in the expression family. The two halves do two different jobs.
//
// The is_same_v is the closure, and it is the whole of it: a type is an input
// only by BEING a prod, or by being the very sum built out of its own two
// member types -- which only a sum is. A declared membership would not do it,
// because a declaration is an invitation and any later class can accept one by
// carrying the same member; identity cannot be accepted, only had. A concept
// cannot be specialized and std::is_same cannot be specialized, so there is
// nothing left for a stranger to add itself to.
//
// `T::an_input` is the invariant, and it is a member only because a concept may
// not name itself -- `input<decltype(T::left)>` here is `'input' was not
// declared in this scope`, because it is not, inside its own definition. So the
// recursion goes out through sum, which carries `input<Left> && input<Right>`,
// and comes back in here. It rides behind the identity check, so a stranger
// declaring `an_input` gains nothing by it; take it away and the family is
// still closed, but `sum<prod, int>` becomes an input.
//
// Asking a type that has neither is not an error, it is a no -- substitution
// failure in an atomic constraint means unsatisfied.
//
// This is what the deduced half of each operator takes, which is why none of
// them will look at a bare item: the only way an item gets in is by being
// converted to a prod, and that conversion wants a name.
template <class T> concept input =
	std::is_same_v<T, prod> ||
	(std::is_same_v<T, sum<decltype(T::left), decltype(T::right)>> && T::an_input);

// Which classes are part of an expression. Nothing is, until it says so: each
// of the three below declares its own membership on the line after its
// definition, and a type that never declares it is an outsider by default
// rather than by having been listed.

// A temporary, and that is the whole of the rule. It names no class, so there
// is no list to have got wrong -- which was the point of writing it as a
// constraint rather than as overloads. What an expression keeps of an item is a
// reference, and a reference outlives a temporary, so no temporary may enter
// one. A concept here is a named bool and nothing more: no requires-expression,
// no requires-clause.
// How much of what: a leaf, and the only place a reference is held. Its
// constructor is the single door into an expression, and it wants a `item&` --
// a non-const lvalue reference, which a temporary cannot bind to. That is the
// whole of the lifetime rule, and it is a whitelist: not a list of ways in that
// are forbidden, but one way in that is allowed.
struct prod {
	item& what;
	double amount;

	prod(item& what, double amount = 1.0) : what(what), amount(amount) {}

	void operator+=(double rate);   // needs item, so it is defined below
};

// Two of them. It holds what is under it by value, because every operator
// returns a prvalue and a reference would bind to a temporary that is gone by
// the next statement. There is no node for the `/`: it is a scaling.
template <class Left, class Right> struct sum {
	// and a sum is one exactly when both halves are -- the invariant said where
	// the compiler holds it rather than in a comment
	static constexpr bool an_input = input<Left> && input<Right>;

	Left  left;
	Right right;

	void operator+=(double rate) {
		left  += rate;   // left to right, so a recipe reads as written
		right += rate;
	}
};

// Two of each, and the pairing is what refusing a bare item costs. The
// concrete one names `prod`, which is a conversion target, so a bare item
// becomes a leaf on the way in -- through `prod(item&)`, which is the only
// door there is. The deduced one names `input`, so it takes a tree, and it will
// not look at a bare item at all: an item satisfies nothing, declares nothing,
// and inherits from nothing that does.
//
// So a temporary cannot get in. Not because some way of getting in was deleted,
// but because the way in is a reference bind that a temporary fails.
inline sum<prod, prod> operator+(prod left, prod right) { return { left, right }; }

template <input L>
sum<L, prod> operator+(L left, prod right) { return { left, right }; }

inline prod operator*(prod term, double per_craft) {
	return { term.what, term.amount * per_craft };
}

template <class L, class R>
sum<L, R> operator*(sum<L, R> expr, double by) { return { expr.left * by, expr.right * by }; }

// a craft that makes several at a time, said once over the whole recipe rather
// than once per term -- so a term keeps the number the wiki writes, 10 rather
// than the 5 it works out to
inline prod operator/(prod term, double makes) { return term * (1 / makes); }

template <input E>
auto operator/(E expr, double makes) { return expr * (1 / makes); }

// ---------------------------------------------------------------------------
// An item is a name and a rate a second, and it is what every term charges. The
// game holds a machine in your inventory too, so an assembly is an item here;
// water and steam are fluids and a kilowatt is neither, which is where the word
// is stretched. An item is also all there is to a thing with no recipe -- an
// offshore pump is one of these and nothing more.
// ---------------------------------------------------------------------------

struct item {
	const char* name;
	double      rate = 0.0;   // a second, and final only at destruction

	explicit item(const char* name) : name(name) {}

	// An item is a node in a graph with a destructor that charges its inputs,
	// so a copy of one is always a mistake -- and saying so once here is what
	// closes the whole family of them. Every version of this file has been bitten
	// by a parameter that took an item by value instead of converting it to a
	// term: the copy went into the tree, charged its own inputs, printed its own
	// line, and left the real one at nothing, and it compiled every time. Now it
	// does not compile, wherever it is written.
	item(const item&) = delete;

	// Charged by whoever wants it, and it says so on the way in: `per_second`
	// is what one recipe takes of this a second, which is one link. Both of an
	// item's lines are written here, the share as it arrives and the total when
	// there is nothing left to arrive -- so an item several recipes ask for
	// says each share once per asker and its total once, at the end.
	void operator+=(double per_second) {
		if (per_second == 0.0) return;   // nobody asked, so there is no link
		rate += per_second;
		std::cout << "    " << std::left  << std::setw(18) << name
		          << std::right << std::fixed << std::setprecision(4)
		          << std::setw(13) << per_second << "\n";
	}

	// The total, and it comes last: a base is destroyed after the rest of the
	// object, so by the time this runs the recipe below has charged -- and
	// printed -- every input, and an item with no recipe under it, which is the
	// whole of an offshore pump, still gets here.
	~item() {
		if (rate == 0.0) return;    // nobody asked for it
		std::cout << "  " << std::left  << std::setw(20) << name
		          << std::right << std::fixed << std::setprecision(4)
		          << std::setw(13) << rate << "\n";
	}
};

// A term charges what it is about, and that is all it does -- the two lines
// of the sweep stay the two lines of the sweep. What it charges is the link,
// so the item it lands on has the number it needs to say, and says it there.
// A block therefore reads inputs first and total last, which is not a choice:
// a base is destroyed after its members, so nothing can print a heading for
// charges its own members have not made yet. The seed at the end of factory()
// writes `research_unit.rate` and not the item, so the demand is not a link
// and does not print as one.
inline void prod::operator+=(double rate) { what += rate * amount; }

// ---------------------------------------------------------------------------
// A recipe is an item with what one of it takes -- and Ins is constrained, so
// `recipe<double>` is not a type that exists rather than a type that fails to
// compile later. Its two destructors are the sweep and the report, in that
// order,
// because a base goes after the rest of the object -- by the time either runs,
// every recipe that wanted any of this has already run and said so, and
// nothing below it has moved yet.
// ---------------------------------------------------------------------------

template <input Ins> struct recipe : item {
	Ins ins;

	// What one of it takes, and the parameter is Ins itself rather than
	// something deduced -- so for a recipe<prod> it reads `prod`, concretely,
	// and a bare item converts on the way in exactly as it does at an operator.
	// One constructor covers both, because the guides have already decided
	// which recipe this is by the time it is called.
	recipe(const char* name, Ins ins) : item(name), ins(ins) {}

	~recipe() { ins += rate; }
};

// One guide, and it is the concrete half again. The constructor above writes
// the deduced half for itself -- `recipe(const char*, Ins) -> recipe<Ins>`,
// constrained, so it deduces a written expression and refuses a bare item --
// which leaves only the bare item to say out loud. There is no third for the
// empty recipe, because an item with no inputs is not a recipe at all: it is
// written `item("offshore_pump")` and its class is the base.
//
// Neither takes `template` in front of it. `template <class A> f(x) -> T;` is a
// guide; `template f(x) -> T;` without the angle brackets is an explicit
// instantiation of a function template called f, and there is no such function
// template. gcc and clang refuse it, MSVC only warns -- C4667, cannot find a
// function template that matches the explicit instantiation -- and then the
// guide is simply not there, so every `item("name")` in the graph fails to
// deduce with no further explanation.
recipe(const char*, prod) -> recipe<prod>;   // one input, written bare, converted

// ---------------------------------------------------------------------------
// The graph. In dependency order, because an input has to be written to be
// named -- and therefore in reverse of the order it is swept, which is the
// only item that has to be true for the sweep to be right, and the only item
// the compiler has to be able to see to check it.
// ---------------------------------------------------------------------------

static void factory() {
	// The power plant. None of these four needs electricity: a boiler and a
	// burner drill burn fuel, a steam engine makes the power, an offshore pump
	// needs nothing at all -- which is what lets a kilowatt have a price.
	auto offshore_pump = item("offshore_pump");
	auto boiler        = item("boiler");
	auto steam_engine  = item("steam_engine");
	// mining speed 0.25 less the 0.0375 a second it burns of what it mines, so
	// 0.2125 -- and a leaf has no inputs to divide it into, which is the whole
	// argument for the divisor being on the link: a machine with no recipe of
	// its own still has a speed, and this is where it says it.
	auto burner_drill_impl = item("burner_drill");
	auto burner_drill      = burner_drill_impl / 0.2125;

	auto water = recipe("water", offshore_pump/1200);
	auto fuel  = recipe("fuel",  burner_drill);      // coal, off the power grid

	// a boiler turns 1.8 MW of fuel into 60 steam a second, and coal is 4 MJ,
	// so 0.45 coal. 1.1 took 60 water for that; 2.0.7 made the ratio 1:10
	auto steam = recipe("steam", (boiler + fuel*0.45 + water*6.0) / 60);

	// a steam engine turns 30 steam a second into 900 kW. So a kilowatt is a
	// coal every 4 MJ and a water every 300, from the far side of the ladder.
	auto electricity = recipe("electricity", (steam_engine + steam*30.0) / 900);

	// The machines that burn fuel: what one second of one costs is its
	// consumption over coal's 4 MJ. The fuel is the one the boilers take, mined
	// by a burner drill, which is what keeps a furnace out of the electric
	// drill's chain and so out of a cycle.
	auto stone_furnace      = recipe("stone_furnace", fuel*(90.0 / 4000.0));
	auto steel_furnace_impl = recipe("steel_furnace", fuel*(90.0 / 4000.0));
	auto steel_furnace      = steel_furnace_impl / 2.00;

	// The machines that take power. The wiki gives two numbers and they add:
	// energy consumption while working, then drain, which is charged whether it
	// is working or not -- an active assembling machine 2 draws 155 kW. Both are
	// per machine-second, so they are written on the machine and not divided.
	//
	// Under each is its speed, on the link: a second of crafting takes 1/speed
	// machine-seconds, which is where the two clocks meet. A machine at 1.00
	// needs no second line -- its node already is the term.
	auto assembly_1_impl       = recipe("assembly_1", electricity*(75.0 + 2.5));
	auto assembly_1            = assembly_1_impl / 0.50;
	auto assembly_2_impl       = recipe("assembly_2", electricity*(150.0 + 5.0));
	auto assembly_2            = assembly_2_impl / 0.75;
	auto assembly_3_impl       = recipe("assembly_3", electricity*(375.0 + 12.5));
	auto assembly_3            = assembly_3_impl / 1.25;
	auto electric_furnace_impl = recipe("electric_furnace", electricity*(180.0 + 6.0));
	auto electric_furnace      = electric_furnace_impl / 2.00;
	auto chemical_plant        = recipe("chemical_plant", electricity*(210.0 + 7.0));
	auto refinery              = recipe("refinery", electricity*(420.0 + 14.0));
	// mining speed 1 times the patch's yield: a 538% patch is 5.38, and the
	// recipe below is 10 crude a second, which is what 100% yields. The dials
	// are speeds too, so these three keep a term even at a base figure of 1.00.
	auto pumpjack_impl         = recipe("pumpjack", electricity*(90.0 + 3.0));
	auto pumpjack              = pumpjack_impl / (1.00 * (1 + mining_productivity));
	auto electric_drill_impl   = recipe("electric_drill", electricity*(90.0 + 3.0));
	auto electric_drill        = electric_drill_impl / (0.50 * (1 + mining_productivity));
	auto lab_impl              = recipe("lab", electricity*(60.0 + 2.0));
	auto lab                   = lab_impl / (1.00 * (1 + lab_research_speed));

	// What is built. References, so they are not objects and take no part in
	// the order -- and a reference to a local cannot name one written below it
	// either, so these are checked like everything else.
	auto& assembly = assembly_3;
	auto& furnace  = electric_furnace;
	auto& drill    = electric_drill;

	// ore patches: mining time is 1 second for all four vanilla ores
	auto iron_ore   = recipe("iron_ore",   drill);
	auto copper_ore = recipe("copper_ore", drill);
	auto coal       = recipe("coal",       drill);
	auto stone      = recipe("stone",      drill);
	auto crude_oil  = recipe("crude_oil",  pumpjack / 10);

	// smelting
	auto iron_plate   = recipe("iron_plate",   furnace*3.2  + iron_ore);
	auto copper_plate = recipe("copper_plate", furnace*3.2  + copper_ore);
	auto steel_plate  = recipe("steel_plate",  furnace*16.0 + iron_plate*5.0);
	auto stone_brick  = recipe("stone_brick",  furnace*3.2  + stone*2.0);

	// oil
	auto petroleum_gas = recipe("petroleum_gas",
	                          (refinery*5.0 + crude_oil*100.0) / 45);
	auto plastic_bar   = recipe("plastic_bar",
	                          (chemical_plant + coal + petroleum_gas*20.0) / 2);
	auto sulfur        = recipe("sulfur",
	                          (chemical_plant + water*30.0 + petroleum_gas*30.0) / 2);

	// intermediates
	auto iron_gear    = recipe("iron_gear", assembly*0.5 + iron_plate*2.0);
	auto copper_cable = recipe("copper_cable", (assembly*0.5 + copper_plate) / 2);
	auto electronic_circuit = recipe("electronic_circuit",
	                               assembly*0.5 + iron_plate + copper_cable*3.0);
	auto pipe           = recipe("pipe", assembly*0.5 + iron_plate);
	auto transport_belt = recipe("transport_belt",
	                           (assembly*0.5 + iron_gear + iron_plate) / 2);
	auto inserter       = recipe("inserter",
	                           assembly*0.5 + electronic_circuit + iron_gear + iron_plate);
	auto advanced_circuit = recipe("advanced_circuit",
	                             assembly*6.0 + plastic_bar*2.0
	                             + electronic_circuit*2.0 + copper_cable*4.0);
	auto engine_unit    = recipe("engine_unit",
	                           assembly*10.0 + steel_plate + iron_gear + pipe*2.0);

	// military. A wall is five bricks, and a brick is two stone.
	auto wall        = recipe("wall", assembly*0.5 + stone_brick*5.0);
	auto firearm_mag = recipe("firearm_mag", assembly + iron_plate*4.0);
	// 2.0.46 made this 2 at a time in 6s from 2 mags and 2 copper; in 1.1 it is
	// assembly*3.0 + firearm_mag + copper_plate*5.0 + steel_plate, one at a time
	auto piercing_mag = recipe("piercing_mag",
	                         (assembly*6.0 + firearm_mag*2.0 + copper_plate*2.0
	                          + steel_plate) / 2);
	auto grenade      = recipe("grenade",
	                         assembly*8.0 + coal*10.0 + iron_plate*5.0);  // 10 coal

	// science
	auto automation_pack = recipe("automation_pack",
	                            assembly*5.0 + copper_plate + iron_gear);
	auto logistic_pack   = recipe("logistic_pack",
	                            assembly*6.0 + inserter + transport_belt);
	auto chemical_pack   = recipe("chemical_pack",
	                            (assembly*24.0 + advanced_circuit*3.0
	                             + engine_unit*2.0 + sulfur) / 2);
	auto military_pack   = recipe("military_pack",
	                            (assembly*10.0 + grenade + piercing_mag
	                             + wall*2.0) / 2);

	// One research unit is one of each pack the technology asks for, taking the
	// technology's time. Drop the packs your current research does not need and
	// the labs and their power fall out of the same graph.
	auto research_unit = recipe("research_unit",
		lab*30.0
		//+ automation_pack 
		//+ logistic_pack	                          
		//+ chemical_pack 
		+ military_pack
	);

	research_unit.rate += 1.0;
}

// ---------------------------------------------------------------------------
// Plates per pack is the standard total raw figure, which is materials only:
// coal is only ever an ingredient here, while water is also what the boilers
// drink, so the water on a table is both.
//
//   automation   2 iron plate, 1 copper plate,                        3.16 MW
//   logistic     5.5 iron, 1.5 copper,                               6.10 MW
//   chemical     12 iron, 7.5 copper, 1.5 coal, 37.5 gas, 83.33 crude, 25.86 MW
//   military     5.75 iron, 0.5 copper, 5 coal (10 a grenade),
//                10 stone (2 walls, 5 brick, 2 stone),               11.24 MW
//   research     all four packs, a 30 s tech,                        48.21 MW
//
// The kilowatts are electricity's own line, because a kilowatt is an item.
// ---------------------------------------------------------------------------

int main() { factory(); }
