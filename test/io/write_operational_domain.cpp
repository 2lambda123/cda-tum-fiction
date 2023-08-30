//
// Created by marcel on 24.06.22.
//

#include <catch2/catch_test_macros.hpp>

#include <fiction/algorithms/simulation/sidb/operational_domain.hpp>
#include <fiction/io/write_operational_domain.hpp>

#include <sstream>
#include <string>

using namespace fiction;

TEST_CASE("Write empty operational domain", "[write-operational-domain]")
{
    operational_domain opdom{};

    std::ostringstream os{};

    SECTION("default sweep dimensions")
    {
        static constexpr const char* expected = "epsilon_r,lambda_tf,operational status\n";

        write_operational_domain(opdom, os);

        CHECK(os.str() == expected);
    }
    SECTION("custom sweep dimensions")
    {
        opdom.x_dimension = operational_domain::sweep_parameter::LAMBDA_TF;
        opdom.y_dimension = operational_domain::sweep_parameter::MU_MINUS;

        static constexpr const char* expected = "lambda_tf,mu_minus,operational status\n";

        write_operational_domain(opdom, os);

        CHECK(os.str() == expected);
    }
}

TEST_CASE("Write simple operational domain", "[write-operational-domain]")
{
    operational_domain opdom{};
    opdom.operational_values = {{{0, 0}, operational_domain::operational_status::OPERATIONAL},
                                {{0, 1}, operational_domain::operational_status::NON_OPERATIONAL}};

    std::ostringstream os{};

    SECTION("default operational tags")
    {
        std::set<std::string> expected{"epsilon_r,lambda_tf,operational status", "0,0,operational",
                                       "0,1,non-operational"};

        write_operational_domain(opdom, os);
        const auto os_str = os.str();

        std::istringstream is{os_str};

        for (std::string line{}; std::getline(is, line);)
        {
            CHECK(expected.find(line) != expected.end());
        }
    }

    SECTION("custom operational tags")
    {
        const write_operational_domain_params params = {"True", "False"};

        std::set<std::string> expected{"epsilon_r,lambda_tf,operational status", "0,0,True", "0,1,False"};

        write_operational_domain(opdom, os, params);
        const auto os_str = os.str();

        std::istringstream is{os_str};

        for (std::string line{}; std::getline(is, line);)
        {
            CHECK(expected.find(line) != expected.end());
        }
    }
}

TEST_CASE("Write operational domain with floating-point parameter values", "[write-operational-domain]")
{
    operational_domain opdom{};
    opdom.operational_values = {{{0.1, 0.2}, operational_domain::operational_status::OPERATIONAL},
                                {{0.3, 0.4}, operational_domain::operational_status::NON_OPERATIONAL},
                                {{1.2, 1.4}, operational_domain::operational_status::OPERATIONAL},
                                {{2.4, 5.75}, operational_domain::operational_status::NON_OPERATIONAL}};

    std::ostringstream os{};

    SECTION("default operational tags")
    {
        std::set<std::string> expected{"epsilon_r,lambda_tf,operational status", "0.1,0.2,operational",
                                       "0.3,0.4,non-operational",
                                       "1.2,1.4,"
                                       "operational",
                                       "2.4,5.75,non-operational"};

        write_operational_domain(opdom, os);
        const auto os_str = os.str();

        std::istringstream is{os_str};

        for (std::string line{}; std::getline(is, line);)
        {
            CHECK(expected.find(line) != expected.end());
        }
    }
    SECTION("custom operational tags")
    {
        const write_operational_domain_params params = {"1", "0"};

        std::set<std::string> expected{"epsilon_r,lambda_tf,operational status", "0.1,0.2,1", "0.3,0.4,0", "1.2,1.4,1",
                                       "2.4,5.75,0"};

        write_operational_domain(opdom, os, params);
        const auto os_str = os.str();

        std::istringstream is{os_str};

        for (std::string line{}; std::getline(is, line);)
        {
            CHECK(expected.find(line) != expected.end());
        }
    }
}
