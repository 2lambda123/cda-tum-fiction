//
// Created by Jan Drewniok on 19.04.23.
//

#include <catch2/catch_template_test_macros.hpp>

#include <fiction/utils/math_utils.hpp>

using namespace fiction;

TEST_CASE("round_to_n_decimal_places should round an input number to n decimal places", "[round_to_n_decimal_places]")
{
    SECTION("int64_t")
    {
        CHECK(round_to_n_decimal_places(-1LL, 0) == -1LL);
        CHECK(round_to_n_decimal_places(-1LL, 10) == -1LL);
        CHECK(round_to_n_decimal_places(1LL, 0) == 1LL);
        CHECK(round_to_n_decimal_places(1LL, 10) == 1LL);
    }

    SECTION("double")
    {
        const double value_positive = 3.145926;
        CHECK(round_to_n_decimal_places(value_positive, 0) == 3);
        CHECK(round_to_n_decimal_places(value_positive, 1) == 3.1);
        CHECK(round_to_n_decimal_places(value_positive, 2) == 3.15);
        CHECK(round_to_n_decimal_places(value_positive, 3) == 3.146);
        CHECK(round_to_n_decimal_places(value_positive, 4) == 3.1459);
        CHECK(round_to_n_decimal_places(value_positive, 5) == 3.14593);
        CHECK(round_to_n_decimal_places(value_positive, 6) == 3.145926);

        const double value_negative = -3.145926;
        CHECK(round_to_n_decimal_places(value_negative, 0) == -3);
        CHECK(round_to_n_decimal_places(value_negative, 1) == -3.1);
        CHECK(round_to_n_decimal_places(value_negative, 2) == -3.15);
        CHECK(round_to_n_decimal_places(value_negative, 3) == -3.146);
        CHECK(round_to_n_decimal_places(value_negative, 4) == -3.1459);
        CHECK(round_to_n_decimal_places(value_negative, 5) == -3.14593);
        CHECK(round_to_n_decimal_places(value_negative, 6) == -3.145926);
    }

    SECTION("Edge cases")
    {
        CHECK(round_to_n_decimal_places(1.005, 2) == 1.0);
        CHECK(round_to_n_decimal_places(0.000001, 6) == 0.000001);
        CHECK(round_to_n_decimal_places(0.0000001, 6) == 0);
        CHECK(round_to_n_decimal_places(-0.000001, 6) == -0.000001);
        CHECK(round_to_n_decimal_places(-0.0000001, 6) == 0);
    }
}

TEMPLATE_TEST_CASE("integral_abs should compute the absolute value of a number of different integral types",
                   "[integral_abs]", int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t)
{
    const auto x = static_cast<TestType>(-42);

    CHECK(integral_abs(integral_abs(x) - (x < 0 ? -x : x)) == 0);
}

TEST_CASE("Binomial Coefficient Tests")
{
    SECTION("C(5, 2)")
    {
        const uint64_t result = binomial_coefficient(5, 2);
        REQUIRE(result == 10);  // C(5, 2) = 10
    }

    SECTION("C(10, 3)")
    {
        const uint64_t result = binomial_coefficient(10, 3);
        REQUIRE(result == 120);  // C(10, 3) = 120
    }

    SECTION("C(0, 0)")
    {
        const uint64_t result = binomial_coefficient(0, 0);
        REQUIRE(result == 1);  // C(0, 0) = 1
    }

    SECTION("C(8, 8)")
    {
        const uint64_t result = binomial_coefficient(8, 8);
        REQUIRE(result == 1);  // C(8, 8) = 1
    }

    SECTION("C(7, 10)")
    {
        const uint64_t result = binomial_coefficient(7, 10);
        REQUIRE(result == 0);  // C(7, 10) = 0 (k > n)
    }
    SECTION("C(15, 7)")
    {
        const uint64_t result = binomial_coefficient(15, 7);
        REQUIRE(result == 6435);  // C(15, 7) = 6435
    }

    SECTION("C(20, 10)")
    {
        const uint64_t result = binomial_coefficient(20, 10);
        REQUIRE(result == 184756);  // C(20, 10) = 184,756
    }

    SECTION("C(30, 15)")
    {
        const uint64_t result = binomial_coefficient(30, 15);
        REQUIRE(result == 155117520);  // C(30, 15) = 155,117,520
    }

    SECTION("C(50, 25)")
    {
        const uint64_t result = binomial_coefficient(50, 25);
        REQUIRE(result == 126410606437752);  // C(50, 25) = 126,410,606,437,752
    }
}

TEST_CASE("Generate Combinations Test")
{
    SECTION("Generating combinations of 3 elements from 5.")
    {
        const auto result = generate_unsigned_integer_combinations(3, 5);
        REQUIRE(result.size() == 10);  // There should be 10 combinations.
    }

    SECTION("Test case 2: Generating combinations of 2 elements from 4.")
    {
        const auto result = generate_unsigned_integer_combinations(2, 4);
        REQUIRE(result.size() == 6);  // There should be 6 combinations.
        CHECK(result[0].size() == 2);
        CHECK(result[2].size() == 2);
        CHECK(result[3].size() == 2);
        CHECK(result[4].size() == 2);
        CHECK(result[5].size() == 2);
    }

    SECTION("Generating combinations of 1 element from 3.")
    {
        const auto result = generate_unsigned_integer_combinations(1, 3);
        REQUIRE(result.size() == 3);  // There should be 3 combinations.
        CHECK(result[0].size() == 1);
        CHECK(result[1].size() == 1);
        CHECK(result[2].size() == 1);

        CHECK(result[0][0] == 0);
        CHECK(result[1][0] == 1);
        CHECK(result[2][0] == 2);
    }

    SECTION("Generating combinations of 0 elements from 2.")
    {
        const auto result = generate_unsigned_integer_combinations(0, 2);
        REQUIRE(result.size() == 1);  // There should be 1 combination (empty set).
        CHECK(result.front().empty());
    }

    SECTION("Generating combinations of 5 elements from 5.")
    {
        const auto result = generate_unsigned_integer_combinations(5, 5);
        REQUIRE(result.size() == 1);  // There should be 1 combination (all elements).
        REQUIRE(result.front().size() == 5);
        CHECK(result.front()[0] == 0);
        CHECK(result.front()[1] == 1);
        CHECK(result.front()[2] == 2);
        CHECK(result.front()[3] == 3);
        CHECK(result.front()[4] == 4);
    }

    SECTION("Generating combinations of 0 elements from 0.")
    {
        const auto result = generate_unsigned_integer_combinations(0, 0);
        REQUIRE(result.size() == 1);  // There should be 1 combination (empty set).
        CHECK(result.front().empty());
    }

    SECTION("Generating combinations of 3 elements from 20.")
    {
        const auto result = generate_unsigned_integer_combinations(3, 20);
        REQUIRE(result.size() == 1140);  // There should be 1 combination (all elements).
        REQUIRE(result.front().size() == 3);
        CHECK(result.front()[0] == 0);
        CHECK(result.front()[1] == 1);
        CHECK(result.front()[2] == 2);

        REQUIRE(result.back().size() == 3);
        CHECK(result.back()[0] == 17);
        CHECK(result.back()[1] == 18);
        CHECK(result.back()[2] == 19);
    }

    SECTION("Generating combinations of 3 elements from 200.")
    {
        const auto result = generate_unsigned_integer_combinations(3, 200);
        REQUIRE(result.size() == 1313400);  // There should be 1 combination (all elements).
        REQUIRE(result.front().size() == 3);
        CHECK(result.front()[0] == 0);
        CHECK(result.front()[1] == 1);
        CHECK(result.front()[2] == 2);

        REQUIRE(result.back().size() == 3);
        CHECK(result.back()[0] == 197);
        CHECK(result.back()[1] == 198);
        CHECK(result.back()[2] == 199);
    }
}
