//
// Created by shuzhu on 04/03/24.
//

#include "synthetico/synthetico.hpp"

#include "synthetico/varmgr.hpp"
#include <black/logic/prettyprint.hpp>
#include <black/support/range.hpp>

#include <string>
#include <iostream>

namespace synth {

    enum class player_t {
        controller,
        environment
    };

    struct encoder {
        logic::alphabet &sigma;
        automatabdd aut;
    };

    struct SynthesisResult{
        bool realizability;
        CUDD::BDD winning_states;
        CUDD::BDD winning_moves;
        std::unique_ptr<Transducer> transducer;
    };

    struct DfaGameSynthesizer {
        std::shared_ptr<varmgr> var_mgr_;
        Player starting_player_;
        std::vector<int> initial_vector_;
        std::vector<CUDD::BDD> transition_vector_;
        std::unique_ptr<Quantification> quantify_independent_variables_;
        std::unique_ptr<Quantification> quantify_non_state_variables_;

        CUDD::BDD preimage(const CUDD::BDD& winning_states) const;

        CUDD::BDD project_into_states(const CUDD::BDD& winning_moves) const;

        std::unordered_map<int, CUDD::BDD> synthesize_strategy(
                const CUDD::BDD& winning_moves) const;

        bool includes_initial_state(const CUDD::BDD& winning_states) const;

        DfaGameSynthesizer(automatabdd aut, Player starting_player);


        SynthesisResult run_reachability(const CUDD::BDD& goal_states);
        SynthesisResult run_safety(const CUDD::BDD& goal_states);
    };

    DfaGameSynthesizer::DfaGameSynthesizer(automatabdd aut, Player starting_player){
        starting_player_ = starting_player;
        var_mgr_ = aut.var_mgr_;

        // Make versions of the initial state and transition function that can be used
        // with CUDD::BDD::Eval and CUDD::BDD::VectorCompose, respectively
        initial_vector_ = var_mgr_->make_eval_vector(aut.automaton_id_,
                                                     aut.initial_state_);
        transition_vector_ = var_mgr_->make_compose_vector(
                aut.automaton_id_, aut.transition_function_);

        CUDD::BDD input_cube = var_mgr_->input_cube();
        CUDD::BDD output_cube = var_mgr_->output_cube();

        // quantify_independent_variables_ quantifies all variables that the outputs
        // don't depend on (input variables if the agent plays first, or no variables
        // if the environment plays first). quantify_non_state_variables_ quantifies
        // all remaining variables that are not state variables.
        if (starting_player_ == Player::Environment) {
                quantify_independent_variables_ = std::make_unique<NoQuantification>();
                quantify_non_state_variables_ = std::make_unique<ForallExists>(input_cube,
                                                                               output_cube);
        } else {
                quantify_independent_variables_ = std::make_unique<Forall>(input_cube);
                quantify_non_state_variables_ = std::make_unique<Exists>(output_cube);
            }
        }

    CUDD::BDD DfaGameSynthesizer::preimage(
            const CUDD::BDD& winning_states) const {
        // Transitions that move into a winning state
        CUDD::BDD winning_transitions =
                winning_states.VectorCompose(transition_vector_);

        // Quantify all variables that the outputs don't depend on
        return quantify_independent_variables_->apply(winning_transitions);
    }

    CUDD::BDD DfaGameSynthesizer::project_into_states(
            const CUDD::BDD& winning_moves) const {
        return quantify_non_state_variables_->apply(winning_moves);
    }

    bool DfaGameSynthesizer::includes_initial_state(
            const CUDD::BDD& winning_states) const {
        // Need to create a copy if we want to define the function as const, since
        // CUDD::BDD::Eval does not take the data as const
        std::vector<int> copy(initial_vector_);

        return winning_states.Eval(copy.data()).IsOne();
    }

    /*
    std::unordered_map<int, CUDD::BDD> DfaGameSynthesizer::synthesize_strategy(
            const CUDD::BDD& winning_moves) const {
        std::vector<CUDD::BDD> parameterized_output_function;
        int* output_indices;
        CUDD::BDD output_cube = var_mgr_->output_cube();
        std::size_t output_count = var_mgr_->output_variable_count();

        // Need to negate the BDD because b.SolveEqn(...) solves the equation b = 0
        CUDD::BDD pre = (!winning_moves).SolveEqn(output_cube,
                                                  parameterized_output_function,
                                                  &output_indices,
                                                  output_count);

        // Copy the index since it will be necessary in the last step
        std::vector<int> index_copy(output_count);

        for (std::size_t i = 0; i < output_count; ++i) {
            index_copy[i] = output_indices[i];
        }

        // Verify that the solution is correct, also frees output_index
        CUDD::BDD verified = (!winning_moves).VerifySol(parameterized_output_function,
                                                        output_indices);

        black_assert(pre == verified);

        std::unordered_map<int, CUDD::BDD> output_function;

        for (int i = output_count - 1; i >= 0; --i) {
            int output_index = index_copy[i];

            output_function[output_index] = parameterized_output_function[i];

            for (int j = output_count - 1; j >= i; --j) {
                int parameter_index = index_copy[j];

                // Can be anything, set to the constant 1 for simplicity
                CUDD::BDD parameter_value = var_mgr_->cudd_mgr()->bddOne();

                output_function[output_index] =
                        output_function[output_index].Compose(parameter_value,
                                                              parameter_index);
            }
        }

        return output_function;
    }
     */

    SynthesisResult DfaGameSynthesizer::run_reachability(const CUDD::BDD& goal_states_) {
        SynthesisResult result;
        CUDD::BDD winning_states = goal_states_;
        CUDD::BDD winning_moves = winning_states;

        while (true) {
            CUDD::BDD new_winning_states, new_winning_moves;

            if (starting_player_ == Player::Agent){
                CUDD::BDD quantified_X_transitions_to_winning_states = preimage(winning_states);
                new_winning_moves = winning_moves |
                                    ((!winning_states) & quantified_X_transitions_to_winning_states);

                new_winning_states = project_into_states(new_winning_moves);
            } else {
                CUDD::BDD transitions_to_winning_states = preimage(winning_states);
                CUDD::BDD new_collected_winning_states = project_into_states(transitions_to_winning_states);
                new_winning_states = winning_states | new_collected_winning_states;
                new_winning_moves = winning_moves |
                                    ((!winning_states) & new_collected_winning_states & transitions_to_winning_states);
            }

            if (includes_initial_state(new_winning_states)) {
                result.realizability = true;
                result.winning_states = new_winning_states;
                result.winning_moves = new_winning_moves;
                result.transducer = nullptr;
                return result;

            } else if (new_winning_states == winning_states) {
                result.realizability = false;
                result.winning_states = new_winning_states;
                result.winning_moves = new_winning_moves;
                result.transducer = nullptr;
                return result;
            }

            winning_moves = new_winning_moves;
            winning_states = new_winning_states;
        }

    }

    SynthesisResult DfaGameSynthesizer::run_safety(const CUDD::BDD &goal_states) {
        SynthesisResult result;
        CUDD::BDD candidate_winning_states = goal_states;
        CUDD::BDD candidate_winning_moves = candidate_winning_states;
//        std::cout<<candidate_winning_moves<<std::endl;

        while (true) {
            CUDD::BDD new_candidate_winning_moves = candidate_winning_moves & preimage(candidate_winning_states);

            CUDD::BDD new_candidate_winning_states = project_into_states(new_candidate_winning_moves);
//            std::cout<<new_candidate_winning_moves<<std::endl;
//            std::cout<<new_candidate_winning_states<<std::endl;

            if (!includes_initial_state(new_candidate_winning_states)) {
                result.realizability = false;
                result.winning_states = var_mgr_->cudd_mgr()->bddZero(); // the set of candidate_winning_states is not exactly winning states
                result.winning_moves = var_mgr_->cudd_mgr()->bddZero();
                result.transducer = nullptr;
                return result;

            } else if (new_candidate_winning_states == candidate_winning_states) {
                result.realizability = true;
                result.winning_states = new_candidate_winning_states;
                result.winning_moves = new_candidate_winning_moves;
                result.transducer = nullptr;
                return result;
            }

            candidate_winning_moves = new_candidate_winning_moves;
            candidate_winning_states = new_candidate_winning_states;
        }
    }

    static constexpr bool debug = false;

    black::tribool is_realizable_bdd(spec sp) {

//        logic::alphabet &sigma = *sp.formula.sigma();
        std::shared_ptr<varmgr> var_mgr = std::make_shared<varmgr>();
        automatabdd aut = encodebdd(sp, var_mgr);

        if(debug)
            std::cerr << aut << "\n";
        DfaGameSynthesizer dfagame(aut, Player::Agent);
        SynthesisResult res;
        sp.type.match(
                [&](game_t::eventually) {
                    res = dfagame.run_reachability(aut.final_states_);
                },
                [&](game_t::always) {
                    res = dfagame.run_safety(aut.final_states_);
                }
        );

        return res.realizability;
    }

}
