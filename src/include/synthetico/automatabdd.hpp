//
// Created by shuzhu on 08/04/24.
//

#ifndef SYNTH_AUTOMATABDD_HPP
#define SYNTH_AUTOMATABDD_HPP

#include <black/logic/logic.hpp>

#include <synthetico/synthetico.hpp>
#include <synthetico/varmgr.hpp>

namespace synth {

    namespace logic = black::logic;

    struct automatabdd {
        std::vector<proposition> inputs;
        std::vector<proposition> outputs;
        std::vector<proposition> variables;

        bformula init;
        bformula trans;
        bformula objective;

        std::shared_ptr<varmgr> var_mgr_;
        std::size_t automaton_id_;
        std::vector<int> initial_state_;
        std::vector<CUDD::BDD> transition_function_;
        CUDD::BDD final_states_;
    };

    std::ostream &operator<<(std::ostream &, automatabdd);

    automatabdd encodebdd(spec sp, std::shared_ptr<varmgr> var_mgr);

}

#endif //SYNTH_AUTOMATABDD_HPP
