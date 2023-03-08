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

  namespace logic = black::logic;

  struct prenex_qbf {
    std::vector<logic::qbf<logic::QBF>> blocks;
    logic::formula<logic::propositional> matrix;
  };

  static std::optional<prenex_qbf> prenex(qbf f) 
  {
    std::vector<logic::qbf<logic::QBF>> blocks;
    qbf matrix = f;

    while(matrix.is<logic::qbf<logic::QBF>>()) {
      auto q = *matrix.to<logic::qbf<logic::QBF>>();
      blocks.push_back(q);
      matrix = q.matrix();
    }

    if(auto m = matrix.to<logic::qbf<logic::propositional>>(); m)
      return prenex_qbf{blocks, *m};

    return {};
  }

  bool is_sat(logic::formula<logic::QBF>) {
    return false;
  }

}
