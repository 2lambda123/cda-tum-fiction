//
// Created by Willem Lambooy on 06/02/2024.
//

#ifndef FICTION_GROUND_STATE_SPACE_HPP
#define FICTION_GROUND_STATE_SPACE_HPP

#include "fiction/algorithms/simulation/sidb/sidb_simulation_parameters.hpp"
#include "fiction/technology/charge_distribution_surface.hpp"
#include "fiction/technology/physical_constants.hpp"
#include "fiction/technology/sidb_charge_state.hpp"
#include "fiction/technology/sidb_cluster_hierarchy.hpp"

#include <btree.h>
#include <mockturtle/utils/stopwatch.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace fiction
{

struct ground_state_space_result
{
    const sidb_cluster_ptr              top_cluster;
    const std::chrono::duration<double> runtime;
    const uint64_t                      pruned_top_level_multisets;
    const uint64_t                      maximum_top_level_multisets;
};

namespace detail
{

template <typename Lyt>
class ground_state_space
{
  public:
    explicit ground_state_space(const Lyt& lyt, const uint64_t max_cluster_size_for_witness_partitioning,
                                const sidb_simulation_parameters& phys_params) noexcept :
            top_cluster{to_sidb_cluster(sidb_cluster_hierarchy(lyt))},
            clustering{get_initial_clustering(top_cluster, get_local_potential_bounds(lyt, phys_params))},
            witness_partitioning_max_cluster_size{max_cluster_size_for_witness_partitioning},
            mu_bounds_with_error{physical_constants::POP_STABILITY_ERR - phys_params.mu_minus,
                                 -physical_constants::POP_STABILITY_ERR - phys_params.mu_minus,
                                 physical_constants::POP_STABILITY_ERR - phys_params.mu_plus(),
                                 -physical_constants::POP_STABILITY_ERR - phys_params.mu_plus()}
    {}

    ground_state_space_result run() noexcept
    {
        mockturtle::stopwatch<>::duration time_counter{};
        {
            const mockturtle::stopwatch stop{time_counter};

            while (!terminate)
            {
                while (!update_charge_spaces())
                    ;

                move_up_hierarchy();
            }
        }

        const uint64_t max_multisets = maximum_top_level_multisets(top_cluster->size());

        return ground_state_space_result{top_cluster, time_counter, max_multisets - top_cluster->charge_space.size(),
                                         max_multisets};
    }

  private:
    constexpr inline bool fail_onto_negative_charge(const double pot_bound) const noexcept
    {
        // V > e - mu-
        return pot_bound > mu_bounds_with_error[0];
    }

    constexpr inline bool fail_onto_positive_charge(const double pot_bound) const noexcept
    {
        // V < -e - mu+
        return pot_bound < mu_bounds_with_error[3];
    }

    constexpr inline bool ub_fail_onto_neutral_charge(const double pot_bound) const noexcept
    {
        // V < -e - mu-
        return pot_bound < mu_bounds_with_error[1];
    }

    constexpr inline bool lb_fail_onto_neutral_charge(const double pot_bound) const noexcept
    {
        // V > e - mu+
        return pot_bound > mu_bounds_with_error[2];
    }

    static std::pair<charge_distribution_surface<Lyt>, charge_distribution_surface<Lyt>>
    get_local_potential_bounds(const Lyt& lyt, const sidb_simulation_parameters& phys_params) noexcept
    {
        charge_distribution_surface<Lyt> cl_min{lyt};
        charge_distribution_surface<Lyt> cl_max{lyt};

        cl_min.assign_physical_parameters(phys_params);
        cl_max.assign_physical_parameters(phys_params);

        cl_min.assign_all_charge_states(sidb_charge_state::POSITIVE);
        cl_max.assign_all_charge_states(sidb_charge_state::NEGATIVE);

        cl_min.update_after_charge_change();
        cl_max.update_after_charge_change();

        return {cl_min, cl_max};
    }

    static sidb_clustering
    get_initial_clustering(const sidb_cluster_ptr& c,
                           const std::pair<charge_distribution_surface<Lyt>, charge_distribution_surface<Lyt>>&
                               local_potential_bound_containers) noexcept
    {
        const auto& [min_loc_pot_cds, max_loc_pot_cds] = local_potential_bound_containers;

        sidb_clustering clustering{};

        if (c->size() == 1)
        {
            const uint64_t i = get_singleton_sidb_ix(c);

            c->initialize_singleton_cluster_charge_space(i, -min_loc_pot_cds.get_local_potential_by_index(i).value(),
                                                         -max_loc_pot_cds.get_local_potential_by_index(i).value());

            for (uint64_t j = 0; j < min_loc_pot_cds.num_cells(); ++j)
            {
                c->pot_projs[j] = potential_projection_order{min_loc_pot_cds.get_potential_by_indices(i, j)};
            }

            clustering.emplace(c);
        }

        for (const sidb_cluster_ptr& child : c->children)
        {
            const sidb_clustering& child_clustering = get_initial_clustering(child, local_potential_bound_containers);
            clustering.insert(child_clustering.cbegin(), child_clustering.cend());
        }

        return clustering;
    }

    template <bound_direction bound>
    static constexpr inline potential_projection get_proj_bound(const sidb_cluster_ptr& c,
                                                                const uint64_t          sidb_ix) noexcept
    {
        return c->pot_projs.at(sidb_ix).get<bound>();
    }

    template <bound_direction bound>
    static constexpr inline double get_next_proj_pot_bound(const sidb_cluster_ptr& c, const uint64_t sidb_ix) noexcept
    {
        return c->pot_projs.at(sidb_ix).get_next<bound>().V;
    }

    template <bound_direction bound>
    static constexpr inline potential_projection get_proj_state_bound(const sidb_cluster_projector_state& pst,
                                                                      const uint64_t sidb_ix) noexcept
    {
        return pst.cluster->pot_projs.at(sidb_ix).get_pot_proj_for_m_conf<bound>(pst.multiset_conf);
    }

    static constexpr inline void add_pot_proj(const sidb_cluster_ptr& c, const uint64_t sidb_ix,
                                              const potential_projection& pp) noexcept
    {
        c->pot_projs[sidb_ix].add(pp);
    }

    static constexpr inline void remove_all_cluster_charge_state_occurrences(const sidb_cluster_projector_state& rm_pst,
                                                                             const uint64_t sidb_ix) noexcept
    {
        rm_pst.cluster->pot_projs[sidb_ix].remove_m_conf(rm_pst.multiset_conf);
    }

    template <bound_direction bound>
    static constexpr void update_external_pot_proj_if_bound_removed(const sidb_cluster_projector_state& pst,
                                                                    const sidb_cluster_receptor_state&  rst) noexcept
    {
        const potential_projection& cur_bound = get_proj_bound<bound>(pst.cluster, rst.sidb_ix);

        if (cur_bound.M == pst.multiset_conf)
        {
            rst.cluster->update_recv_ext_pot_bound<bound>(
                rst.sidb_ix, get_next_proj_pot_bound<bound>(pst.cluster, rst.sidb_ix) - cur_bound.V);
        }
    }

    constexpr void update_external_potential_projection(const sidb_cluster_projector_state& pst,
                                                        const sidb_cluster_receptor_state&  rst) const noexcept
    {
        update_external_pot_proj_if_bound_removed<bound_direction::LOWER>(pst, rst);
        update_external_pot_proj_if_bound_removed<bound_direction::UPPER>(pst, rst);

        remove_all_cluster_charge_state_occurrences(pst, rst.sidb_ix);
    }

    void handle_invalid_state(const sidb_cluster_projector_state& pst) noexcept
    {
        // may need to remove the multiset charge configuration from the internal potential projection order too

        for (const sidb_cluster_ptr& other_c : clustering)
        {
            if (other_c == pst.cluster)
            {
                continue;
            }

            for (const uint64_t sidb_ix : other_c->sidbs)
            {
                update_external_potential_projection(pst, sidb_cluster_receptor_state{other_c, sidb_ix});
            }
        }
    }

    struct witness_partitioning_state
    {
        using witness_set = phmap::btree_set<uint64_t>;

        witness_set negative_witnesses{}, positive_witnesses{}, neutral_witnesses{};
        uint64_t    required_neg_count, required_pos_count, required_neut_count;

        explicit witness_partitioning_state(const sidb_cluster_projector_state& pst) noexcept :
                required_neg_count{pst.get_count<sidb_charge_state::NEGATIVE>()},
                required_pos_count{pst.get_count<sidb_charge_state::POSITIVE>()},
                required_neut_count{pst.get_count<sidb_charge_state::NEUTRAL>()}
        {}

        void omit_free_witnesses() noexcept
        {
            // find free witnesses to reduce the problem, leaving only witnesses to partition that overlap
            for (const auto& [this_set, competing_sets] :
                 std::array{std::make_pair(std::make_pair(std::ref(negative_witnesses), std::ref(required_neg_count)),
                                           std::make_pair(std::ref(positive_witnesses), std::ref(neutral_witnesses))),
                            std::make_pair(std::make_pair(std::ref(positive_witnesses), std::ref(required_pos_count)),
                                           std::make_pair(std::ref(negative_witnesses), std::ref(neutral_witnesses))),
                            std::make_pair(std::make_pair(std::ref(neutral_witnesses), std::ref(required_neut_count)),
                                           std::make_pair(std::ref(negative_witnesses), std::ref(positive_witnesses)))})
            {
                for (witness_set::const_iterator it = this_set.first.cbegin(); it != this_set.first.cend();)
                {
                    if (competing_sets.first.find(*it) == competing_sets.first.cend() &&
                        competing_sets.second.find(*it) == competing_sets.second.cend())
                    {
                        it = this_set.first.erase(it);
                        this_set.second--;
                        continue;
                    }

                    ++it;
                }
            }
        }
    };

    template <sidb_charge_state current_fill_cs>
    static bool find_valid_witness_partitioning(witness_partitioning_state& st,
                                                const uint64_t              num_witnesses_for_current_cs) noexcept
    {
        if constexpr (current_fill_cs == sidb_charge_state::NEUTRAL)
        {
            return st.neutral_witnesses.size() >= num_witnesses_for_current_cs;
        }
        else if constexpr (current_fill_cs == sidb_charge_state::POSITIVE)
        {
            if (num_witnesses_for_current_cs == 0)
            {
                return find_valid_witness_partitioning<sidb_charge_state::NEUTRAL>(st, st.required_neut_count);
            }

            for (typename witness_partitioning_state::witness_set::const_iterator it = st.positive_witnesses.cbegin();
                 it != st.positive_witnesses.cend();)
            {
                const uint64_t take_witness = *st.positive_witnesses.cbegin();

                st.positive_witnesses.erase(it);

                st.neutral_witnesses.erase(take_witness);

                if (find_valid_witness_partitioning<sidb_charge_state::POSITIVE>(st, num_witnesses_for_current_cs - 1))
                {
                    return true;
                }

                it = ++st.positive_witnesses.emplace(take_witness).first;

                st.neutral_witnesses.emplace(take_witness);
            }

            return false;
        }
        else if constexpr (current_fill_cs == sidb_charge_state::NEGATIVE)
        {
            if (num_witnesses_for_current_cs == 0)
            {
                return find_valid_witness_partitioning<sidb_charge_state::POSITIVE>(st, st.required_pos_count);
            }

            for (typename witness_partitioning_state::witness_set::const_iterator it = st.negative_witnesses.cbegin();
                 it != st.negative_witnesses.cend();)
            {
                const uint64_t take_witness = *it;

                st.negative_witnesses.erase(it);

                st.positive_witnesses.erase(take_witness);
                st.neutral_witnesses.erase(take_witness);

                if (find_valid_witness_partitioning<sidb_charge_state::NEGATIVE>(st, num_witnesses_for_current_cs - 1))
                {
                    return true;
                }

                it = ++st.negative_witnesses.emplace(take_witness).first;

                st.positive_witnesses.emplace(take_witness);
                st.neutral_witnesses.emplace(take_witness);
            }

            return false;
        }
    }

    enum class potential_bound_analysis_mode
    {
        ANALYZE_MULTISET,
        ANALYZE_COMPOSITION
    };

    template <potential_bound_analysis_mode mode>
    static inline std::pair<double, double>
    get_received_potential_bounds(const sidb_cluster_projector_state& pst, const uint64_t sidb_ix,
                                  const std::optional<intra_cluster_potential_bounds>& internal_pot_bounds) noexcept
    {
        if constexpr (mode == potential_bound_analysis_mode::ANALYZE_MULTISET)
        {
            return {get_proj_state_bound<bound_direction::LOWER>(pst, sidb_ix).V +
                        pst.cluster->get_recv_ext_pot_bound<bound_direction::LOWER>(sidb_ix),
                    get_proj_state_bound<bound_direction::UPPER>(pst, sidb_ix).V +
                        pst.cluster->get_recv_ext_pot_bound<bound_direction::UPPER>(sidb_ix)};
        }
        else if constexpr (mode == potential_bound_analysis_mode::ANALYZE_COMPOSITION)
        {
            return {internal_pot_bounds.value().at(sidb_ix)[static_cast<uint8_t>(bound_direction::LOWER)] +
                        pst.cluster->get_recv_ext_pot_bound<bound_direction::LOWER>(sidb_ix),
                    internal_pot_bounds.value().at(sidb_ix)[static_cast<uint8_t>(bound_direction::UPPER)] +
                        pst.cluster->get_recv_ext_pot_bound<bound_direction::UPPER>(sidb_ix)};
        }
    }

    template <potential_bound_analysis_mode mode>
    bool perform_potential_bound_analysis(
        const sidb_cluster_projector_state&                  pst,
        const std::optional<intra_cluster_potential_bounds>& internal_potential_bounds = std::nullopt) const noexcept
    {
        witness_partitioning_state st{pst};

        for (const uint64_t sidb_ix : pst.cluster->sidbs)
        {
            const auto& [recv_pot_lb, recv_pot_ub] =
                get_received_potential_bounds<mode>(pst, sidb_ix, internal_potential_bounds);

            if (st.required_neg_count != 0 && !fail_onto_negative_charge(recv_pot_lb))
            {
                st.negative_witnesses.emplace(sidb_ix);
            }

            if (st.required_pos_count != 0 && !fail_onto_positive_charge(recv_pot_ub))
            {
                st.positive_witnesses.emplace(sidb_ix);
            }

            if (st.required_neut_count != 0 && !ub_fail_onto_neutral_charge(recv_pot_ub) &&
                !lb_fail_onto_neutral_charge(recv_pot_lb))
            {
                st.neutral_witnesses.emplace(sidb_ix);
            }
        }

        if (st.negative_witnesses.size() < st.required_neg_count ||
            st.positive_witnesses.size() < st.required_pos_count ||
            st.neutral_witnesses.size() < st.required_neut_count)
        {
            return false;
        }

        if (pst.cluster->size() > witness_partitioning_max_cluster_size)
        {
            return true;
        }

        st.omit_free_witnesses();

        return find_valid_witness_partitioning<sidb_charge_state::NEGATIVE>(st, st.required_neg_count);
    }

    bool check_charge_space(const sidb_cluster_ptr& c) noexcept
    {
        // skip if |charge space| = 1?
        if (c->charge_space.size() == 1)
        {
            return true;
        }

        bool fixpoint = true;

        std::vector<uint64_t> removed_ms{};
        removed_ms.reserve(c->charge_space.size());

        for (const sidb_cluster_charge_state& m : c->charge_space)
        {
            const sidb_cluster_projector_state pst{c, static_cast<uint64_t>(m)};

            if (!perform_potential_bound_analysis<potential_bound_analysis_mode::ANALYZE_MULTISET>(pst))
            {
                handle_invalid_state(pst);
                removed_ms.emplace_back(pst.multiset_conf);
                fixpoint = false;
            }
        }

        for (const uint64_t m : removed_ms)
        {
            c->charge_space.erase(sidb_cluster_charge_state{m});
        }

        return fixpoint;
    }

    bool update_charge_spaces(const std::optional<uint64_t>& skip_cluster = std::nullopt) noexcept
    {
        bool fixpoint = true;

        // make a pass over the clustering and see if the charge spaces contain invalid cluster charge states
        for (const sidb_cluster_ptr& c : clustering)
        {
            if (!skip_cluster.has_value() || c->uid != skip_cluster.value())
            {
                fixpoint &= check_charge_space(c);
            }
        }

        return fixpoint;
    }

    template <bound_direction bound>
    void subtract_sibling_pot_from_recv_ext_pot_bound(const sidb_cluster_ptr&            parent,
                                                      const sidb_cluster_receptor_state& child_rst) const noexcept
    {
        double recv_pot_without_siblings = child_rst.cluster->get_recv_ext_pot_bound<bound>(child_rst.sidb_ix);

        for (const sidb_cluster_ptr& sibling : parent->children)
        {
            if (sibling != child_rst.cluster)
            {
                recv_pot_without_siblings -= get_proj_bound<bound>(sibling, child_rst.sidb_ix).V;
            }
        }

        child_rst.cluster->set_recv_ext_pot_bound<bound>(child_rst.sidb_ix, recv_pot_without_siblings);
        parent->set_recv_ext_pot_bound<bound>(child_rst.sidb_ix, recv_pot_without_siblings);
    }

    void derive_children_recv_bounds_without_siblings(const sidb_cluster_ptr& parent) const noexcept
    {
        for (const sidb_cluster_ptr& child : parent->children)
        {
            for (const uint64_t sidb_ix : child->sidbs)
            {
                const sidb_cluster_receptor_state child_rst{child, sidb_ix};
                subtract_sibling_pot_from_recv_ext_pot_bound<bound_direction::LOWER>(parent, child_rst);
                subtract_sibling_pot_from_recv_ext_pot_bound<bound_direction::UPPER>(parent, child_rst);
            }
        }
    }

    bool verify_composition(sidb_cluster_state_composition& composition) const noexcept
    {
        for (sidb_cluster_state& receiving_st : composition)
        {
            for (const uint64_t sidb_ix : receiving_st.proj_st.cluster->sidbs)
            {
                double internal_pot_lb{};
                double internal_pot_ub{};

                for (const sidb_cluster_state& cst : composition)
                {
                    internal_pot_lb += get_proj_state_bound<bound_direction::LOWER>(cst.proj_st, sidb_ix).V;
                    internal_pot_ub += get_proj_state_bound<bound_direction::UPPER>(cst.proj_st, sidb_ix).V;
                }

                receiving_st.set_pot_bounds(sidb_ix, internal_pot_lb, internal_pot_ub);
            }

            if (!perform_potential_bound_analysis<potential_bound_analysis_mode::ANALYZE_COMPOSITION>(
                    receiving_st.proj_st, receiving_st.internal_pot_bounds))
            {
                return false;
            }
        }

        return true;
    }

    void fill_merged_charge_state_space(const sidb_cluster_ptr& parent, const uint64_t cur_child_ix,
                                        sidb_cluster_charge_state& m) const noexcept
    {
        if (cur_child_ix >= parent->children.size())
        {
            if (!verify_composition(m.compositions[0]))
            {
                return;
            }

            // check if composition exists
            sidb_cluster_charge_state_space::iterator it = parent->charge_space.find(m);
            if (it != parent->charge_space.cend())
            {
                it->compositions.emplace_back(m.compositions[0]);
            }
            else
            {
                parent->charge_space.emplace(m);
            }

            return;
        }

        const sidb_cluster_ptr& cur_child = *std::next(parent->children.cbegin(), static_cast<int64_t>(cur_child_ix));

        for (const sidb_cluster_charge_state& m_part : cur_child->charge_space)
        {
            m.compositions[0].emplace_back(sidb_cluster_state{cur_child, static_cast<uint64_t>(m_part)});
            m += m_part;

            fill_merged_charge_state_space(parent, cur_child_ix + 1, m);

            m.compositions[0].pop_back();
            m -= m_part;
        }
    }

    void construct_merged_charge_state_space(const sidb_cluster_ptr& parent) const noexcept
    {
        sidb_cluster_charge_state m{};
        m.compositions.emplace_back(sidb_cluster_state_composition{});
        fill_merged_charge_state_space(parent, 0, m);
    }

    template <bound_direction bound>
    void merge_pot_proj_bounds(const sidb_cluster_ptr& parent, const sidb_cluster_receptor_state& rst) const noexcept
    {
        // construct external projected potential bounds for every composition of every element in the charge space
        for (const sidb_cluster_charge_state& m : parent->charge_space)
        {
            for (const sidb_cluster_state_composition& composition : m.compositions)
            {
                potential_projection pot_proj_onto_other_c{};

                for (const auto& [child_pst, _] : composition)
                {
                    pot_proj_onto_other_c += get_proj_state_bound<bound>(child_pst, rst.sidb_ix);
                }

                add_pot_proj(parent, rst.sidb_ix, pot_proj_onto_other_c);
            }
        }

        // update the received external potential bound for the subject
        double diff = get_proj_bound<bound>(parent, rst.sidb_ix).V;

        for (const sidb_cluster_ptr& child : parent->children)
        {
            diff -= get_proj_bound<bound>(child, rst.sidb_ix).V;
        }

        rst.cluster->update_recv_ext_pot_bound<bound>(rst.sidb_ix, diff);
    }

    template <bound_direction bound>
    void merge_recv_pot_proj_bounds(const sidb_cluster_projector_state& pst,
                                    const sidb_cluster_ptr&             parent) const noexcept
    {
        for (const sidb_cluster_charge_state& m : parent->charge_space)
        {
            for (const sidb_cluster_state_composition& composition : m.compositions)
            {
                for (const auto& [child_pst, _] : composition)
                {
                    for (const uint64_t child_sidb_ix : child_pst.cluster->sidbs)
                    {
                        add_pot_proj(pst.cluster, child_sidb_ix, get_proj_state_bound<bound>(pst, child_sidb_ix));
                    }
                }
            }
        }
    }

    void construct_merged_potential_projections(const sidb_cluster_ptr& parent) const noexcept
    {
        for (const sidb_cluster_ptr& non_child : clustering)
        {
            for (const uint64_t sidb_ix : non_child->sidbs)
            {
                const sidb_cluster_receptor_state rst{non_child, sidb_ix};

                merge_pot_proj_bounds<bound_direction::LOWER>(parent, rst);
                merge_pot_proj_bounds<bound_direction::UPPER>(parent, rst);
            }

            for (const sidb_cluster_charge_state& m : non_child->charge_space)
            {
                const sidb_cluster_projector_state pst{non_child, static_cast<uint64_t>(m)};

                merge_recv_pot_proj_bounds<bound_direction::LOWER>(pst, parent);
                merge_recv_pot_proj_bounds<bound_direction::UPPER>(pst, parent);
            }
        }
    }

    static void compute_meets_for_internal_pot_bounds(const sidb_cluster_ptr& parent) noexcept
    {
        for (const sidb_cluster_charge_state& m : parent->charge_space)
        {
            for (const uint64_t sidb_ix : parent->sidbs)
            {
                double lb_meet = potential_bound_top<bound_direction::LOWER>();
                double ub_meet = potential_bound_top<bound_direction::UPPER>();

                for (const sidb_cluster_state_composition& composition : m.compositions)
                {
                    for (const auto& [pst, sibling_pot_bounds] : composition)
                    {
                        if (sibling_pot_bounds.count(sidb_ix) != 0)
                        {
                            take_meet_of_potential_bounds<bound_direction::LOWER>(
                                lb_meet, sibling_pot_bounds.at(sidb_ix)[static_cast<uint8_t>(bound_direction::LOWER)]);
                            take_meet_of_potential_bounds<bound_direction::UPPER>(
                                ub_meet, sibling_pot_bounds.at(sidb_ix)[static_cast<uint8_t>(bound_direction::UPPER)]);
                        }
                    }
                }

                add_pot_proj(parent, sidb_ix, potential_projection{lb_meet, static_cast<uint64_t>(m)});
                add_pot_proj(parent, sidb_ix, potential_projection{ub_meet, static_cast<uint64_t>(m)});
            }
        }
    }

    void move_up_hierarchy() noexcept
    {
        if (clustering.size() == 1)
        {
            terminate = true;
            return;
        }

        // find the parent with the minimum cluster size
        const sidb_cluster_ptr& min_parent =
            (*std::min_element(clustering.cbegin(), clustering.cend(),
                               [](const sidb_cluster_ptr& c1, const sidb_cluster_ptr& c2)
                               { return c1->get_parent()->size() < c2->get_parent()->size(); }))
                ->get_parent();

        for (const sidb_cluster_ptr& c : min_parent->children)
        {
            clustering.erase(c);
        }

        derive_children_recv_bounds_without_siblings(min_parent);

        construct_merged_charge_state_space(min_parent);

        construct_merged_potential_projections(min_parent);

        compute_meets_for_internal_pot_bounds(min_parent);

        clustering.emplace(min_parent);

        update_charge_spaces(min_parent->uid);
    }

    static constexpr inline uint64_t maximum_top_level_multisets(const uint64_t number_of_sidbs) noexcept
    {
        // computes nCr(N + 2, 2)
        return ((number_of_sidbs + 1) * (number_of_sidbs + 2)) / 2;
    }

    const sidb_cluster_ptr top_cluster;

    sidb_clustering clustering{};

    bool terminate = false;

    const uint64_t witness_partitioning_max_cluster_size;

    const std::array<double, 4> mu_bounds_with_error;
};

}  // namespace detail

template <typename Lyt>
ground_state_space_result
ground_state_space(const Lyt& lyt, const uint64_t max_cluster_size_for_witness_partitioning = 6,
                   const sidb_simulation_parameters& phys_params = sidb_simulation_parameters{}) noexcept
{
    return detail::ground_state_space(lyt, max_cluster_size_for_witness_partitioning, phys_params).run();
}

}  // namespace fiction

#endif  // FICTION_GROUND_STATE_SPACE_HPP
