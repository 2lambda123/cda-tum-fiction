//
// Created by Jan Drewniok on 28.01.23.
//

#ifndef FICTION_NM_POSITION_HPP
#define FICTION_NM_POSITION_HPP

#include "fiction/layouts/coordinates.hpp"
#include "fiction/traits.hpp"

#include <cassert>
#include <utility>

namespace fiction
{

/**
 * Computes the position of a cell in nanometers from the layout origin in an SiDB layout (unit: nm).
 *
 * @tparam Lyt SiDB cell-level layout type.
 * @param c The cell to compute the position for.
 * @return A pair representing the `(x,y)` position of `c` in nanometers from the layout origin.
 */
template <typename Lyt>
[[nodiscard]] constexpr std::pair<double, double> sidb_nm_position(const cell<Lyt>& c) noexcept
{
    static_assert(is_cell_level_layout_v<Lyt>, "Lyt is not a cell-level layout");
    static_assert(has_sidb_technology_v<Lyt>, "Lyt is not an SiDB layout");
    static_assert(is_sidb_lattice_v<Lyt>, "Lyt is not an SiDB lattice layout");
    static_assert(is_sidb_lattice_100_v<Lyt> || is_sidb_lattice_111_v<Lyt>, "Unsupported lattice orientation");

    if constexpr (!is_sidb_lattice_100_v<Lyt> && !is_sidb_lattice_111_v<Lyt>)
    {
        return {};
    }

    auto calculate_nm_position = [](const siqad::coord_t& c_siqad) -> std::pair<double, double>
    {
        const auto x =
            (c_siqad.x * lattice_orientation<Lyt>::LAT_A + c_siqad.z * lattice_orientation<Lyt>::LAT_C.first) * 0.1;
        const auto y =
            (c_siqad.y * lattice_orientation<Lyt>::LAT_B + c_siqad.z * lattice_orientation<Lyt>::LAT_C.second) * 0.1;
        return {x, y};
    };

    if constexpr (has_siqad_coord_v<Lyt>)
    {
        return calculate_nm_position(c);
    }
    else
    {
        const auto cell_in_siqad = siqad::to_siqad_coord(c);
        return calculate_nm_position(cell_in_siqad);
    }
}

}  // namespace fiction

#endif  // FICTION_NM_POSITION_HPP
