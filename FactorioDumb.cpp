int main()
{
	// Convert to seconds up front if wiki uses speed.
	// Use variables to mention each constant once.

	constexpr double science_all = 1;
	constexpr double science_seconds = 10;
	constexpr double science_lab = science_all * science_seconds;

	constexpr double science_red = science_all * 0;
	constexpr double science_red_assembly = science_red * 5;

	constexpr double science_green = science_all * 1;
	constexpr double science_green_assembly = science_green * 6;



	constexpr double inserter
		= science_green;
	constexpr double inserter_assembly = inserter * 0.5;

	constexpr double belt
		= science_green;
	constexpr double belt_mult = 2;
	constexpr double belt_assembly = belt * 0.5 / belt_mult;

	constexpr double circuit
		= inserter;
	constexpr double circuit_assembly = circuit * 0.5;

	constexpr double gears
		= science_red
		+ inserter
		+ belt / belt_mult;
	constexpr double gears_assembly = gears * 0.5;

	constexpr double cable =
		circuit * 3;
	constexpr double cable_mult = 2;
	constexpr double cable_assembly = cable * 0.5 / cable_mult;

	constexpr double copper
		= science_red
		+ cable / cable_mult;

	constexpr double iron
		= gears * 2
		+ inserter
		+ circuit
		+ belt / belt_mult;



	constexpr double furnace_stone_seconds = 3.2;
	constexpr double furnace_steel_seconds = 1.6;
	constexpr double furnace_electric_seconds = 1.6;
	constexpr double furnace_seconds = furnace_stone_seconds;
	constexpr double furnace_iron = iron * furnace_seconds;
	constexpr double furnace_copper = copper * furnace_seconds;

	constexpr double drill_burner_seconds = 1 / 0.25;
	constexpr double drill_electric_seconds = 1 / 0.5;
	constexpr double drill_seconds = drill_burner_seconds;
	constexpr double drill_iron = iron * drill_seconds;
	constexpr double drill_copper = copper * drill_seconds;
}