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
  
  using namespace logic;

  namespace {
    struct encoder {

      static formula<pLTL> nnf(formula<pLTL> f);

      void collect(spec sp);

      static proposition ground(formula<pLTL> y);

      static formula<pLTL> lift(proposition p);

      static automata::formula snf(formula<pLTL> f);

      automata encode(spec sp);

      std::vector<yesterday<pLTL>> yreqs;
      std::vector<w_yesterday<pLTL>> zreqs;
      std::vector<proposition> variables;

    };
  
    formula<pLTL> encoder::nnf(formula<pLTL> f) {
      return f.match(
        [](boolean b) { return b; },
        [](proposition p) { return p; },
        [](disjunction<pLTL>, auto left, auto right) {
          return nnf(left) || nnf(right);
        },
        [](conjunction<pLTL>, auto left, auto right) {
          return nnf(left) && nnf(right);
        },
        [](implication<pLTL>, auto left, auto right) {
          return nnf(!left || right);
        },
        [](iff<pLTL>, auto left, auto right) {
          return nnf(implies(left, right) && implies(right, left));
        },
        [](yesterday<pLTL>, auto arg) {
          return Y(nnf(arg));
        },
        [](w_yesterday<pLTL>, auto arg) {
          return Z(nnf(arg));
        },
        [](once<pLTL>, auto arg) {
          return O(nnf(arg));
        },
        [](historically<pLTL>, auto arg) {
          return H(nnf(arg));
        },
        [](since<pLTL>, auto left, auto right) {
          return S(nnf(left), nnf(right));
        },
        [](triggered<pLTL>, auto left, auto right) {
          return T(nnf(left), nnf(right));
        },
        [](negation<pLTL>, auto arg) {
          return arg.match(
            [](boolean b) { return b.sigma()->boolean(!b.value()); },
            [](proposition p) { return !p; },
            [](negation<pLTL>, auto op) { return op; },
            [](disjunction<pLTL>, auto left, auto right) {
              return nnf(!left) && nnf(!right);
            },
            [](conjunction<pLTL>, auto left, auto right) {
              return nnf(!left) || nnf(!right);
            },
            [](implication<pLTL>, auto left, auto right) {
              return nnf(left) && nnf(!right);
            },
            [](iff<pLTL>, auto left, auto right) {
              return nnf(!implies(left,right)) || nnf(!implies(right,left));
            },
            [](yesterday<pLTL>, auto op) {
              return Z(nnf(!op));
            },
            [](w_yesterday<pLTL>, auto op) {
              return Y(nnf(!op));
            },
            [](once<pLTL>, auto op) {
              return H(nnf(!op));
            },
            [](historically<pLTL>, auto op) {
              return O(nnf(!op));
            },
            [](since<pLTL>, auto left, auto right) {
              return T(nnf(!left), nnf(!right));
            },
            [](triggered<pLTL>, auto left, auto right) {
              return S(nnf(!left), nnf(!right));
            }
          );
        }
      );
    }

    void encoder::collect(spec sp) {

      std::unordered_set<proposition> _variables;
      std::unordered_set<yesterday<pLTL>> _yreqs;
      std::unordered_set<w_yesterday<pLTL>> _zreqs;

      sp.type.match(
        [&](spec::type_t::eventually) {
          _variables.insert(ground(Y(sp.formula)));
          _yreqs.insert(Y(sp.formula));
        },
        [&](spec::type_t::always) {
          _variables.insert(ground(Z(sp.formula)));
          _zreqs.insert(Z(sp.formula));    
        }
      );    

      for_each_child_deep(sp.formula, [&](auto child) {
        child.match(
          [&](yesterday<pLTL> y) {
            _variables.insert(ground(y));
            _yreqs.insert(y);
          },
          [&](w_yesterday<pLTL> z) {
            _variables.insert(ground(z));
            _zreqs.insert(z);
          },
          [&](since<pLTL> s) {
            _variables.insert(ground(Y(s)));
            _yreqs.insert(Y(s));
          },
          [&](triggered<pLTL> s) {
            _variables.insert(ground(Z(s)));
            _zreqs.insert(Z(s));
          },
          [&](once<pLTL> o) {
            _variables.insert(ground(Y(o)));
            _yreqs.insert(Y(o));
          },
          [&](historically<pLTL> h) {
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
    
    automata::formula encoder::snf(formula<pLTL> f) {
      return f.match(
        [](boolean b) { return b; },
        [](proposition p) { return p; },
        [](negation<pLTL>, auto arg) { 
          return !snf(arg);
        },
        [](disjunction<pLTL>, auto left, auto right) {
          return snf(left) || snf(right);
        },
        [](conjunction<pLTL>, auto left, auto right) {
          return snf(left) && snf(right);
        },
        [](yesterday<pLTL> y) {
          return ground(y);
        },
        [](w_yesterday<pLTL> z) {
          return ground(z);
        },
        [](once<pLTL>, auto arg) {
          return snf(arg) || ground(Y(O(arg)));
        },
        [](historically<pLTL>, auto arg) {
          return snf(arg) && ground(Z(H(arg)));
        },
        [](since<pLTL>, auto left, auto right) {
          return snf(right) || (snf(left) && ground(Y(S(left, right))));
        },
        [](triggered<pLTL>, auto left, auto right) {
          return snf(right) && (snf(left) || ground(Z(T(left, right))));
        },
        [](implication<pLTL>) -> automata::formula { black_unreachable(); },
        [](iff<pLTL>) -> automata::formula { black_unreachable(); }
      );
    }

    automata encoder::encode(spec sp) {
      alphabet &sigma = *sp.formula.sigma();
      sp.formula = encoder::nnf(sp.formula);

      collect(sp);

      automata::formula init = 
        big_and(sigma, zreqs, [](auto req) {
          return ground(req);
        }) && 
        big_and(sigma, yreqs, [](auto req) {
          return !ground(req);
        });

      automata::formula trans = big_and(sigma, variables, [](proposition var) {
        auto req = lift(var).to<unary<pLTL>>();
        black_assert(req);
        
        return iff(primed(var), snf(req->argument()));
      });

      automata::formula objective = sp.type.match(
        [&](spec::type_t::eventually) {
          return ground(Y(sp.formula));
        },
        [&](spec::type_t::always) {
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
