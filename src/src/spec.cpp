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

#include <black/logic/logic.hpp>
#include <black/logic/parser.hpp>
#include <black/logic/prettyprint.hpp>

#include <unordered_set>

namespace synth {
  

  tformula to_formula(purepast_spec sp) {
    return sp.type.match(
      [&](game_t::eventually) -> tformula {
        return F(sp.formula);
      },
      [&](game_t::always) -> tformula {
        return G(sp.formula);
      }
    );
  }

  spec to_spec(purepast_spec sp) {
    return spec {
      .formula = to_formula(sp),
      .inputs = sp.inputs,
      .outputs = sp.outputs
    };
  }

  std::ostream &operator<<(std::ostream &ostr, spec s) {
    std::string formula = to_string(s.formula);

    ostr << "formula: " << formula << "\n";
    ostr << "inputs: \n";
    for(auto i : s.inputs)
      ostr << "- " << to_string(i) << "\n";
    ostr << "outputs: \n";
    for(auto o : s.outputs)
      ostr << "- " << to_string(o) << "\n";

    return ostr;
  }

  std::ostream &operator<<(std::ostream &ostr, purepast_spec s) {
    return ostr << to_spec(s);
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

    auto parsed = black::parse_formula(sigma, argv[1], error);

    if(!parsed)
      return std::nullopt;

    auto f = parsed->to<tformula>();
    if(!f) {
      error("Only LTL+P formulas are supported");
      return std::nullopt;
    }

    std::unordered_set<proposition> iset;
    for(int i = 2; i < argc; i++)
      iset.insert(sigma.proposition(std::string(argv[i])));

    std::unordered_set<proposition> oset;
    transform(*f, [&](auto child) {
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
      .formula = *f, 
      .inputs = inputs, .outputs = outputs
    };
  }

  std::optional<purepast_spec> to_purepast(spec sp) {
    return sp.formula.match(
      [&](black::only<FG> fg) -> std::optional<purepast_spec> {
        auto arg = fg.to<logic::unary<LTLP>>()->argument().to<formula<pLTL>>();
        if(!arg)
          return std::nullopt;
        
        return purepast_spec {
          .type = fg.node_type(), .formula = *arg,
          .inputs = sp.inputs, .outputs = sp.outputs
        };
      },
      [](otherwise) -> std::optional<purepast_spec> { 
        return std::nullopt;
      }
    );
  }

}
