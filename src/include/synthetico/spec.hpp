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

#ifndef SYNTH_SPEC_HPP
#define SYNTH_SPEC_HPP

#include <functional>
#include <string>
#include <optional>
#include <ostream>

#include <black/logic/logic.hpp>

namespace synth {
  
  namespace logic = black::logic;

  using FG = logic::make_fragment_t<
    logic::syntax_list<
      logic::syntax_element::eventually,
      logic::syntax_element::always
    >
  >;

  using pLTL = logic::make_combined_fragment_t<
    logic::propositional,
    logic::make_fragment_t<
      logic::syntax_list<
        logic::syntax_element::yesterday,
        logic::syntax_element::w_yesterday,
        logic::syntax_element::since,
        logic::syntax_element::triggered,
        logic::syntax_element::once,
        logic::syntax_element::historically
      >
    >
  >;

  struct spec {
    using type_t = logic::formula<FG>::type;
    
    type_t type;
    logic::formula<pLTL> formula;
    std::vector<logic::proposition> inputs;
    std::vector<logic::proposition> outputs;
  };

  std::ostream &operator<<(std::ostream &ostr, spec s);

  std::optional<spec> 
  parse(
    black::alphabet &sigma,
    int argc, char **argv,
    std::function<void(std::string)> error
  );

}

#endif // SYNTH_SPEC_HPP
