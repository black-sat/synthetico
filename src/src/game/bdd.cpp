//
// Created by shuzhu on 04/03/24.
//

#include <synthetico/synthetico.hpp>

#include "synthetico/game/VarMgr.hpp"
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
        automata aut;
    };


    black::tribool is_realizable_bdd(spec sp) {

//        logic::alphabet &sigma = *sp.formula.sigma();
        automata aut = encode(sp);
        std::shared_ptr<VarMgr> var_mgr = std::make_shared<VarMgr>();
        std::vector<std::string> ins, outs;
        for (auto prop : aut.inputs){
            ins.push_back(to_string(prop));
        }
        for (auto prop : aut.outputs){
            outs.push_back(to_string(prop));
        }
        var_mgr->create_named_variables(ins);
        var_mgr->create_named_variables(outs);

        return false;

    }

}
