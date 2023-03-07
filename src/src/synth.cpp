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

#include <synthetico/synth.hpp>

#include <black/logic/logic.hpp>
#include <black/logic/parser.hpp>
#include <black/logic/prettyprint.hpp>

namespace synth {
  

  std::string to_string(spec s) {
    return s.type.match(
      [&](spec::type_t::eventually) {
        return "F(" + to_string(s.formula) + ")";
      },
      [&](spec::type_t::always) {
        return "G(" + to_string(s.formula) + ")";
      }
    );
  }

  std::optional<spec> 
  parse(
    black::alphabet &sigma,
    std::string formula, std::function<void(std::string)> error
  ) {
    auto f = black::parse_formula(sigma, formula, error);

    if(!f)
      return std::nullopt;

    return f->match(
      [&](black::only<FG> fg) -> std::optional<spec> {
        spec::type_t t = fg.node_type();
        auto arg = fg.to<black::unary>()->argument().to<logic::formula<pLTL>>();
        if(!arg) {
          error("only F(pLTL) or G(pLTL) formulas are supported");
          return std::nullopt;
        }

        return spec{ .type = t, .formula = *arg};
      },
      [&](black::otherwise) {
        error("only F(pLTL) or G(pLTL) formulas are supported");
        return std::nullopt;
      }
    );
  }

}
