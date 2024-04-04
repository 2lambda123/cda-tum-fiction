//
// Created by Jan Drewniok on 21.03.24.
//

#include <catch2/catch_template_test_macros.hpp>

#include <fiction/algorithms/simulation/sidb/assess_displacement_robustness.hpp>
#include <fiction/types.hpp>

#include <cmath>

using namespace fiction;

TEST_CASE("assess_displacement_robustness for Y-shape SiDB AND gate", "[assess-displacement-robustness]")
{
    sidb_cell_clk_lyt_siqad lyt{};
    lyt.assign_cell_type({0, 0, 1}, sidb_technology::cell_type::INPUT);
    lyt.assign_cell_type({2, 1, 1}, sidb_technology::cell_type::INPUT);

    lyt.assign_cell_type({20, 0, 1}, sidb_technology::cell_type::INPUT);
    lyt.assign_cell_type({18, 1, 1}, sidb_technology::cell_type::INPUT);

    lyt.assign_cell_type({4, 2, 1}, sidb_technology::cell_type::NORMAL);
    lyt.assign_cell_type({6, 3, 1}, sidb_technology::cell_type::NORMAL);

    lyt.assign_cell_type({14, 3, 1}, sidb_technology::cell_type::NORMAL);
    lyt.assign_cell_type({16, 2, 1}, sidb_technology::cell_type::NORMAL);

    lyt.assign_cell_type({10, 6, 0}, sidb_technology::cell_type::OUTPUT);
    lyt.assign_cell_type({10, 7, 0}, sidb_technology::cell_type::OUTPUT);

    lyt.assign_cell_type({10, 9, 1}, sidb_technology::cell_type::NORMAL);

    SECTION("only one displacement variation")
    {
        displacement_robustness_params<tt> params{};
        params.displacement_variations                        = {1, 0};
        params.tt                                             = std::vector<tt>{create_and_tt()};
        params.operational_params.simulation_parameter        = sidb_simulation_parameters{2, -0.28};
        params.operational_params.bdl_params.maximum_distance = 2.0;
        params.operational_params.bdl_params.minimum_distance = 0.2;
        displacement_robustness_stats stats{};

        const auto result = assess_displacement_robustness(lyt, params, &stats);
        CHECK((stats.num_non_operational_sidb_displacements + stats.num_operational_sidb_displacements) ==
              static_cast<std::size_t>(std::pow(3, lyt.num_cells())));
        CHECK(static_cast<double>(result.operational_values.size()) == std::pow(3, lyt.num_cells()));
        CHECK(num_non_operational_layouts(result) == stats.num_non_operational_sidb_displacements);
        CHECK(num_operational_layouts(result) == stats.num_operational_sidb_displacements);
    }
}
