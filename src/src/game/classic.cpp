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

      [[maybe_unused]]
      qbformula fixpoint(std::optional<qbformula> previous = {});

      [[maybe_unused]]
      qbformula test(qbformula fp, qbformula prevfp);

      [[maybe_unused]]
      qbformula win(qbformula fp);

      logic::alphabet &sigma;
      game_t type;
      automaton aut;
    };

    static constexpr bool debug = true;

    static qbformula print(std::string name, qbformula f) {
      if(debug)
        std::cerr << name << ": " << to_string(f) << "\n";
      return f;
    }

    qbformula encoder::fixpoint(std::optional<qbformula> previous) {
      using namespace logic::fragments::QBF;
      using op_t = logic::binary<Bool>::type;

      if(!previous)
        return stepped(aut.objective);

      op_t op = type.match(
        [](game_t::eventually) { return op_t::disjunction{}; },
        [](game_t::always) { return op_t::conjunction{}; }
      );

      return print("fixpoint",
        binary(op, 
          *previous, 
          thereis(stepped(aut.outputs),
            foreach(stepped(aut.inputs),
              foreach(stepped(primed(aut.variables)),
                implies(stepped(aut.trans), stepped(*previous))
              )
            )
          )
        )
      );
    }

    qbformula encoder::test(qbformula fp, qbformula prevfp) {
      using namespace logic::fragments::QBF;

      return type.match(
        [&](game_t::eventually) -> qbformula { 
          return 
            print("test", foreach(stepped(aut.variables), implies(fp, prevfp)));
        },
        [&](game_t::always) -> qbformula { 
          return 
            print("test", foreach(stepped(aut.variables), implies(prevfp, fp)));
        }
      );
    }

    qbformula encoder::win(qbformula fp) {
      using namespace logic::fragments::QBF;

      return 
        print("win", thereis(stepped(aut.variables), fp && stepped(aut.init)));
    }

  }

  static black::tribool solve(automaton aut, game_t type) { 
    using namespace logic::fragments::QBF;

    alphabet &sigma = *aut.init.sigma();

    if(debug)
      std::cerr << aut << "\n";

    encoder enc{sigma, type, aut};

    black::tribool result = black::tribool::undef;

    size_t k = 0;
    if(debug)
      std::cerr << "- k = 0\n";

    qbformula prevfp = enc.fixpoint();
    qbformula fp = enc.fixpoint(prevfp);
    while((result = is_sat(enc.test(fp, prevfp))) == false) {
      k++;
      if(debug)
        std::cerr << "- k = " << k << "\n";
      prevfp = fp;
      fp = enc.fixpoint(fp);
    }

    if(result == black::tribool::undef)
      return result;

    return is_sat(enc.win(fp));
  }


  black::tribool is_realizable_classic(purepast_spec sp) 
  {
    automaton aut = encode(sp);

    return solve(aut, sp.type);
  }

  black::tribool is_realizable_classic(spec sp) 
  {
    // if(auto ppspec = to_purepast(sp); ppspec) 
    //   return is_realizable_classic(*ppspec);
    
    automaton aut = encode(sp);

    return solve(aut, game_t::eventually{});
  }

}
