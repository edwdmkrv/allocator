#include "container.hpp"

#include <gtest/gtest.h>

TEST(Container, InitialValues) {
	Container<int> container;

	EXPECT_TRUE(container.empty());
	EXPECT_EQ(container.capacity(), 0);
	EXPECT_EQ(container.size(), 0);
	EXPECT_EQ(container.begin(), container.end());
}

TEST(Container, SingleValue) {
	Container<int> container;

	container.emplace_back(5);

	EXPECT_FALSE(container.empty());
	EXPECT_EQ(container.capacity(), 1);
	EXPECT_EQ(container.size(), 1);
	EXPECT_NE(container.begin(), container.end());

	std::size_t counter{};

	for ([[maybe_unused]] auto const &item: container) {
		counter++;
	}

	EXPECT_EQ(counter, 1);
	EXPECT_EQ(container.at(0), 5);
}

TEST(Container, Reserve) {
	Container<int> container;

	container.reserve(10);

	EXPECT_TRUE(container.empty());
	EXPECT_EQ(container.capacity(), 10);
	EXPECT_EQ(container.size(), 0);
	EXPECT_EQ(container.begin(), container.end());

	for (int i{1}; i < 9; i++) {
		container.emplace_back(i * i);
	}

	EXPECT_FALSE(container.empty());
	EXPECT_EQ(container.capacity(), 10);
	EXPECT_EQ(container.size(), 8);
	EXPECT_NE(container.begin(), container.end());

	std::size_t counter{};

	for ([[maybe_unused]] auto const &item: container) {
		counter++;
	}

	EXPECT_EQ(counter, 8);

	for (int i{1}, pos{}; i < 9; i++, pos++) {
		EXPECT_EQ(container.at(pos), i * i);
	}
}

TEST(Container, ReserveStress) {
	enum: std::size_t {num = 55'555'555};

	Container<std::size_t> container;

	container.reserve(num);

	for (std::size_t n{}; n < num; n++) {
		container.emplace_back(n);
	}
}

TEST(Container, NoReserveStress) {
	enum: std::size_t {num = 55'555'555};

	Container<std::size_t> container;

	for (std::size_t n{}; n < num; n++) {
		container.emplace_back(n);
	}
}

int main(int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
