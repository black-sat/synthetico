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

#include <black/logic/logic.hpp>
#include <black/logic/parser.hpp>
#include <black/logic/prettyprint.hpp>

#include <unordered_set>

namespace synth {
  

  logic::formula<logic::LTLP> to_formula(spec sp) {
    return sp.type.match(
      [&](game_t::eventually) -> logic::formula<logic::LTLP> {
        return F(sp.formula);
      },
      [&](game_t::always) -> logic::formula<logic::LTLP> {
        return G(sp.formula);
      }
    );
  }

  std::ostream &operator<<(std::ostream &ostr, spec s) {
    std::string formula = to_string(to_formula(s));

    ostr << "formula: " << formula << "\n";
    ostr << "inputs: \n";
    for(auto i : s.inputs)
      ostr << "- " << to_string(i) << "\n";
    ostr << "outputs: \n";
    for(auto o : s.outputs)
      ostr << "- " << to_string(o) << "\n";

    return ostr;
  }

  std::optional<spec> 
  parse(
    black::alphabet &sigma,
    int argc, char **argv,
    std::function<void(std::string)> error
  ) {
    if(argc < 2) {
      error("insufficient command-line arguments");
      return std::nullopt;
    }

    auto f = black::parse_formula(sigma, argv[1], error);

    if(!f)
      return std::nullopt;

    return f->match(
      [&](black::only<FG> fg) -> std::optional<spec> {
        game_t t = fg.node_type();
        auto arg = fg.to<black::unary>()->argument().to<formula<pLTL>>();
        if(!arg) {
          error("only F(pLTL) or G(pLTL) formulas are supported");
          return std::nullopt;
        }

        std::unordered_set<proposition> iset;
        for(int i = 2; i < argc; i++)
          iset.insert(sigma.proposition(std::string(argv[i])));

        std::unordered_set<proposition> oset;
        transform(*arg, [&](auto child) {
          child.match(
            [&](proposition p) {
              if(iset.find(p) == iset.end())
                oset.insert(p);
            },
            [](otherwise) { }
          );
        });

        std::vector<proposition> inputs(iset.begin(), iset.end());
        std::vector<proposition> outputs(oset.begin(), oset.end());

        return spec{ 
          .type = t, .formula = *arg, 
          .inputs = inputs, .outputs = outputs
        };
      },
      [&](black::otherwise) {
        error("only F(pLTL) or G(pLTL) formulas are supported");
        return std::nullopt;
      }
    );
  }

}
