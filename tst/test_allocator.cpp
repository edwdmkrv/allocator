#include "lib.hpp"

#include <gtest/gtest.h>

#include <cstdint>

template <typename X, std::size_t Size, std::size_t Extra>
void TestVariousSizesAllocation() noexcept try {
	AllocatorN<X, Size> allocator;
	X *prev{allocator.allocate(1)};

	unsigned n{};

	static X *array[Size + Extra];

	array[n] = prev;

	/* Memory is allocated from the special buffer, sequential addresses, next == prev + 1 */
	while (++n < Size) {
		X *const next{allocator.allocate(1)};

		EXPECT_EQ(next, prev + 1);
		array[n] = prev = next;
	}

	/* The special buffer space is over, memory is allocated from malloc, next != prev + 1 for relatively small types */
	while (++n < Size + Extra) {
		X *const next{allocator.allocate(1)};

		EXPECT_NE(next, prev + 1);
		array[n] = prev = next;
	}

	while (n) {
		allocator.deallocate(array[--n], 1);
	};
} catch (...) {
	EXPECT_TRUE(false);
}

TEST(Allocator, TestUint8TAllocation) {
	TestVariousSizesAllocation<uint8_t, 5, 3>();
	TestVariousSizesAllocation<uint8_t, 255, 333>();
}

TEST(Allocator, TestUint16TAllocation) {
	TestVariousSizesAllocation<uint16_t, 5, 3>();
	TestVariousSizesAllocation<uint16_t, 256, 333>();
	TestVariousSizesAllocation<uint16_t, 65'535, 33'333>();
}

TEST(Allocator, TestUint32TAllocation) {
	TestVariousSizesAllocation<uint32_t, 5, 3>();
	TestVariousSizesAllocation<uint32_t, 256, 333>();
	TestVariousSizesAllocation<uint32_t, 65'536, 33'333>();
	TestVariousSizesAllocation<uint32_t, 5'555'555, 3'333'333>();
}

TEST(Allocator, TestSizeTAllocation) {
	TestVariousSizesAllocation<std::size_t, 5, 3>();
	TestVariousSizesAllocation<std::size_t, 256, 333>();
	TestVariousSizesAllocation<std::size_t, 65'536, 33'333>();
	TestVariousSizesAllocation<std::size_t, 5'555'555, 3'333'333>();
}

template <typename X, std::size_t Size, std::size_t Extra>
void TestVariousSizesSourceOfAllocation() noexcept try {
	AllocatorN<X, Size> allocator;
	X *prev{allocator.allocate(1)};

	unsigned n{};

	static X *array[2 * (Size + Extra)];

	array[n++] = prev;

	/* Each second time memory is allocated from the special buffer, sequential addresses, next2 == prev + 1 */
	while (n < 2 * Size - 1) {
		X *const next1{allocator.allocate(2)};
		X *const next2{allocator.allocate(1)};

		EXPECT_NE(next1, prev + 1);
		array[n++] = next1;

		EXPECT_EQ(next2, prev + 1);
		array[n++] = prev = next2;
	}

	/* The special buffer space is over, memory is allocated from malloc, next2 != prev + 1 for relatively small types */
	while (n < 2 * (Size + Extra) - 1) {
		X *const next1{allocator.allocate(2)};
		X *const next2{allocator.allocate(1)};

		EXPECT_NE(next1, prev + 1);
		array[n++] = next1;

		EXPECT_NE(next2, prev + 1);
		array[n++] = prev = next2;
	}

	while (n) {
		allocator.deallocate(array[--n], 1);
	};
} catch (...) {
	EXPECT_TRUE(false);
}

TEST(Allocator, TestUint8TSourceOfAllocation) {
	TestVariousSizesSourceOfAllocation<uint8_t, 5, 3>();
	TestVariousSizesSourceOfAllocation<uint8_t, 255, 333>();
}

TEST(Allocator, TestUint16TSourceOfAllocation) {
	TestVariousSizesSourceOfAllocation<uint16_t, 5, 3>();
	TestVariousSizesSourceOfAllocation<uint16_t, 256, 333>();
	TestVariousSizesSourceOfAllocation<uint16_t, 65'535, 33'333>();
}

TEST(Allocator, TestUint32TSourceOfAllocation) {
	TestVariousSizesSourceOfAllocation<uint32_t, 5, 3>();
	TestVariousSizesSourceOfAllocation<uint32_t, 256, 333>();
	TestVariousSizesSourceOfAllocation<uint32_t, 65'536, 33'333>();
	TestVariousSizesSourceOfAllocation<uint32_t, 5'555'555, 3'333'333>();
}

TEST(Allocator, TestSizeTSourceOfAllocation) {
	TestVariousSizesSourceOfAllocation<std::size_t, 5, 3>();
	TestVariousSizesSourceOfAllocation<std::size_t, 256, 333>();
	TestVariousSizesSourceOfAllocation<std::size_t, 65'536, 33'333>();
	TestVariousSizesSourceOfAllocation<std::size_t, 5'555'555, 3'333'333>();
}

int main(int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
