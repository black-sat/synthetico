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

#include "synthetico/synthetico.hpp"

#include <black/internal/debug/random_formula.hpp>

#include <random>

namespace synth {

  static logic::formula<pLTL> mirror(logic::formula<logic::LTL> f) {
    using namespace logic;

    return f.match(
      [](boolean b) { return b; },
      [](proposition p) { return p; },
      [](negation<LTL>, auto arg) {
        return !mirror(arg);
      },
      [](conjunction<LTL>, auto left, auto right) { 
        return mirror(left) && mirror(right);
      },
      [](disjunction<LTL>, auto left, auto right) { 
        return mirror(left) || mirror(right);
      },
      [](implication<LTL>, auto left, auto right) {
        return implies(mirror(left), mirror(right));
      },
      [](iff<LTL>, auto left, auto right) {
        return iff(mirror(left), mirror(right));
      },
      [](tomorrow<LTL>, auto arg) {
        return Y(mirror(arg));
      },
      [](w_tomorrow<LTL>, auto arg) {
        return Z(mirror(arg));
      },
      [](eventually<LTL>, auto arg) {
        return O(mirror(arg));
      },
      [](always<LTL>, auto arg) {
        return H(mirror(arg));
      },
      [](until<LTL>, auto left, auto right) {
        return S(mirror(left), mirror(right));
      },
      [](release<LTL>, auto left, auto right) {
        return T(mirror(left), mirror(right));
      },
      [](w_until<LTL>, auto left, auto right) {
        return S(mirror(left), mirror(right));
      },
      [](s_release<LTL>, auto left, auto right) {
        return T(mirror(left), mirror(right));
      }
    );
  }

  static logic::formula<pLTL> replace_consts(logic::formula<pLTL> f) {
    using namespace logic;

    return f.match(
      [](boolean b) {
        if(b.value())
          return b.sigma()->proposition("c0");
        return b.sigma()->proposition("u0");
      },
      [](proposition p) { return p; },
      [](yesterday<pLTL> y, formula<pLTL> arg) {
        if(arg.is<boolean>())
          return y;
        return Y(replace_consts(arg));
      },
      [](w_yesterday<pLTL> y, formula<pLTL> arg) {
        if(arg.is<boolean>())
          return y;
        return Z(replace_consts(arg));
      },
      [](unary<pLTL> u, auto arg) {
        return unary<pLTL>(u.node_type(), replace_consts(arg));
      },
      [](binary<pLTL> b, auto left, auto right) {
        return binary<pLTL>(
          b.node_type(), replace_consts(left), replace_consts(right)
        );
      }
    );
  }

  spec random_spec(
    logic::alphabet &sigma, 
    std::mt19937 &gen, size_t nsymbols, size_t size
  ) {
    using namespace logic;

    game_t type = game_t::eventually{};
    if(std::bernoulli_distribution{}(gen))
       type = game_t::always{};

    if(nsymbols < 2)
      nsymbols = 2;
    
    std::uniform_int_distribution<size_t> dist{1, nsymbols - 1};
    size_t ninputs = dist(gen);
    size_t noutputs = nsymbols - ninputs;

    std::vector<std::string> symbols;
    std::vector<proposition> inputs;
    std::vector<proposition> outputs;

    for(size_t i = 0; i < ninputs; i++) {
      std::string symbol = "u" + std::to_string(i);
      symbols.push_back(symbol);
      inputs.push_back(sigma.proposition(symbol));
    }

    for(size_t i = 0; i < noutputs; i++) {
      std::string symbol = "c" + std::to_string(i);
      symbols.push_back(symbol);
      outputs.push_back(sigma.proposition(symbol));
    }

    formula<pLTL> f = 
      replace_consts(mirror(black_internal::random::random_ltl_formula(
        gen, sigma, (int)size, symbols
      )));

    return spec{type, f, inputs, outputs};
  }

}
