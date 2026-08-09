void mainLambda()
{
	double iron = 0;
	auto sweep0 = [&] {};

	double gear = 0;
	auto sweep1 = [&] { iron += gear; sweep0(); };

	double belt = 0;
	auto sweep2 = [&] { gear += belt; iron += belt; sweep1(); };

	belt += 10;
	sweep2();
}