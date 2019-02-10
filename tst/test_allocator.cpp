#include "lib.hpp"

#include <gtest/gtest.h>

struct Data {
};

struct TestLibrary: testing::Test, testing::WithParamInterface<Data> {
};

TEST_P(TestLibrary, TestFunctionSplit) {
	Data const data{GetParam()};

	EXPECT_TRUE(&data);
}

INSTANTIATE_TEST_CASE_P(SimpleDataset, TestLibrary,
	testing::Values(
		Data{
		},
		Data{
		}
	),
);

int main(int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
