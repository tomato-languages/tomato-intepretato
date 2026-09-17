#include <lib/interpreter.h>
#include <gtest/gtest.h>



TEST(StdlibTests, NumericFunctions) {
    std::string code = R"(
        println(abs(-5))
        println(ceil(1.2))
        println(floor(1.8))
        println(round(1.4))
        println(round(1.6))
        println(sqrt(9))
        println(parse_num("123"))
        println(parse_num("invalid"))
        println(to_string(3.14))
    )";

    std::string expected =
        "5\n" 
        "2\n"
        "1\n"
        "1\n"
        "2\n"
        "3\n"
        "123\n"
        "nullptr\n"
        "3.140000\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StdlibTests, StringFunctions) {
    std::string code = R"(
        println(len("ITMo"))
        println(lower("ITMo"))
        println(upper("ITMo"))
        l = split("I,T,M,O", ",")
        println(join(l, "-"))
        println(replace("239_ITMO", "_", "->"))
    )";

    std::string expected =
        "4\n"
        "itmo\n"
        "ITMO\n"
        "I-T-M-O\n"
        "239->ITMO\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StdlibTests, ListFunctions) {
    std::string code = R"(
        l = [1, 3]
        push(l, 2)
        println(l[1])
        println(l[2])
        println(pop(l))

        l2 = [1, 3]
        insert(l2, 1, 2)
        println(l2[1])
        println(remove(l2, 1))

        l3 = [3, 2, 9]
        l3 = sort(l3)
        println(l3[0])
        println(l3[1])
        println(l3[2])

        l4 = range(5)
        for i in range(5)
            print(l4[i])
        end for
    )";

    std::string expected =
        "3\n"
        "2\n"
        "2\n"
        "2\n"
        "[1, 3]\n"
        "2\n"
        "3\n"
        "9\n"
        "01234";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(LoopTestSuit, ForBreakTest) {
    std::string code = R"(
        for i in range(0,5,1)
            if (i == 3) then
                break
            end if
            print(i)
        end for
    )";
    
    std::string expected = "012";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(LoopTestSuit, WhileInt) {
    std::string code = R"(
        x = 0
        while  (x < 5)
            x = x + 1
        end while
        print(x)
    )";

   std::string expected = "5";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(LoopTestSuit, WhileContinueTest) {
    std::string code = R"(
        x = 0
        y = 0
        while  (x < 5)
            x = x + 1
            if (x % 2 == 0) then
                continue
            end if
            y = y + 1            
        end while
        print(y)
    )";

   std::string expected = "3";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}



TEST(LoopTestSuit, NilIfTest) {
    std::string code = R"(
        x = nil
        if x then
            print("yes")
        else
            print("no")
        end if
    )";
    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_ANY_THROW(interpret(input, output));
}


TEST(LoopTestSuit, RangeNegativeStepTest) {
    std::string code = R"(
        l = range(5, 0, -1)
        for i in l
            print(i)
        end for
    )";
    std::string expected = "54321";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}



TEST(LoopTestSuit, EmptyForTest) {
    std::string code = R"(
        for x in []
            print(1)
        end for
    )";
    std::string expected = "";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(ListTests, IndexTest) {
    std::string code = R"(
        l = [1, 2, 3, 4, 5, 6, 7, 8, 9]

        print(l[1])
        print(l[2])
        print(l[8])
    )";

    std::string expected = "239";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, SliceTest) {
    std::string code = R"(
        l = [1, 2, 3, 4, 5, 6, 7, 8, 9]

        print(l[1:3])
        print(l[8:9])
    )";

    std::string expected = "[2, 3][9]";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, IndexTest) {
    std::string code = R"(
        s = "ITMO239ITMO"

        print(s[0])
        print(s[1])
        print(s[4])
        print(s[5])
        print(s[6])
    )";

    std::string expected = "IT239";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, SliceTest) {
    std::string code = R"(
        s = "ITMO239ITMO239"

        print(s[0:4])
        println(s[11:14])
        println(s[13:])
        println(s[:1])
    )";

    std::string expected = "ITMO239\n9\nI\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, MultTest) {
    std::string code = R"(
        s = "ITMO"
        s = s * 2
        k = s * 0
        println(s)
        println(k)
    )";

    std::string expected = "ITMOITMO\n\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, PlusMinusStrTest) {
    std::string code = R"(
        s = "ITMO"
        t = s + "239"
        println(t)
        t = t - "239"
        print(t)
    )";

    std::string expected = "ITMO239\nITMO";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, CompStrTest) {
    std::string code = R"(
        s = "ITMO"
        t = "NOTITMO"
        r = "ITMO"
        print(s < t)
        print(s <= t)
        print(s > t)
        print(s >= t)
        print(s == r)
        print(s != r)
    )";

    std::string expected = "110010";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, 2listsTest) {
    std::string code = R"(
        l = [1, 2, 3]
        l2 = l
        push(l, 9)
        push(l2, 100)
        println(l[3])
        println(l2[3])
        println(l[4])
        println(l2[4])
    )";

    std::string expected = "9\n9\n100\n100\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, listSortTest) {
    std::string code = R"(
        l = [1, 2, "itmo", 3, "hi"]
        l = sort(l)
        for i in l 
            println(i)
        end for
    )";

    std::string expected = "1\n2\n3\nhi\nitmo\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, NegativeIndexTest) {
    std::string code = R"(
        lst = [10, 20, 30]
        print(lst[-1])
    )";
    std::string expected = "30";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(StrTests, SliceStrBeyondBoundsTest) {
    std::string code = R"(
        s = "abc"
        print(s[1:10])
    )";
    std::string expected = "bc";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, EmptyListTest) {
    std::string code = R"(
        l = []
        pop(l)
    )";
    std::string expected = "";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_ANY_THROW(interpret(input, output));
}

TEST(ListTests, NegativeSliceTest) {
    std::string code = R"(
        l = [10,20,30,40]
        s = l[-3:-1]
        print(s[0])
        print(s[1])
    )";
    std::string expected = "2030";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(StrTests, EscapeStrTest) {
    std::string code = R"(
        s = "\"ITMO\" University"
        print(s)
    )";
    std::string expected = R"("ITMO" University)";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}


TEST(ListTests, MatrixTest) {
    std::string code = R"(
        m = [[1,2],[3,4],[5,6]]
        print(m[1][1])
    )";
    std::string expected = "4";

    std::istringstream input(code);
    std::ostringstream output;
    
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}
