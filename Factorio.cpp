//#include <cstddef>
//
//template <typename T>
//struct recipe
//{
//
//};
//
//struct recipe_list
//{
//	recipe_list(nullptr_t) {}
//};
//
//template<typename T>
//recipe_list operator+(const recipe_list& lhs, const recipe<T>& rhs)
//{
//	return recipe_list(nullptr);
//}
//
//int main()
//{
//	auto recipes =
//		nullptr
//		+ recipe<struct red_science, struct copper_plate, struct gear>()
//		+ recipe<struct gear, struct iron_plate>()
//		;
//}