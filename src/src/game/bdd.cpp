//
// Created by shuzhu on 04/03/24.
//

#include <synthetico/synthetico.hpp>

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

    static constexpr bool debug = true;


    black::tribool is_realizable_bdd(spec sp) {

//        logic::alphabet &sigma = *sp.formula.sigma();
        std::shared_ptr<varmgr> var_mgr = std::make_shared<varmgr>();
        automatabdd aut = encodebdd(sp, var_mgr);
        if(debug)
            std::cerr << aut << "\n";

        return false;

    }

}
