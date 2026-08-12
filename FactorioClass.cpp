struct base
{
	void sweep() {};
};
struct iron : base
{
	double amount = 0;
	void sweep() { __super::sweep(); };
};
struct gear : iron
{
	double amount = 0;
	void sweep()
	{
		iron::amount += amount;
		__super::sweep();
	}
};
struct belt : gear
{
	double amount = 0;
	void sweep()
	{
		gear::amount += amount;
		iron::amount += amount;
		__super::sweep();
	}
};
struct cookbook : belt {};

void mainClass()
{
	cookbook cb;
	cb.belt::amount += 10;
	cb.sweep();
}