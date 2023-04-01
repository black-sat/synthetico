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

namespace synth {

  qbformula rename(qbformula f, renaming_t renaming) {
    using namespace logic::fragments::QBF;

    return f.match(
      [](boolean b) { return b; },
      [&](proposition p) { return renaming(p); },
      [&](negation, auto arg) {
        return !rename(arg, renaming);
      },
      [&](binary b, auto left, auto right) {
        return binary(
          b.node_type(), rename(left, renaming), rename(right, renaming)
        );
      },
      [&](qbf q, auto vars, auto matrix) {
        std::vector<proposition> new_vars;
        for(auto p : vars)
          new_vars.push_back(renaming(p));

        return qbf(q.node_type(), new_vars, rename(matrix, renaming));
      }
    );
  }

  bformula rename(bformula f, renaming_t renaming) {
    return *rename(qbformula{f}, renaming).to<bformula>();
  }

  std::vector<proposition> 
  rename(std::vector<proposition> props, renaming_t renaming) {
    std::vector<proposition> result;
    for(auto p : props) {
      result.push_back(renaming(p));
    }

    return result;
  }

  proposition fresh_gen_t::operator()(proposition p) {
    if(auto name = p.name().to<fresh_t>(); name.has_value()) {
      return (*this)(name->prop);
    }

    return p.sigma()->proposition(fresh_t{p, next_fresh++});
  }


  proposition stepped(proposition p, size_t n) {
    if(auto pp = p.name().to<primed_t>(); pp.has_value()) {
      return stepped(p.sigma()->proposition(pp->label), n + 1);
    }
    
    return p.sigma()->proposition(stepped_t{p, n});
  }
  
  proposition stepped(proposition p) {
    if(auto pp = p.name().to<stepped_t>(); pp.has_value()) {
      return stepped(pp->prop, pp->step + 1);
    }
    
    return stepped(p, 0);
  }

}
