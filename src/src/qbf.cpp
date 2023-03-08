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

#include <black/logic/cnf.hpp>
#include <black/logic/prettyprint.hpp>

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iostream>

namespace synth {

  namespace logic = black::logic;
  using namespace logic::fragments::propositional;
  using quantifier_t = logic::qbf<logic::QBF>::type;

  using var_t = uint32_t;
  using lit_t = int64_t;

  struct prenex_qbf {
    std::vector<logic::qbf<logic::QBF>> blocks;
    logic::formula<logic::propositional> matrix;
  };

}

namespace synth {

  struct clause {
    std::vector<lit_t> literals;
  };

  struct qdimacs_block {
    enum {
      existential,
      universal
    } type;

    std::vector<var_t> variables;
  };

  struct qdimacs {
    size_t n_vars;
    std::vector<qdimacs_block> blocks;
    std::vector<clause> clauses;

    std::unordered_map<var_t, proposition> props;
    std::unordered_map<proposition, var_t> vars;
  };

  static prenex_qbf prenex(qbf f) 
  {
    std::vector<logic::qbf<logic::QBF>> blocks;
    qbf matrix = f;

    while(matrix.is<logic::qbf<logic::QBF>>()) {
      auto q = *matrix.to<logic::qbf<logic::QBF>>();
      blocks.push_back(q);
      matrix = q.matrix();
    }

    auto m = matrix.to<logic::formula<logic::propositional>>();
    black_assert(m.has_value());
    
    return prenex_qbf{blocks, *m};
  }

  [[maybe_unused]]
  static std::string to_string(black::cnf cnf) {
    std::stringstream str;

    for(black::clause cl : cnf.clauses) {
      str << " ∧\n";
      for(auto [sign, prop] : cl.literals) {
        if(sign)
          str << "  ∨ " << to_string(!prop) << "\n";
        else
          str << "  ∨ " << to_string(prop) << "\n";
      }
    }

    return str.str();
  }

  [[maybe_unused]]
  static qdimacs clausify(prenex_qbf qbf) {
    black::cnf cnf = black::to_cnf(qbf.matrix);

    //std::cout << to_string(cnf) << "\n";

    std::unordered_map<var_t, proposition> props;
    std::unordered_map<proposition, var_t> vars;

    // clauses
    std::vector<clause> clauses;

    var_t next_var = 1;
    for(auto cl : cnf.clauses) {
      std::vector<lit_t> literals;
      for(auto [sign, prop] : cl.literals) {
        var_t var;
        if(auto it = vars.find(prop); it != vars.end()) 
          var = it->second;
        else {
          var = next_var++;
          props.insert({var, prop});
          vars.insert({prop, var});
        }

        literals.push_back(sign ? -lit_t{var} : lit_t{var});
      }
      clauses.push_back(clause{literals});
    }

    std::vector<qdimacs_block> blocks;
    std::unordered_set<var_t> declared_vars;

    // quantifiers
    for(auto block : qbf.blocks) {
      
      std::vector<var_t> block_vars;
      for(auto p : block.variables()) {
        declared_vars.insert(vars[p]);
        block_vars.push_back(vars[p]);
      }

      if(block.node_type() == quantifier_t::thereis{})
        blocks.push_back(qdimacs_block{qdimacs_block::existential, block_vars});
      else
        blocks.push_back(qdimacs_block{qdimacs_block::universal, block_vars});

    }

    // quantifiers for Tseitin variables
    std::unordered_set<var_t> last;
    for(auto cl : cnf.clauses) {
      for(auto [sign, prop] : cl.literals) {
        var_t var = vars[prop];
        if(!declared_vars.contains(var)) {
          last.insert(var);
        }
      }
    }
    if(!last.empty())
      blocks.push_back(qdimacs_block{
        qdimacs_block::existential, std::vector(last.begin(), last.end())
      });

    return qdimacs{next_var - 1, blocks, clauses, props, vars};
  }

  std::string dimacs(qbf f) {
    prenex_qbf qbf = prenex(f);
    qdimacs qd = clausify(qbf);

    std::stringstream str;

    // header
    str << "p cnf " << qd.n_vars << " " << qd.clauses.size() << "\n";

    // quantifiers
    for(auto block : qd.blocks) {
      if(block.type == qdimacs_block::existential)
        str << "e ";
      else
        str << "a ";

      for(var_t var : block.variables) 
        str << var << " ";
      str << "0\n";
    }

    // clauses
    for(clause cl : qd.clauses) {
      for(lit_t lit : cl.literals) {
        str << lit << " ";
      }
      str << "0\n";
    }

    return str.str();
  }


  bool is_sat(qbf) {
    return false;
  }

}
