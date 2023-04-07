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

#include <synthetico/encoding/full/full.hpp>
#include <synthetico/encoding/full/det.hpp>

#include <black/logic/prettyprint.hpp>

#include <unordered_set>

namespace synth {
  
  namespace {
    struct encoder {

      static formula<LTLP> nnf(formula<LTLP> f);

      void collect(spec sp);

      static proposition ground(formula<LTLP> y);

      static formula<LTLP> lift(proposition p);

      static bformula snf(formula<LTLP> f, bool prime);

      automaton encode(spec sp);

      std::vector<logic::tomorrow<LTLP>> xreqs;
      std::vector<logic::w_tomorrow<LTLP>> wxreqs;
      std::vector<logic::yesterday<LTLP>> yreqs;
      std::vector<logic::w_yesterday<LTLP>> zreqs;
      std::vector<proposition> variables;

    };
  
    formula<LTLP> encoder::nnf(formula<LTLP> f) {
      return f.match(
        [](logic::boolean b) { return b; },
        [](proposition p) { return p; },
        [](logic::disjunction<LTLP>, auto left, auto right) {
          return nnf(left) || nnf(right);
        },
        [](logic::conjunction<LTLP>, auto left, auto right) {
          return nnf(left) && nnf(right);
        },
        [](logic::implication<LTLP>, auto left, auto right) {
          return nnf(!left || right);
        },
        [](logic::iff<LTLP>, auto left, auto right) {
          return nnf(implies(left, right) && implies(right, left));
        },
        [](logic::tomorrow<LTLP>, auto arg) { 
          return X(nnf(arg));
        },
        [](logic::w_tomorrow<LTLP>, auto arg) { 
          return wX(nnf(arg));
        },
        [](logic::yesterday<LTLP>, auto arg) {
          return Y(nnf(arg));
        },
        [](logic::w_yesterday<LTLP>, auto arg) {
          return Z(nnf(arg));
        },
        [](logic::eventually<LTLP>, auto arg) {
          return F(nnf(arg));
        },
        [](logic::always<LTLP>, auto arg) {
          return G(nnf(arg));
        },
        [](logic::once<LTLP>, auto arg) {
          return O(nnf(arg));
        },
        [](logic::historically<LTLP>, auto arg) {
          return H(nnf(arg));
        },
        [](logic::until<LTLP>, auto left, auto right) {
          return U(nnf(left), nnf(right));
        },
        [](logic::release<LTLP>, auto left, auto right) {
          return R(nnf(left), nnf(right));
        },
        [](logic::w_until<LTLP>, auto left, auto right) {
          return W(nnf(left), nnf(right));
        },
        [](logic::s_release<LTLP>, auto left, auto right) {
          return M(nnf(left), nnf(right));
        },
        [](logic::since<LTLP>, auto left, auto right) {
          return S(nnf(left), nnf(right));
        },
        [](logic::triggered<LTLP>, auto left, auto right) {
          return T(nnf(left), nnf(right));
        },
        [](logic::negation<LTLP>, auto arg) {
          return arg.match(
            [](logic::boolean b) { return b.sigma()->boolean(!b.value()); },
            [](logic::proposition p) { return !p; },
            [](logic::negation<LTLP>, auto op) { return op; },
            [](logic::disjunction<LTLP>, auto left, auto right) {
              return nnf(!left) && nnf(!right);
            },
            [](logic::conjunction<LTLP>, auto left, auto right) {
              return nnf(!left) || nnf(!right);
            },
            [](logic::implication<LTLP>, auto left, auto right) {
              return nnf(left) && nnf(!right);
            },
            [](logic::iff<LTLP>, auto left, auto right) {
              return nnf(!implies(left,right)) || nnf(!implies(right,left));
            },
            [](logic::tomorrow<LTLP>, auto op) {
              return wX(nnf(!op));
            },
            [](logic::w_tomorrow<LTLP>, auto op) {
              return X(nnf(!op));
            },
            [](logic::yesterday<LTLP>, auto op) {
              return Z(nnf(!op));
            },
            [](logic::w_yesterday<LTLP>, auto op) {
              return Y(nnf(!op));
            },
            [](logic::eventually<LTLP>, auto op) {
              return G(nnf(!op));
            },
            [](logic::always<LTLP>, auto op) {
              return F(nnf(!op));
            },
            [](logic::once<LTLP>, auto op) {
              return H(nnf(!op));
            },
            [](logic::historically<LTLP>, auto op) {
              return O(nnf(!op));
            },
            [](logic::until<LTLP>, auto left, auto right) {
              return R(nnf(!left), nnf(!right));
            },
            [](logic::release<LTLP>, auto left, auto right) {
              return U(nnf(!left), nnf(!right));
            },
            [](logic::w_until<LTLP>, auto left, auto right) {
              return M(nnf(!left), nnf(!right));
            },
            [](logic::s_release<LTLP>, auto left, auto right) {
              return W(nnf(!left), nnf(!right));
            },
            [](logic::since<LTLP>, auto left, auto right) {
              return T(nnf(!left), nnf(!right));
            },
            [](logic::triggered<LTLP>, auto left, auto right) {
              return S(nnf(!left), nnf(!right));
            }
          );
        }
      );
    }

    void encoder::collect(spec sp) {

      std::unordered_set<proposition> _variables;
      std::unordered_set<logic::tomorrow<LTLP>> _xreqs;
      std::unordered_set<logic::w_tomorrow<LTLP>> _wxreqs;
      std::unordered_set<logic::yesterday<LTLP>> _yreqs;
      std::unordered_set<logic::w_yesterday<LTLP>> _zreqs;

      _variables.insert(ground(X(sp.formula)));
      _xreqs.insert(X(sp.formula));
      
      transform(sp.formula, [&](auto child) {
        child.match(
          [&](logic::tomorrow<LTLP> x) {
            _variables.insert(ground(x));
            _xreqs.insert(x);
          },
          [&](logic::w_tomorrow<LTLP> wx) {
            _variables.insert(ground(wx));
            _wxreqs.insert(wx);
          },
          [&](logic::yesterday<LTLP> y) {
            _variables.insert(ground(y));
            _yreqs.insert(y);
          },
          [&](logic::w_yesterday<LTLP> z) {
            _variables.insert(ground(z));
            _zreqs.insert(z);
          },
          [&](logic::until<LTLP> u) {
            _variables.insert(ground(X(u)));
            _xreqs.insert(X(u));
          },
          [&](logic::release<LTLP> r) {
            _variables.insert(ground(wX(r)));
            _wxreqs.insert(wX(r));
          },
          [&](logic::w_until<LTLP> u) {
            _variables.insert(ground(wX(u)));
            _wxreqs.insert(wX(u));
          },
          [&](logic::s_release<LTLP> r) {
            _variables.insert(ground(X(r)));
            _xreqs.insert(X(r));
          },
          [&](logic::eventually<LTLP> f) {
            _variables.insert(ground(X(f)));
            _xreqs.insert(X(f));
          },
          [&](logic::always<LTLP> g) {
            _variables.insert(ground(wX(g)));
            _wxreqs.insert(wX(g));
          },
          [&](logic::since<LTLP> s) {
            _variables.insert(ground(Y(s)));
            _yreqs.insert(Y(s));
          },
          [&](logic::triggered<LTLP> s) {
            _variables.insert(ground(Z(s)));
            _zreqs.insert(Z(s));
          },
          [&](logic::once<LTLP> o) {
            _variables.insert(ground(Y(o)));
            _yreqs.insert(Y(o));
          },
          [&](logic::historically<LTLP> h) {
            _variables.insert(ground(Z(h)));
            _zreqs.insert(Z(h));
          },
          [](otherwise) { }
        );
      });

      variables.insert(variables.begin(), _variables.begin(), _variables.end());
      xreqs.insert(xreqs.begin(), _xreqs.begin(), _xreqs.end());
      wxreqs.insert(wxreqs.begin(), _wxreqs.begin(), _wxreqs.end());
      yreqs.insert(yreqs.begin(), _yreqs.begin(), _yreqs.end());
      zreqs.insert(zreqs.begin(), _zreqs.begin(), _zreqs.end());
    }

    proposition encoder::ground(formula<LTLP> f) {
      return f.sigma()->proposition(f);
    }

    formula<LTLP> encoder::lift(proposition p) {
      auto name = p.name().to<formula<LTLP>>();
      black_assert(name);
      
      return *name;
    }
    
    bformula encoder::snf(formula<LTLP> f, bool p) {
      auto _primed = [&](proposition prop){ 
        return p ? primed(prop) : prop;
      };
      return f.match(
        [](logic::boolean b) { return b; },
        [](logic::proposition prop) { return prop; },
        [&](logic::negation<LTLP>, auto arg) { 
          return !snf(arg, p);
        },
        [&](logic::disjunction<LTLP>, auto left, auto right) {
          return snf(left, p) || snf(right, p);
        },
        [&](logic::conjunction<LTLP>, auto left, auto right) {
          return snf(left, p) && snf(right, p);
        },
        [&](logic::tomorrow<LTLP> y) {
          return _primed(ground(y));
        },
        [&](logic::w_tomorrow<LTLP> z) {
          return _primed(ground(z));
        },
        [&](logic::yesterday<LTLP> y) {
          return _primed(ground(y));
        },
        [&](logic::w_yesterday<LTLP> z) {
          return _primed(ground(z));
        },
        [&](logic::eventually<LTLP>, auto arg) {
          return snf(arg, p) || _primed(ground(X(F(arg))));
        },
        [&](logic::always<LTLP>, auto arg) {
          return snf(arg, p) && _primed(ground(wX(G(arg))));
        },
        [&](logic::once<LTLP>, auto arg) {
          return snf(arg, p) || _primed(ground(Y(O(arg))));
        },
        [&](logic::historically<LTLP>, auto arg) {
          return snf(arg, p) && _primed(ground(Z(H(arg))));
        },
        [&](logic::until<LTLP>, auto left, auto right) {
          return 
            snf(right, p) || 
            (snf(left, p) && _primed(ground(X(U(left, right)))));
        },
        [&](logic::release<LTLP>, auto left, auto right) {
          return 
            snf(right, p) && 
            (snf(left, p) || _primed(ground(wX(R(left, right)))));
        },
        [&](logic::w_until<LTLP>, auto left, auto right) {
          return 
            snf(right, p) || 
            (snf(left, p) && _primed(ground(wX(W(left, right)))));
        },
        [&](logic::s_release<LTLP>, auto left, auto right) {
          return 
            snf(right, p) && 
            (snf(left, p) || _primed(ground(X(M(left, right)))));
        },
        [&](logic::since<LTLP>, auto left, auto right) {
          return 
            snf(right, p) || 
            (snf(left, p) && _primed(ground(Y(S(left, right)))));
        },
        [&](logic::triggered<LTLP>, auto left, auto right) {
          return 
            snf(right, p) && 
            (snf(left, p) || _primed(ground(Z(T(left, right)))));
        },
        [&](logic::implication<LTLP>) -> bformula { black_unreachable(); },
        [&](logic::iff<LTLP>) -> bformula { black_unreachable(); }
      );
    }

    automaton encoder::encode(spec sp) {
      logic::alphabet &sigma = *sp.formula.sigma();
      sp.formula = encoder::nnf(sp.formula);

      collect(sp);

      bformula init = ground(X(sp.formula));

      bformula trans = big_and(sigma, variables, [](proposition var) {
        auto req = lift(var).to<logic::unary<LTLP>>();
        black_assert(req);

        bool prime = true;
        req->match(
          [&](logic::yesterday<LTLP>) { 
            var = primed(var);
            prime = false;
          },
          [&](logic::w_yesterday<LTLP>) { 
            var = primed(var);
            prime = false;
          },
          [](otherwise) { }
        );

        return logic::iff(var, snf(req->argument(), prime));
      });

      bformula objective = big_and(sigma, xreqs, [](logic::tomorrow<LTLP> x) {
        return !ground(x);
      });

      return automaton{sp.inputs, sp.outputs, variables, init, trans, objective};
    }
  }

  automaton encode(spec sp) {
    return monoidet(encoder{}.encode(sp));
  }

}
