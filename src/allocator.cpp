#include <map>
#include <type_traits>
#include <iostream>

#include "lib.hpp"

template <typename X>
static constexpr X factorial(X const n) noexcept {
	static_assert(std::is_integral<X>::value,
		"The value of an integral type is expected for the argument of function factorial"
	);

	static_assert(std::is_unsigned<X>::value,
		"The value of an unsigned type is expected for the argument of function factorial"
	);

	return n == 0 ? 1 : n * factorial(n - 1);
}

template <typename X, template <typename> typename A>
static void map() {
	static_assert(std::is_integral<X>::value,
		"The value of an integral type is expected for the template argument of function map"
	);

	enum: X {start = 0, end = 10};

	std::map<X, X, std::less<X>, A<std::pair<X const, X>>> map;

	for (X n{start}; n < end; n++) {
		map.emplace(n, factorial(n));
	}

	if constexpr (!std::is_same<A<X>, std::allocator<X>>::value) {
		for (auto const &pair: map) {
			std::cout << pair.first << ' ' << pair.second << std::endl;
		}
	}
}

template <typename X, template <typename> typename A>
static void container() {
	static_assert(std::is_integral<X>::value,
		"The value of an integral type is expected for the template argument of function container"
	);

	enum: X {start = 0, end = 10};

	Container<X, A<X>> container{};

	for (X n{start}; n < end; n++) {
		container.emplace_back(n);
	}

	if constexpr (!std::is_same<A<X>, std::allocator<X>>::value) {
		for (auto const &item: container) {
			std::cout << item << std::endl;
		}
	}
}

template <typename X>
using Allocator = AllocatorN<X, 10>;

int main() try {
	std::cout.exceptions(std::ostream::badbit | std::ostream::failbit | std::ostream::eofbit);

	map<std::size_t, std::allocator>();
	map<std::size_t, Allocator>();

	container<int, std::allocator>();
	container<int, Allocator>();

	return EXIT_SUCCESS;
} catch (std::exception const &e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return EXIT_FAILURE;
} catch (...) {
	std::cerr << "Error: " << "unidentified exception" << std::endl;
	return EXIT_FAILURE;
}
