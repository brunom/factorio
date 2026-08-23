// An item is anything a factory needs a quantity of. The game holds a machine
// in your inventory too, so an assembly is an item here; water and steam are
// fluids and a kilowatt is neither, which is where the word is stretched.
//
// Each one is both the thing and the way it is got, so struct transport_belt is
// its own recipe -- written as a struct rather than an alias, so that two items
// with the same wiki numbers stay two items, and so an item is complete the
// moment it is written, which is what makes a cycle unwritable. Its name is its
// name: typeid reads it back off the type, so nothing carries a string.
//
// Read an item as: made in <M>, <A> at a time, taking <T> seconds, from these
// inputs. The numbers are kept as the wiki writes them and the division
// happens where it is used. An ore patch has no inputs.
//
// A machine is an item as well, and so is a kilowatt. One craft holds its
// machine for as long as it takes, so an item's list of inputs is the ones it
// writes plus its machine, which it does not; a machine's list is its power
// alone, or nothing; a machine-second in turn costs
// kilowatts, a kilowatt costs steam, and steam costs fuel and water. So there
// is one graph, one kind of input, and asking it for a machine gives a machine
// count while asking it for electricity gives kilowatts -- the same question:
//
//     count_v<iron_plate, transport_belt>    plates in one belt
//     count_v<assembly, transport_belt>      machines for one a second
//     count_v<electricity, transport_belt>   kilowatts for one a second
//     count_v<boiler, transport_belt>        boilers for those kilowatts
//
// count_v is the only question, and it is two declarations: a primary that
// asks the output to fold, and a specialisation for being asked about itself.
// Both sides are template arguments the whole way down, and every answer is a
// double, so nothing is ever turned into an object or into a type to carry a
// number that was already a number.
//
// The ladder ends at fuel and water, because a burner drill and an offshore
// pump need no power. Mining that fuel with an electric drill instead would
// make electricity cost electricity, and that is the cycle this file cannot
// write.
//
// The bill of materials is a walk from the output, inputs before their user,
// that asks count_v once per item; an item already printed is a base of the
// object the walk carries, so a shared subtree is skipped, not reprinted. The
// local question is one multiplication: how many of an item a second, times
// what one of it takes of an input, is what that one link has to carry.
//
// Recipes are vanilla Factorio 2.0 (no Space Age), no modules anywhere. Where
// 1.1 differs it is noted at the recipe.
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>
#include <typeinfo>

// The item's own name, off the type. MSVC writes "struct iron_plate", the
// Itanium ABI writes the length in front of it as "10iron_plate", so a leading
// keyword and a leading count are both stepped over. Runtime only, which is
// all the bill of materials needs -- nothing here is a compile-time string.
template <class T> static const char* name() {
	const char* s = typeid(T).name();
	if (const char* keyword = std::strchr(s, ' ')) s = keyword + 1;
	while (*s >= '0' && *s <= '9') ++s;
	return s;
}

// Printing checks its format string against its arguments at compile time --
// std::format_string is consteval, so a stale {} is a build error where
// printf's %s was a crash. This is C++23's std::print spelled in C++20; on a
// standard bump the helper deletes and the calls stay.
template <class... A>
static void print(std::format_string<A...> f, A&&... a) {
	std::cout << std::vformat(f.get(), std::make_format_args(a...));
}

// An input, and how much of it. amount is what the fold multiplies by, so
// once a recipe has divided it is per one of the thing being made; wiki is
// the number as written, which rides along so printing it undoes nothing.
// They start out the same, and only item<> ever tells them apart.
template <class Item, double Amount = 1.0, double Wiki = Amount> struct in {
	using item = Item;
	static constexpr double amount = Amount;
	static constexpr double wiki = Wiki;
};

// How much of Input one Output costs, all the way down. Both sides are
// template arguments, so the two cases are a primary and a specialisation and
// nothing is ever made into an object to be matched on.
template <class Input, class Output>
constexpr double count_v = Output::template count_fold<Input>;

// asked about itself: one of it is one of it
template <class Same>
constexpr double count_v<Same, Same> = 1.0;

// What a thing costs: its inputs, and an amount of each per one of it. A
// machine is written this way and no other -- it is not made in a machine, so
// it has no machine, no craft to divide by, and nothing above it in the
// ladder. An empty node is the whole of what that means.
template <class... Each>
struct node {
	// sizeof demands a complete type, so an input must already be
	// defined -- which makes a cycle unwritable, without inheriting anything
	static_assert(((sizeof(typename Each::item) > 0) && ...),
		"an input is not defined yet -- items cannot form a cycle");

	using inputs = node;   // itself: a node is its inputs

	// The fold lives inside, because only a recipe knows its own inputs, and
	// it is a variable template like count_v -- which is where the remembering
	// happens: each pair is one specialisation and the compiler computes it
	// once. A constexpr function would be re-evaluated at every call and this
	// would be exponential.
	//
	// The + 0.0 is load-bearing, not just a terminator for ore patches and
	// machines: any branch that dead-ends without meeting the stop expands to
	// nothing, and nothing has to read as zero rather than as an error.
	template <class Input> static constexpr double count_fold =
		((Each::amount * count_v<Input, typename Each::item>) + ... + 0.0);
};

// Something made in a machine: the wiki's numbers, undivided, and the machine
// as an argument rather than a line. Its base is what one of it costs, so this
// is the one place anything is divided by how many a craft makes -- the machine
// included, since a craft holds it for Time, which at that machine's speed is
// Time/speed machine-seconds.
template <class Machine, int Amount, double Time, class... In>
struct item : node<in<typename In::item, In::amount / Amount, In::amount>...,
                   in<Machine, Time / (Amount * Machine::speed)>> {
	using machine = Machine;
	static constexpr int amount = Amount;
	static constexpr std::chrono::duration<double> time{ Time };
	static constexpr double occupies = Time / (Amount * Machine::speed);
};

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
// The power plant. None of these four needs electricity: a boiler and a burner
// drill burn fuel, a steam engine makes the power, an offshore pump needs
// nothing at all. That is what lets a kilowatt have a price.
// ---------------------------------------------------------------------------

struct offshore_pump : node<> { static constexpr double speed = 1.00; };
struct boiler        : node<> { static constexpr double speed = 1.00; };
struct steam_engine  : node<> { static constexpr double speed = 1.00; };
struct burner_drill  : node<> {
	// mining speed 0.25, less the 0.0375 a second it burns of what it mines
	static constexpr double speed = 0.2125 * (1 + mining_productivity);
};

struct water : item<offshore_pump, 1200, 1.0> {};
struct fuel  : item<burner_drill, 1, 1.0> {};   // coal, but off the power grid

// a boiler turns 1.8 MW of fuel into 60 steam a second, and coal is 4 MJ, so
// 0.45 coal. 1.1 took 60 water for that; 2.0.7 made the ratio 1:10
struct steam : item<boiler, 60, 1.0, in<fuel, 0.45>, in<water, 6.0>> {};

// a steam engine turns 30 steam a second into 900 kW
struct electricity : item<steam_engine, 900, 1.0, in<steam, 30.0>> {};

// ---------------------------------------------------------------------------
// The machines that burn fuel instead of taking power: what one second of one
// costs is its consumption over coal's 4 MJ. The fuel is the same one the
// boilers take, mined by a burner drill, which is what keeps a furnace out of
// the electric drill's chain and so out of a cycle.
// ---------------------------------------------------------------------------

template <double Consumption>
using burns = node<in<fuel, Consumption / 4000.0>>;

struct stone_furnace : burns<90.0> { static constexpr double speed = 1.00; };
struct steel_furnace : burns<90.0> { static constexpr double speed = 2.00; };

// ---------------------------------------------------------------------------
// The machines that do need it. The two numbers are what one second of it
// takes: energy consumption, then drain, which is charged whether it is
// working or not.
// ---------------------------------------------------------------------------

template <double Consumption, double Drain>
using consumes = node<in<electricity, Consumption + Drain>>;

struct assembly_1       : consumes<75.0, 2.5>   { static constexpr double speed = 0.50; };
struct assembly_2       : consumes<150.0, 5.0>  { static constexpr double speed = 0.75; };
struct assembly_3       : consumes<375.0, 12.5> { static constexpr double speed = 1.25; };
struct electric_furnace : consumes<180.0, 6.0>  { static constexpr double speed = 2.00; };
struct chemical_plant   : consumes<210.0, 7.0>  { static constexpr double speed = 1.00; };
struct refinery         : consumes<420.0, 14.0> { static constexpr double speed = 1.00; };
struct pumpjack         : consumes<90.0, 3.0>   {
	// mining speed 1 times the patch's yield: a 538% patch is 5.38, and the
	// recipe below is 10 crude a second, which is what 100% yields
	static constexpr double speed = 1.00 * (1 + mining_productivity);
};
struct electric_drill   : consumes<90.0, 3.0>   {
	static constexpr double speed = 0.50 * (1 + mining_productivity);
};
struct lab              : consumes<60.0, 2.0>   {
	static constexpr double speed = 1.00 * (1 + lab_research_speed);
};

// What is built.
using assembly = assembly_1;
using furnace  = electric_furnace;
using drill    = electric_drill;

// ---------------------------------------------------------------------------
// The recipes. In dependency order, because an item is complete when it is
// written and an input has to be complete to be named.
// ---------------------------------------------------------------------------

// ore patches: mining time is 1 second for all four vanilla ores
struct iron_ore     : item<drill, 1, 1.0> {};
struct copper_ore   : item<drill, 1, 1.0> {};
struct coal         : item<drill, 1, 1.0> {};
struct stone        : item<drill, 1, 1.0> {};
struct crude_oil    : item<pumpjack, 10, 1.0> {};

// smelting
struct iron_plate   : item<furnace, 1, 3.2, in<iron_ore>> {};
struct copper_plate : item<furnace, 1, 3.2, in<copper_ore>> {};
struct steel_plate  : item<furnace, 1, 16.0, in<iron_plate, 5.0>> {};
struct stone_brick  : item<furnace, 1, 3.2, in<stone, 2.0>> {};

// oil
struct petroleum_gas : item<refinery, 45, 5.0, in<crude_oil, 100.0>> {};
struct plastic_bar   : item<chemical_plant, 2, 1.0, in<coal>, in<petroleum_gas, 20.0>> {};
struct sulfur        : item<chemical_plant, 2, 1.0, in<water, 30.0>, in<petroleum_gas, 30.0>> {};

// intermediates
struct iron_gear          : item<assembly, 1, 0.5, in<iron_plate, 2.0>> {};
struct copper_cable       : item<assembly, 2, 0.5, in<copper_plate>> {};
struct electronic_circuit : item<assembly, 1, 0.5, in<iron_plate>, in<copper_cable, 3.0>> {};
struct pipe               : item<assembly, 1, 0.5, in<iron_plate>> {};
struct transport_belt     : item<assembly, 2, 0.5, in<iron_gear>, in<iron_plate>> {};
struct inserter           : item<assembly, 1, 0.5, in<electronic_circuit>, in<iron_gear>, in<iron_plate>> {};
struct advanced_circuit   : item<assembly, 1, 6.0, in<plastic_bar, 2.0>, in<electronic_circuit, 2.0>, in<copper_cable, 4.0>> {};
struct engine_unit        : item<assembly, 1, 10.0, in<steel_plate>, in<iron_gear>, in<pipe, 2.0>> {};

// military
struct wall         : item<assembly, 1, 0.5, in<stone_brick, 5.0>> {};
struct firearm_mag  : item<assembly, 1, 1.0, in<iron_plate, 4.0>> {};
// 2.0.46 made this 2 at a time in 6s from 2 mags and 2 copper;
// in 1.1 it is <1, 3.0, in<firearm_mag>, in<copper_plate, 5.0>, in<steel_plate>>
struct piercing_mag : item<assembly, 2, 6.0, in<firearm_mag, 2.0>, in<copper_plate, 2.0>, in<steel_plate>> {};
struct grenade      : item<assembly, 1, 8.0, in<coal, 10.0>, in<iron_plate, 5.0>> {};

// science
struct automation_pack : item<assembly, 1, 5.0, in<copper_plate>, in<iron_gear>> {};
struct logistic_pack   : item<assembly, 1, 6.0, in<inserter>, in<transport_belt>> {};
struct chemical_pack   : item<assembly, 2, 24.0, in<advanced_circuit, 3.0>, in<engine_unit, 2.0>, in<sulfur>> {};
struct military_pack   : item<assembly, 2, 10.0, in<grenade>, in<piercing_mag>, in<wall, 2.0>> {};

// One research unit is one of each pack the technology asks for, taking the
// technology's time. Edit the inputs to the packs your current research needs
// and the labs and their power fall out of the same graph as everything else.
struct research_unit : item<lab, 1, 30.0
	, in<automation_pack>
	//, in<logistic_pack>
	//, in<chemical_pack>
	//, in<military_pack>
> {};

// ---------------------------------------------------------------------------
// The wiki's numbers, as tests. Plates per pack is the standard "total raw"
// figure, which is what these check. Only materials: what a machine or a
// kilowatt costs depends on which machine is named above, so asserting it
// would break the moment that changes -- and the report prints it anyway.
// ---------------------------------------------------------------------------

// automation: 1 copper plate + 1 gear
static_assert(count_v<copper_plate, automation_pack> == 1);
static_assert(count_v<iron_plate, automation_pack> == 2);
static_assert(count_v<copper_ore, automation_pack> == 1);

// logistic: 1 inserter (4 iron, 1.5 copper) + 1 belt (1.5 iron)
static_assert(count_v<iron_plate, logistic_pack> == 5.5);
static_assert(count_v<copper_plate, logistic_pack> == 1.5);

// chemical: 2 at a time from 3 advanced circuits, 2 engines, 1 sulfur
static_assert(count_v<iron_plate, chemical_pack> == 12);
static_assert(count_v<copper_plate, chemical_pack> == 7.5);
static_assert(count_v<coal, chemical_pack> == 1.5);
static_assert(count_v<petroleum_gas, chemical_pack> == 37.5);

// military: 2 at a time from 1 grenade, 1 piercing mag, 2 walls
static_assert(count_v<iron_plate, military_pack> == 5.75);
static_assert(count_v<copper_plate, military_pack>
	== 2.0 / piercing_mag::amount / military_pack::amount);
static_assert(count_v<coal, military_pack> == 10.0 / military_pack::amount);        // 10 a grenade
static_assert(count_v<stone, military_pack> == 2 * 5 * 2.0 / military_pack::amount);  // 2 walls, 5 brick, 2 stone

// nothing leaks sideways
static_assert(count_v<iron_plate, copper_plate> == 0);
static_assert(count_v<copper_plate, iron_plate> == 0);
static_assert(count_v<coal, automation_pack> == 0);

// the old regression case: a belt is made 2 at a time from 1 gear -- which is
// 2 plates -- and 1 more plate, so 1.5 plates each
static_assert(count_v<iron_plate, transport_belt> == 3.0 / transport_belt::amount);

// ---------------------------------------------------------------------------
// The bill of materials: a walk from the output, inputs before their user, so
// everything prints in the order a factory is built. The same item is reached
// by many paths, so the walk carries what it has already printed as an object
// with one tag<> base per item -- and whether the next item is in is not a
// question a trait answers but one overload resolution does: the object's own
// pointer converts to tag<I>* exactly when I has been printed, and a
// conversion to a base outranks the one to void*, so the visited case is
// simply the overload that wins.
//
// An item found in the object was printed with everything beneath it, so its
// whole subtree is skipped -- which is what makes the walk linear. The object
// grows by one base on the way out of each first visit, and it grows in the
// return value, because an object cannot change its type in place.
//
// The one walk is the whole report: an item prints its line, its recipe as
// the wiki writes it, and what feeds it, the moment everything beneath it
// has; a machine prints how many of it are running instead. Which of the two
// a thing is is not a flag it carries, it is what it is: a machine is the
// thing with no machine of its own.
// ---------------------------------------------------------------------------

template <class I> struct tag {};

template <class... Have> struct seen : tag<Have>... {
	// unconditional, because the only caller is the overload that just
	// established I is not in -- the other one took every case where it was
	template <class I>
	friend constexpr seen<Have..., I> operator+(seen, tag<I>) { return {}; }
};

template <class I> constexpr bool made = requires { typename I::machine; };

// one link: what it carries, which is how many of this item a second times
// what one of it takes of that input. Both are already per one, so there is
// nothing to divide here either. There is no into column -- a link prints
// under the line of the item it feeds.
template <class Output, class Input, class In>
static void feed(double rate) {
	if constexpr (made<Input>) {
		constexpr double each = count_v<Input, Output> * In::amount;
		if (each == 0) return;
		print("      {:<20} {:>8.3f}\n", name<typename In::item>(), each * rate);
	}
}

template <class Output, class Input, class... In>
static void feeds_of(node<In...>, [[maybe_unused]] double rate) {
	(feed<Output, Input, In>(rate), ...);
}

// the recipe as the wiki writes it, straight off each input's wiki number.
// The machine is not an ingredient the wiki lists -- it is the craft this
// prints under -- and made<> is what tells them apart.
template <class In>
static void ingredient(const char*& sep) {
	if constexpr (made<typename In::item>) {
		print("{}{:g} {}", sep, In::wiki, name<typename In::item>());
		sep = ", ";
	}
}

template <class Item, class... In>
static void recipe(node<In...>) {
	print("      {} every {:g} s", Item::amount, Item::time.count());
	const char* sep = "  from  ";
	(ingredient<In>(sep), ...);
	print("\n");
}

template <class Input, class Output>
static void line(double rate) {
	constexpr double each = count_v<Input, Output>;
	if (each == 0) return;
	const double need = each * rate;

	if constexpr (made<Input>) {
		// this item's own machines, and what they take: the machine count is
		// the machine-seconds one of these needs, and the draw is that machine
		// asked the same question
		const double running = Input::occupies * need;
		const double power = running * count_v<electricity, typename Input::machine>;
		print("  {:<20} {:>10.3f}  {:<17} {:>8.2f} {:>9.1f}\n",
			name<Input>(), need, name<typename Input::machine>(), running, power);
		recipe<Input>(typename Input::inputs{});
		feeds_of<Output, Input>(typename Input::inputs{}, rate);
	} else {
		print("  {:<20} {:>10.2f} running\n", name<Input>(), need);
	}
}

// the walk is two overloads of visiting one item and two of crossing an input
// list, and the object threads through them left to right. Declared before
// defined, because each pair calls the other.
template <class Output, class S>
static S across(node<>, S, double);
template <class Output, class First, class... Rest, class S>
static auto across(node<First, Rest...>, S, double);

// seen already: nothing to do, and nothing was tested -- that the argument
// converted to this parameter is the answer
template <class Output, class I, class... Have>
static seen<Have...> visit(seen<Have...> s, tag<I>*, double) { return s; }

// not yet: inputs first -- the machine is one of them -- then its own line,
// and I joins the object on the way out
template <class Output, class I, class... Have>
static auto visit(seen<Have...> s, void*, double rate) {
	auto grown = across<Output>(typename I::inputs{}, s, rate);
	line<I, Output>(rate);
	return grown + tag<I>{};
}

template <class Output, class S>
static S across(node<>, S s, double) { return s; }

template <class Output, class First, class... Rest, class S>
static auto across(node<First, Rest...>, S s, double rate) {
	auto grown = visit<Output, typename First::item>(s, &s, rate);
	return across<Output>(node<Rest...>{}, grown, rate);
}

template <class Output>
static void bill(double rate) {
	seen<> nothing;
	print("\n{}, {:g} per second\n", name<Output>(), rate);
	print("  {:<20} {:>10}  {:<17} {:>8} {:>9}\n", "item", "per second", "machine", "count", "kW");
	print("  ----------------------------------------------------------------------\n");
	visit<Output, Output>(nothing, &nothing, rate);
	print("  ----------------------------------------------------------------------\n");
	print("  {:<59} {:>9.1f}  = {:.2f} MW\n", "total",
		count_v<electricity, Output> * rate, count_v<electricity, Output> * rate / 1000);
}

int main() {
	print("{} {:g}   {} {:g}   {} {:g}   lab {:g}\n",
		name<assembly>(), assembly::speed, name<furnace>(), furnace::speed,
		name<drill>(), drill::speed, lab::speed);
	print("mining productivity +{:g}%   lab research speed +{:g}%\n",
		mining_productivity * 100, lab_research_speed * 100);

	bill<research_unit>(1.0);
}
