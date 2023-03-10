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

#include <synthetico/automata.hpp>

#include <black/logic/prettyprint.hpp>

#include <unordered_set>

namespace synth {
  
  namespace {
    struct encoder {

      static formula<pLTL> nnf(formula<pLTL> f);

      void collect(spec sp);

      static proposition ground(formula<pLTL> y);

      static formula<pLTL> lift(proposition p);

      static bformula snf(formula<pLTL> f);

      automata encode(spec sp);

      std::vector<logic::yesterday<pLTL>> yreqs;
      std::vector<logic::w_yesterday<pLTL>> zreqs;
      std::vector<proposition> variables;

    };
  
    formula<pLTL> encoder::nnf(formula<pLTL> f) {
      return f.match(
        [](logic::boolean b) { return b; },
        [](proposition p) { return p; },
        [](logic::disjunction<pLTL>, auto left, auto right) {
          return nnf(left) || nnf(right);
        },
        [](logic::conjunction<pLTL>, auto left, auto right) {
          return nnf(left) && nnf(right);
        },
        [](logic::implication<pLTL>, auto left, auto right) {
          return nnf(!left || right);
        },
        [](logic::iff<pLTL>, auto left, auto right) {
          return nnf(implies(left, right) && implies(right, left));
        },
        [](logic::yesterday<pLTL>, auto arg) {
          return Y(nnf(arg));
        },
        [](logic::w_yesterday<pLTL>, auto arg) {
          return Z(nnf(arg));
        },
        [](logic::once<pLTL>, auto arg) {
          return O(nnf(arg));
        },
        [](logic::historically<pLTL>, auto arg) {
          return H(nnf(arg));
        },
        [](logic::since<pLTL>, auto left, auto right) {
          return S(nnf(left), nnf(right));
        },
        [](logic::triggered<pLTL>, auto left, auto right) {
          return T(nnf(left), nnf(right));
        },
        [](logic::negation<pLTL>, auto arg) {
          return arg.match(
            [](logic::boolean b) { return b.sigma()->boolean(!b.value()); },
            [](logic::proposition p) { return !p; },
            [](logic::negation<pLTL>, auto op) { return op; },
            [](logic::disjunction<pLTL>, auto left, auto right) {
              return nnf(!left) && nnf(!right);
            },
            [](logic::conjunction<pLTL>, auto left, auto right) {
              return nnf(!left) || nnf(!right);
            },
            [](logic::implication<pLTL>, auto left, auto right) {
              return nnf(left) && nnf(!right);
            },
            [](logic::iff<pLTL>, auto left, auto right) {
              return nnf(!implies(left,right)) || nnf(!implies(right,left));
            },
            [](logic::yesterday<pLTL>, auto op) {
              return Z(nnf(!op));
            },
            [](logic::w_yesterday<pLTL>, auto op) {
              return Y(nnf(!op));
            },
            [](logic::once<pLTL>, auto op) {
              return H(nnf(!op));
            },
            [](logic::historically<pLTL>, auto op) {
              return O(nnf(!op));
            },
            [](logic::since<pLTL>, auto left, auto right) {
              return T(nnf(!left), nnf(!right));
            },
            [](logic::triggered<pLTL>, auto left, auto right) {
              return S(nnf(!left), nnf(!right));
            }
          );
        }
      );
    }

    void encoder::collect(spec sp) {

      std::unordered_set<proposition> _variables;
      std::unordered_set<logic::yesterday<pLTL>> _yreqs;
      std::unordered_set<logic::w_yesterday<pLTL>> _zreqs;

      sp.type.match(
        [&](game_t::eventually) {
          _variables.insert(ground(Y(sp.formula)));
          _yreqs.insert(Y(sp.formula));
        },
        [&](game_t::always) {
          _variables.insert(ground(Z(sp.formula)));
          _zreqs.insert(Z(sp.formula));    
        }
      );    

      transform(sp.formula, [&](auto child) {
        child.match(
          [&](logic::yesterday<pLTL> y) {
            _variables.insert(ground(y));
            _yreqs.insert(y);
          },
          [&](logic::w_yesterday<pLTL> z) {
            _variables.insert(ground(z));
            _zreqs.insert(z);
          },
          [&](logic::since<pLTL> s) {
            _variables.insert(ground(Y(s)));
            _yreqs.insert(Y(s));
          },
          [&](logic::triggered<pLTL> s) {
            _variables.insert(ground(Z(s)));
            _zreqs.insert(Z(s));
          },
          [&](logic::once<pLTL> o) {
            _variables.insert(ground(Y(o)));
            _yreqs.insert(Y(o));
          },
          [&](logic::historically<pLTL> h) {
            _variables.insert(ground(Z(h)));
            _zreqs.insert(Z(h));
          },
          [](otherwise) { }
        );
      });

      variables.insert(variables.begin(), _variables.begin(), _variables.end());
      yreqs.insert(yreqs.begin(), _yreqs.begin(), _yreqs.end());
      zreqs.insert(zreqs.begin(), _zreqs.begin(), _zreqs.end());
    }

    proposition encoder::ground(formula<pLTL> f) {
      return f.sigma()->proposition(f);
    }

    formula<pLTL> encoder::lift(proposition p) {
      auto name = p.name().to<formula<pLTL>>();
      black_assert(name);
      
      return *name;
    }
    
    bformula encoder::snf(formula<pLTL> f) {
      return f.match(
        [](logic::boolean b) { return b; },
        [](logic::proposition p) { return p; },
        [](logic::negation<pLTL>, auto arg) { 
          return !snf(arg);
        },
        [](logic::disjunction<pLTL>, auto left, auto right) {
          return snf(left) || snf(right);
        },
        [](logic::conjunction<pLTL>, auto left, auto right) {
          return snf(left) && snf(right);
        },
        [](logic::yesterday<pLTL> y) {
          return ground(y);
        },
        [](logic::w_yesterday<pLTL> z) {
          return ground(z);
        },
        [](logic::once<pLTL>, auto arg) {
          return snf(arg) || ground(Y(O(arg)));
        },
        [](logic::historically<pLTL>, auto arg) {
          return snf(arg) && ground(Z(H(arg)));
        },
        [](logic::since<pLTL>, auto left, auto right) {
          return snf(right) || (snf(left) && ground(Y(S(left, right))));
        },
        [](logic::triggered<pLTL>, auto left, auto right) {
          return snf(right) && (snf(left) || ground(Z(T(left, right))));
        },
        [](logic::implication<pLTL>) -> bformula { black_unreachable(); },
        [](logic::iff<pLTL>) -> bformula { black_unreachable(); }
      );
    }

    automata encoder::encode(spec sp) {
      logic::alphabet &sigma = *sp.formula.sigma();
      sp.formula = encoder::nnf(sp.formula);

      collect(sp);

      bformula init = 
        big_and(sigma, zreqs, [](auto req) {
          return ground(req);
        }) && 
        big_and(sigma, yreqs, [](auto req) {
          return !ground(req);
        });

      bformula trans = big_and(sigma, variables, [](proposition var) {
        auto req = lift(var).to<logic::unary<pLTL>>();
        black_assert(req);
        
        return logic::iff(primed(var), snf(req->argument()));
      });

      bformula objective = sp.type.match(
        [&](game_t::eventually) {
          return ground(Y(sp.formula));
        },
        [&](game_t::always) {
          return ground(Z(sp.formula));
        }
      );

      return automata{sp.inputs, sp.outputs, variables, init, trans, objective};
    }
  }

  automata encode(spec sp) {
    return encoder{}.encode(sp);
  }

  std::ostream &operator<<(std::ostream &str, automata aut) {
    str << "inputs:\n";
    for(auto in : aut.inputs) {
      str << "- " << to_string(in) << "\n";
    }
    
    str << "\noutputs:\n";
    for(auto out : aut.outputs) {
      str << "- " << to_string(out) << "\n";
    }

    str << "\ninit:\n";
    str << "- " << to_string(aut.init) << "\n";
    
    str << "\ntrans:\n";
    str << "- " << to_string(aut.trans) << "\n";
    
    str << "\nobjective:\n";
    str << "- " << to_string(aut.objective) << "\n";

    return str;
  }
}
