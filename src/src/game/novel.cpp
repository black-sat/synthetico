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

#include <black/logic/prettyprint.hpp>
#include <black/support/range.hpp>

#include <string>
#include <iostream>

namespace synth {

  enum class player_t {
    controller,
    environment
  };

  namespace {
    struct encoder {

      bformula win(player_t player, game_t type, size_t n);
      bformula unravel(size_t n);

      qbformula encode(player_t player, game_t type, size_t n);

      logic::alphabet &sigma;
      automata aut;

    };

    bformula encoder::win(player_t player, game_t type, size_t n) {
      using namespace logic;

      bformula objective = 
        player == player_t::controller ? aut.objective : !aut.objective;

      bool reach = type.match(
        [&](game_t::eventually) {
          return player == player_t::controller;
        },
        [&](game_t::always) {
          return player == player_t::environment;
        }
      );

      if(reach)
        return big_or(sigma, black::range(0, n + 1), [&](auto i) {
          return stepped(objective, i);
        });
      
      return big_or(sigma, black::range(0, n), [&](auto k) {
        auto loop = big_or(sigma, black::range(0, k), [&](auto j) {
          auto ell = [&](auto p) {
            return iff(stepped(p, k), stepped(p, j));
          };
          return 
            big_and(sigma, aut.inputs, ell) &&
            big_and(sigma, aut.outputs, ell);
        });

        auto safety = big_and(sigma, black::range(0, k + 1), [&](auto w) {
          return stepped(objective, w);
        });

        return loop && safety;
      });
    }

    bformula encoder::unravel(size_t n) {
      return 
        stepped(aut.init, 0) &&
        big_and(sigma, black::range(0, n), [&](auto i) {
          return stepped(aut.trans, i);
        });
    }

    qbformula encoder::encode(player_t player, game_t type, size_t n) 
    {
      using namespace logic::fragments::QBF;

      qbformula result = thereis(stepped(aut.variables, n), 
        unravel(n) && win(player, type, n)
      );

      // defaults for Controller
      std::vector<proposition> first = aut.outputs;
      std::vector<proposition> second = aut.inputs;
      quantifier_t qfirst = quantifier_t::thereis{};
      quantifier_t qsecond = quantifier_t::foreach{};

      if(player == player_t::environment) {
        first = aut.outputs;
        second = aut.inputs;
        qfirst = quantifier_t::foreach{};
        qsecond = quantifier_t::thereis{};
      }

      for(size_t i = 0; i < n; i++) {
        size_t step = n - i - 1;
        
        result = 
          thereis(stepped(aut.variables, step),
            qbf(qfirst, stepped(first, step),
              qbf(qsecond, stepped(second, step),
                result
              )
            )
          );
      }

      return result;
    }
  }

  static constexpr bool debug = true;

  static black::tribool solve(automata aut, game_t type) {
    logic::alphabet &sigma = *aut.init.sigma();

    if(debug)
      std::cerr << aut << "\n";

    size_t n = 3;
    while(true) {
      qbformula formulaC = 
        encoder{sigma, aut}.encode(player_t::controller, type, n);
      qdimacs qdC = clausify(formulaC);
      
      qbformula formulaE = 
        encoder{sigma, aut}.encode(player_t::environment, type, n);

      qdimacs qdE = clausify(formulaE);

      if(debug) {
        std::cerr << "- n = " << n << "\n";
        std::cerr << "formula: " << to_string(formulaE) << "\n";
      }

      if(is_sat(qdC) == true)
        return true;
      
      if(is_sat(qdE) == true)
        return false;
      
      n++;
    }
  }

  black::tribool is_realizable_novel(spec sp) 
  {
    if(auto ppspec = to_purepast(sp); ppspec) 
      return is_realizable_novel(*ppspec);

    automata aut = encode(sp);

    return solve(aut, game_t::eventually{});
  }

  black::tribool is_realizable_novel(purepast_spec sp) 
  {
    automata aut = encode(sp);

    return solve(aut, sp.type);
  }

}
