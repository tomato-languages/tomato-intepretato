#include <lib/interpreter.h>
#include <gtest/gtest.h>


TEST(TypesTestSuite, IntTest) {
    std::string code = R"(
        x = 1
        y = 2
        z = 3 * x + y
        print(z)
    )";

    std::string expected = "5";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(TypesTestSuite, ExpNotanionTest) {
    std::string code = R"(
        x = 1e+3
        y = 2.e+3
        z = 3.0e+3
        f = 4.0E+3
        println(x)
        println(y)
        println(z)
        println(f)
    )";

    std::string expected = "1000\n2000\n3000\n4000\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(TypesTestSuite, FunctionTest) {
    std::string code = R"(
        f = function(x)
            return x * 2
        end function
        newf = f
        print(newf(3))
    )";
    std::string expected = "6";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}