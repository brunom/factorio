#include <chrono>
using namespace std;
using namespace std::chrono_literals;
 
void main2()
{
	// Convert to seconds up front if wiki uses speed.
	// Use variables to mention each constant once.

	constexpr auto science_all = 1;
	constexpr auto science_seconds = 10;
	constexpr auto science_lab = science_all * science_seconds;

	constexpr auto science_red = science_all * 2;
	constexpr auto science_red_assembly = science_red * 5;

	constexpr auto science_green = science_all * 0;
	constexpr auto science_green_assembly = science_green * 6;



	constexpr auto inserter
		= science_green;
	constexpr auto inserter_assembly = inserter * 0.5s;

	constexpr auto belt
		= science_green;
	constexpr auto belt_mult = 2;
	constexpr auto belt_assembly = belt * 0.5s / belt_mult;

	constexpr auto circuit
		= inserter;
	constexpr auto circuit_assembly = circuit * 0.5s;

	constexpr auto gear
		= science_red
		+ inserter
		+ belt / belt_mult;
	constexpr auto gears_assembly = gear * 0.5s;

	constexpr auto cable =
		circuit * 3;
	constexpr auto cable_mult = 2;
	constexpr auto cable_assembly = cable * 0.5s / cable_mult;

	constexpr auto copper
		= science_red
		+ cable / cable_mult;

	constexpr auto iron
		= gear * 2
		+ inserter
		+ circuit
		+ belt / belt_mult;



	constexpr auto furnace_stone_seconds = 3.2s;
	constexpr auto furnace_steel_seconds = 1.6s;
	constexpr auto furnace_electric_seconds = 1.6s;
	constexpr auto furnace_seconds = furnace_stone_seconds;
	constexpr auto furnace_iron = iron * furnace_seconds;
	constexpr auto furnace_copper = copper * furnace_seconds;

	constexpr auto drill_burner_seconds = 1 / 0.25;
	constexpr auto drill_electric_seconds = 1 / 0.5;
	constexpr auto drill_seconds = drill_electric_seconds;
	constexpr auto drill_iron = iron * drill_seconds;
	constexpr auto drill_copper = copper * drill_seconds;
}