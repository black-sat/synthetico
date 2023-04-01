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

#ifndef SYNTH_AUTOMATA_HPP
#define SYNTH_AUTOMATA_HPP

#include <black/logic/logic.hpp>

#include <synthetico/common.hpp>

#include <ostream>

namespace synth {
  
  namespace logic = black::logic;

  struct automata {
    std::vector<proposition> inputs;
    std::vector<proposition> outputs;
    std::vector<proposition> variables;

    bformula init;
    qbformula trans;
    bformula objective;
  };

  inline std::ostream &operator<<(std::ostream &str, automata aut) {
    str << "inputs:\n";
    for(auto in : aut.inputs) {
      str << "- " << to_string(in) << "\n";
    }
    
    str << "\noutputs:\n";
    for(auto out : aut.outputs) {
      str << "- " << to_string(out) << "\n";
    }
    
    str << "\nvariables:\n";
    for(auto var : aut.variables) {
      str << "- " << to_string(var) << "\n";
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

#endif // SYNTH_AUTOMATA_HPP
