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

  struct prenex_qbf {
    std::vector<logic::qbf<logic::QBF>> blocks;
    bformula matrix;
  };

  struct flatten_t {
      
    qbformula flatten(qbformula f);

    fresh_gen_t fresh;
    std::unordered_set<proposition> used;
  };

  qbformula flatten_t::flatten(qbformula f) {
    using namespace logic::fragments::QBF;

    return f.match(
      [](boolean b) { return b; },
      [](proposition p) { return p; },
      [&](negation, auto arg) {
        return !flatten(arg);
      },
      [&](binary b, auto left, auto right) {
        auto nleft = flatten(left);
        auto nright = flatten(right);
        return binary(b.node_type(), nleft, nright);
      },
      [&](qbf q, auto vars, auto matrix) {
        std::unordered_map<proposition, proposition> map;
        for(auto p : vars) {
          if(used.contains(p))
            map.insert({p, fresh(p)});
        }

        auto renaming = [&](proposition p) {
          if(map.contains(p))
            return map.at(p);
          return p;
        };

        auto new_vars = rename(vars, renaming);
        auto new_matrix = rename(matrix, renaming);
        used.insert(vars.begin(), vars.end());
        return qbf(q.node_type(), new_vars, flatten(new_matrix));
      }
    );
  }

  qbformula flatten(qbformula f) {
    return flatten_t{}.flatten(f);
  }

  qbformula prenex(qbformula f) {
    using namespace logic::fragments::QBF;

    auto dual = [](qbf::type t) {
      return t.match(
        [](qbf::type::thereis) { return qbf::type::foreach{}; },
        [](qbf::type::foreach) { return qbf::type::thereis{}; }
      );
    };

    auto result = f.match(
      [](boolean b) { return b; },
      [](proposition p) { return p; },
      [&](negation, auto arg) {
        return arg.match(
          [&](qbf q, auto vars, auto matrix) {
            return qbf(dual(q.node_type()), vars, prenex(!matrix));
          },
          [&](otherwise) {
            return !prenex(arg);
          }
        );
      },
      [](conjunction, qbformula left, qbformula right) -> qbformula { 
        auto pleft = prenex(left);
        auto pright = prenex(right);

        if(auto ql = pleft.to<qbf>(); ql) {
          return qbf(
            ql->node_type(), ql->variables(), prenex(ql->matrix() && pright)
          );
        }
        if(auto qr = pright.to<qbf>(); qr) {
          return qbf(
            qr->node_type(), qr->variables(), prenex(pleft && qr->matrix())
          );
        }

        return pleft && pright;
      },
      [](disjunction, qbformula left, qbformula right) -> qbformula { 
        auto pleft = prenex(left);
        auto pright = prenex(right);

        if(auto ql = pleft.to<qbf>(); ql) {
          return qbf(
            ql->node_type(), ql->variables(), prenex(ql->matrix() || pright)
          );
        }
        if(auto qr = pright.to<qbf>(); qr) {
          return qbf(
            qr->node_type(), qr->variables(), prenex(pleft || qr->matrix())
          );
        }

        return pleft || pright;
      },
      [&](implication, qbformula left, qbformula right) -> qbformula {
        auto pleft = prenex(left);
        auto pright = prenex(right);

        if(auto ql = pleft.to<qbf>(); ql) {
          return qbf(
            dual(ql->node_type()), ql->variables(), 
            prenex(implies(ql->matrix(), pright))
          );
        }
        if(auto qr = pright.to<qbf>(); qr) {
          return qbf(
            qr->node_type(), qr->variables(), 
            prenex(implies(pleft, qr->matrix()))
          );
        }

        return implies(pleft, pright);
      },
      [](iff, auto left, auto right) {
        return prenex(implies(left, right) && implies(right, left));
      },
      [](qbf q, auto vars, auto matrix) {
        return qbf(q.node_type(), vars, prenex(matrix));
      }
    );

    //std::cout << to_string(result) << "\n";

    return result;
  }



  static prenex_qbf extract_prenex(qbformula f) 
  {
    using namespace logic::fragments::QBF;

    std::vector<qbf> blocks;
    qbformula matrix = f;

    while(matrix.is<qbf>()) {
      auto q = *matrix.to<qbf>();
      blocks.push_back(q);
      matrix = q.matrix();
    }

    auto m = matrix.to<bformula>();
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

  qdimacs clausify(qbformula f) {

    prenex_qbf qbformula = extract_prenex(f);

    black::cnf cnf = black::to_cnf(qbformula.matrix);

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
    for(auto block : qbformula.blocks) {
      
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

  std::string to_string(qdimacs qd) {
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

}
