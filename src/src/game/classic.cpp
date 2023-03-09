//
// Synthetico - Pure-past LTL synthesizer based on BLACK
//
// (C) 2023 Nicola Gigante
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <synthetico/synthetico.hpp>

#include <functional>
#include <unordered_set>
#include <unordered_map>

#include <iostream>


namespace synth {

  namespace { 

    struct encoder {

      qbformula fixpoint(size_t n);

      logic::alphabet &sigma;
      game_t type;
      automata aut;
    };

    qbformula encoder::fixpoint(size_t n) {
      using namespace logic::fragments::QBF;

      using op_t = logic::binary<Bool>::type;

      if(n == 0)
        return aut.objective;

      op_t op = type.match(
        [](game_t::eventually) { return op_t::disjunction{}; },
        [](game_t::always) { return op_t::conjunction{}; }
      );

      return 
        binary(op, 
          fixpoint(n - 1), 
          foreach(aut.outputs,
            thereis(aut.inputs,
              foreach(primed(aut.variables),
                implies(aut.trans, primed(fixpoint(n - 1)))
              )
            )
          )
        );
    }

  }


  black::tribool is_realizable_classic(spec sp) {
    using namespace logic::fragments::QBF;

    automata aut = encode(sp);

    qbformula qbf = encoder{*aut.init.sigma(), sp.type, aut}.fixpoint(2);


    std::cout << "f:         " << to_string(qbf) << "\n";
    std::cout << "flattened: " << to_string(flatten(qbf)) << "\n";
    std::cout << "prenex:    " << to_string(prenex(flatten(qbf))) << "\n";

    return black::tribool::undef;
  }

}
