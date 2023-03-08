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

  struct dependency {
    var_t var;
    std::vector<var_t> deps;

    bool operator==(dependency const&) const = default;
  };

  static std::string to_string(dependency const& dep) {
    std::stringstream str;
    
    str << "d " << dep.var << " ";
    for(var_t d : dep.deps)
      str << d << " ";
    str << "0";

    return str.str();
  }

}

namespace std {
  template<>
  struct hash<::synth::dependency> {
    size_t operator()(::synth::dependency dep) const {
      using ::synth::var_t;
      size_t h = std::hash<var_t>{}(dep.var);

      for(var_t var : dep.deps) {
        h = black_internal::hash_combine(h, std::hash<var_t>{}(var));
      }
      
      return h;
    }
  };
}

namespace synth {

  struct clause {
    std::vector<lit_t> literals;
  };

  struct dqdimacs {
    std::vector<var_t> universals;
    std::vector<dependency> existentials;
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
  static dqdimacs clausify(prenex_qbf qbf) {
    black::cnf cnf = black::to_cnf(qbf.matrix);

    std::cout << to_string(to_formula(*qbf.matrix.sigma(), cnf)) << "\n";

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

    std::unordered_set<var_t> universals;
    std::unordered_set<var_t> existentials;
    std::unordered_set<dependency> deps;

    // quantifiers
    for(auto block : qbf.blocks) {
      if(block.node_type() == quantifier_t::foreach{}) {
        for(auto p : block.variables())
          universals.insert(vars[p]);
      } else {
        for(auto p : block.variables()) {
          existentials.insert(vars[p]);
          deps.insert(dependency{
            vars[p], std::vector(universals.begin(), universals.end())
          });
        }
      }
    }

    // quantifiers for Tseitin variables
    for(auto cl : cnf.clauses) {
      for(auto [sign, prop] : cl.literals) {
        var_t var = vars[prop];
        if(!existentials.contains(var)) {
          existentials.insert(var);
          deps.insert(dependency{var, {}});
        }
      }
    }


    return dqdimacs{
      std::vector(universals.begin(), universals.end()), 
      std::vector(deps.begin(), deps.end()), 
      clauses, props, vars
    };
  }

  std::string dimacs(qbf f) {
    prenex_qbf qbf = prenex(f);
    dqdimacs dqd = clausify(qbf);

    std::stringstream str;

    // header
    str << "p cnf " << dqd.universals.size() + dqd.existentials.size() 
        << " " << dqd.clauses.size() << "\n";

    // universal quantifiers
    str << "a ";
    for(var_t var : dqd.universals)
      str << var << " ";
    str << "0\n";

    // existential quantifiers
    for(auto dep : dqd.existentials)
      str << to_string(dep) << "\n";

    // clauses
    for(clause cl : dqd.clauses) {
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
