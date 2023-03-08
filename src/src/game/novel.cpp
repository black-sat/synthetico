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

  using namespace logic::fragments::propositional;

  using Quant = logic::make_fragment_t<
    logic::syntax_list<
      logic::syntax_element::thereis,
      logic::syntax_element::foreach
    >
  >;

  using qbf = logic::formula<logic::QBF>;
  using game_t = spec::type_t;

  enum class player_t {
    controller,
    environment
  };

  struct stepped_t {
    logic::proposition prop;
    size_t step;

    bool operator==(stepped_t const&) const = default;
  };

  static std::string to_string(stepped_t s) {
    return to_string(s.prop) + ", " + std::to_string(s.step);
  }

  struct encoder {

    //static formula win(game_t type, size_t n);

    static proposition stepped(proposition p, size_t n);
    static formula stepped(formula f, size_t n);
    
    static 
    std::vector<proposition> stepped(std::vector<proposition> props, size_t n);

    formula win(game_t type, size_t n);
    formula unravel(size_t n);

    qbf encode(player_t player, game_t type, size_t n);

    logic::alphabet &sigma;
    automata aut;

  };

  proposition encoder::stepped(proposition p, size_t n) {
    if(auto pp = p.name().to<primed_t>(); pp.has_value()) {
      return stepped(p.sigma()->proposition(pp->label), n + 1);
    }
    
    return p.sigma()->proposition(stepped_t{p, n});
  }

  formula encoder::stepped(formula f, size_t n) {
    return f.match(
      [](boolean b) { return b; },
      [&](proposition p) {
        return stepped(p, n); 
      },
      [&](negation, auto arg) {
        return !stepped(arg, n);
      },
      [&](conjunction, auto left, auto right) { 
        return stepped(left, n) && stepped(right, n);
      },
      [&](disjunction, auto left, auto right) {
        return stepped(left, n) || stepped(right, n);
      },
      [&](implication, auto left, auto right) {
        return implies(stepped(left, n), stepped(right, n));
      },
      [&](iff, auto left, auto right) {
        return iff(stepped(left, n), stepped(right, n));
      }
    );
  }

  std::vector<proposition> 
  encoder::stepped(std::vector<proposition> props, size_t n) {
    std::vector<proposition> result;
    for(auto p : props)
      result.push_back(stepped(p, n));
    
    return result;
  }

  formula encoder::win(game_t type, size_t n) {
    return type.match(
      [&](game_t::eventually) {
        return big_and(sigma, black::range(0, n), [&](auto i) {
          return stepped(aut.objective, i);
        });
      },
      [&](game_t::always) {
        return big_or(sigma, black::range(0, n), [&](auto k) {
          auto loop = big_or(sigma, black::range(0, k-1), [&](auto j) {
            auto ell = [&](auto p) {
              return iff(stepped(p, k), stepped(p, j));
            };
            return 
              big_and(sigma, aut.inputs, ell) &&
              big_and(sigma, aut.outputs, ell);
          });

          auto safety = big_and(sigma, black::range(0, k), [&](auto w) {
            return stepped(aut.objective, w);
          });

          return loop && safety;
        });
      }
    );
  }

  formula encoder::unravel(size_t n) {
    if(n == 0)
      return stepped(aut.init, 0);

    return 
      stepped(aut.init, 0) &&
      big_and(sigma, black::range(0, n-1), [&](auto i) {
        return stepped(aut.trans, i);
      });
  }

  qbf encoder::encode(player_t player, game_t type, size_t n) 
  {
    using quantifier_t = logic::qbf<logic::QBF>::type;

    qbf result = thereis(stepped(aut.variables, n), unravel(n) && win(type, n));

    // defaults for Controller
    std::vector<proposition> first = aut.outputs;
    std::vector<proposition> second = aut.inputs;
    quantifier_t qfirst = quantifier_t::thereis{};
    quantifier_t qsecond = quantifier_t::foreach{};

    if(player == player_t::environment) {
      first = aut.inputs;
      second = aut.outputs;
      qfirst = quantifier_t::foreach{};
      qsecond = quantifier_t::thereis{};
    }

    for(size_t i = 0; i < n; i++) {
      size_t step = n - i - 1;
      
      result = 
        thereis(stepped(aut.variables, step),
          logic::qbf<logic::QBF>(qfirst, stepped(first, step),
            logic::qbf<logic::QBF>(qsecond, stepped(second, step),
              result
            )
          )
        );
    }

    return result;
  }

  bool is_realizable_novel(spec sp) {

    alphabet &sigma = *sp.formula.sigma();

    automata aut = encode(sp);
    qbf formula = encoder{sigma, aut}.encode(player_t::controller, sp.type, 1);

    std::cout << "Encoding at n = 1:\n";
    std::cout << to_string(formula) << "\n";

    return false;
  }

}

namespace std {
  template<>
  struct hash<::synth::stepped_t> {
    size_t operator()(::synth::stepped_t s) {
      return ::black_internal::hash_combine(
        std::hash<::black::proposition>{}(s.prop),
        std::hash<size_t>{}(s.step)
      );
    }
  };
}
