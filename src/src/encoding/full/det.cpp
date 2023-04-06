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

#include <synthetico/common.hpp>
#include <synthetico/encoding/full/det.hpp>

#include <sdd++/sdd++.hpp>

#include <iostream>
#include <algorithm>
#include <cassert>

namespace synth {

  class var_manager_t {
  public:
    std::unordered_map<logic::proposition, sdd::variable> vars;
    std::unordered_map<sdd::variable, logic::proposition> props;
    black::alphabet &sigma;
    sdd::manager *mgr;
    unsigned next_var = 1;

    var_manager_t(black::alphabet &_sigma, sdd::manager *_mgr) 
      : sigma{_sigma}, mgr{_mgr} { }

    sdd::variable variable(logic::proposition prop) {
      if(vars.contains(prop))
        return vars.at(prop);
      
      sdd::variable var = new_var();
      vars.insert({prop, var});
      props.insert({var, prop});

      return var;
    }

    logic::proposition proposition(sdd::variable var) const {
      return props.at(var);
    }

  private:
    sdd::variable new_var() {
      while(next_var > mgr->var_count())
        mgr->add_var_after_last();
      return next_var++;
    }
  };

  struct det_t {
    det_t(automata _aut) 
      : aut{std::move(_aut)}, sigma{*aut.init.sigma()}, 
        mgr{init_mgr()}, vars{init_vars()}, 
        variables{aut.variables},
        init{to_sdd(aut.init)}, trans{to_sdd(aut.trans)},
        objective{to_sdd(aut.objective)} { }

    automata determinize();

    sdd::manager init_mgr();
    var_manager_t init_vars();

    sdd::variable untag(sdd::variable);
    sdd::variable primed(sdd::variable);
    sdd::variable star(sdd::variable);
    sdd::literal untag(sdd::literal);
    sdd::literal primed(sdd::literal);
    sdd::literal star(sdd::literal);
    sdd::node untag(sdd::node);
    sdd::node primed(sdd::node);
    sdd::node star(sdd::node);
    
    bool is_primed(sdd::variable);
    bool is_star(sdd::variable);
    
    sdd::variable fresh();

    sdd::node to_sdd(synth::qbformula f);

    using node_cache_t = std::unordered_map<sdd::node, synth::bformula>;
    synth::bformula to_formula(sdd::node n, node_cache_t &cache);
    synth::bformula to_formula(sdd::node n);

    sdd::node filter(sdd::node, std::vector<proposition> const&);
    sdd::node source(sdd::node);
    sdd::node letter(sdd::node);
    sdd::node dest1(sdd::node);
    sdd::node dest2(sdd::node);

    sdd::node q_w(sdd::variable w);

    std::optional<sdd::node> has_nondet_edges();
    sdd::node iota(sdd::node nu);

    sdd::node phi_r(sdd::node nu, sdd::variable w);
    sdd::node phi_b(sdd::node nu, sdd::variable w);
    sdd::node phi_s(sdd::node nu, sdd::variable w);
    sdd::node T_s(sdd::node nu, sdd::variable w);
    sdd::node phi_f(sdd::node t_s, sdd::node nu, sdd::variable w);
    sdd::node phi_c(sdd::node nu, sdd::variable w);
    sdd::node phi_0(sdd::variable w);
    
    sdd::node T_f(sdd::node nu, sdd::variable w);

    void fix(sdd::node nu, sdd::variable w);

    automata aut;
    black::alphabet &sigma;
    sdd::manager mgr;
    var_manager_t vars;
    fresh_gen_t fresh_gen;
    std::vector<sdd::variable> freshes;
    std::unordered_map<sdd::variable, sdd::node> iotas;
    
    std::vector<proposition> variables;
    sdd::node init;
    sdd::node trans;
    sdd::node objective;
  };

  sdd::manager det_t::init_mgr() {
    return sdd::manager{
      3 * aut.variables.size() + // normal, primed, and starred state variables 
      aut.inputs.size() +
      aut.outputs.size()
    };
  }

  var_manager_t det_t::init_vars() {
    var_manager_t result{sigma, &mgr};

    for(auto var : aut.variables) {
      result.variable(var);
      result.variable(synth::primed(var));
      result.variable(synth::star(var));
    }

    for(auto set : {aut.inputs, aut.outputs})
      for(auto var : set)
        result.variable(var);
    
    return result;
  }

  sdd::variable det_t::untag(sdd::variable var) {
    return vars.variable(synth::untag(vars.proposition(var)));
  }

  sdd::variable det_t::primed(sdd::variable var) {
    return vars.variable(synth::primed(synth::untag(vars.proposition(var))));
  }

  sdd::variable det_t::star(sdd::variable var) {
    return vars.variable(synth::star(synth::untag(vars.proposition(var))));
  }

  sdd::literal det_t::untag(sdd::literal lit) {
    if(lit)
      return untag(lit.variable());
    return !untag(lit.variable());
  }

  sdd::literal det_t::primed(sdd::literal lit) {
    if(lit)
      return primed(lit.variable());
    return !primed(lit.variable());
  }

  sdd::literal det_t::star(sdd::literal lit) {
    if(lit)
      return star(lit.variable());
    return !star(lit.variable());
  }

  sdd::node det_t::untag(sdd::node nu) {
    return nu.rename([&](sdd::variable var) {
      return untag(var);
    });
  }

  sdd::node det_t::primed(sdd::node nu) {
    return nu.rename([&](sdd::variable var) {
      return primed(var);
    });
  }

  sdd::node det_t::star(sdd::node nu) {
    return nu.rename([&](sdd::variable var) {
      return star(var);
    });
  }

  bool det_t::is_primed(sdd::variable var) {
    return vars.proposition(var).name().is<primed_t>();
  }

  bool det_t::is_star(sdd::variable var) {
    return vars.proposition(var).name().is<starred_t>();
  }

  sdd::variable det_t::fresh() {
    proposition prop = fresh_gen(sigma.proposition("fresh"));
    
    sdd::variable var = vars.variable(prop);
    freshes.push_back(var);
    vars.variable(synth::primed(prop));
    vars.variable(synth::star(prop));

    return var;
  }

  sdd::node det_t::to_sdd(synth::qbformula f) {
    using namespace black::logic::fragments::QBF;

    return f.match(
      [&](boolean, bool value) {
        return value ? mgr.top() : mgr.bottom();
      },
      [&](proposition p) {
        return mgr.literal(vars.variable(p));
      },
      [&](negation, auto arg) {
        return !to_sdd(arg);
      },
      [&](conjunction c) {
        sdd::node result = mgr.top();
        for(auto op : c.operands())
          result = result && to_sdd(op);
        return result;
      },
      [&](disjunction c) {
        sdd::node result = mgr.bottom();
        for(auto op : c.operands())
          result = result || to_sdd(op);
        return result;
      },
      [&](implication, auto left, auto right) {
        return implies(to_sdd(left), to_sdd(right));
      },
      [&](iff, auto left, auto right) {
        return sdd::iff(to_sdd(left), to_sdd(right));
      },
      [&](qbf q, auto qvars, auto matrix) {
        sdd::node sddmatrix = to_sdd(matrix);
        if(qvars.empty())
          return sddmatrix;
        
        sdd::variable var = vars.variable(qvars.back());
        qvars.pop_back();

        return q.node_type().match(
          [&](qbf::type::thereis) {
            return sdd::exists(var, sddmatrix);
          },
          [&](qbf::type::foreach) {
            return sdd::forall(var, sddmatrix);
          }
        );     
      }
    );
  }

  synth::bformula det_t::to_formula(sdd::node n, node_cache_t &cache) {
    if(cache.contains(n))
      return cache.at(n);
    
    auto result = [&]() -> synth::bformula {
      if(n.is_valid())
        return sigma.top();
      if(n.is_unsat())
        return sigma.bottom();
      
      if(n.is_literal()) {
        sdd::literal lit = n.literal();
        if(lit)
          return vars.proposition(lit.variable());
        return !vars.proposition(lit.variable());
      }

      if(n.is_decision()) {
        auto elements = n.elements();
        return big_or(sigma, elements, [&](auto elem) {
          return to_formula(elem.prime, cache) &&
                 to_formula(elem.sub, cache);
        });
      }
      throw "unreachable";
    }();
    
    cache.insert({n, result});
    return result;
  }

  synth::bformula det_t::to_formula(sdd::node n) {
    node_cache_t cache;
    return to_formula(n, cache);
  }

  std::optional<sdd::node> det_t::has_nondet_edges() {
    sdd::node base = trans && trans.rename([&](sdd::variable var) {
      if(is_primed(var))
        return star(untag(var));
      return var;
    });

    for(auto prop : variables) {
      sdd::variable var = vars.variable(prop);
      sdd::node test = base && primed(var) && !star(var);
      if(test.is_sat()) {
        sdd::node nu = mgr.top();
        auto model = *test.model();
        for(auto lit : model)
          if(unsigned(lit.variable()) >= 3)
            nu = nu && lit;
        return nu;
      }
    }

    return {};
  }

  sdd::node det_t::filter(
    sdd::node node, std::vector<proposition> const& props
  ) {
    sdd::node result = node.manager()->top();
    for(auto prop : props) {
      sdd::variable var = vars.variable(prop);
      std::optional<bool> value = node.value(var);
      if(value.has_value())
        result = result && (*value ? var : !var);
    }
    return result;
  }

  sdd::node det_t::source(sdd::node nu) {
    return filter(nu, variables);
  }
  
  sdd::node det_t::letter(sdd::node nu) {
    std::vector<proposition> letters;
    letters.insert(begin(letters), begin(aut.inputs), end(aut.inputs));
    letters.insert(begin(letters), begin(aut.outputs), end(aut.outputs));

    return filter(nu, letters);
  }

  sdd::node det_t::dest1(sdd::node nu) {
    return filter(nu, synth::primed(aut.variables));
  }

  sdd::node det_t::dest2(sdd::node nu) {
    return filter(nu, synth::star(aut.variables));
  }

  sdd::node det_t::q_w(sdd::variable w) {
    sdd::node negx = mgr.top();
    for(auto x : variables) {
      sdd::variable var = vars.variable(x);
      if(var != w)
        negx = negx && !var;
    }

    return negx && w;
  }

  sdd::node det_t::iota(sdd::node nu) {
    auto _iota = [&](sdd::node part) {
      for(auto w : freshes) {
        if(part.value(w) == true)
          return iotas.at(w);
      }
      return part;
    };

    sdd::node part1 = filter(untag(dest1(nu)), aut.variables);
    sdd::node part2 = filter(untag(dest2(nu)), aut.variables);

    return _iota(part1) || _iota(part2);
  }

  sdd::node det_t::phi_r(sdd::node nu, sdd::variable w) {
    return 
      !(source(nu) && letter(nu) && dest1(nu)) &&
      !(source(nu) && letter(nu) && dest2(nu)) &&
      !w && !primed(w);
  }

  sdd::node det_t::phi_b(sdd::node nu, sdd::variable w) {
    return source(nu) && letter(nu) && primed(w);
  }

  sdd::node det_t::phi_s(sdd::node nu, sdd::variable w) {
    auto loop1 = implies(untag(dest1(nu)), source(nu));
    auto loop2 = implies(untag(dest2(nu)), source(nu));

    if(loop1.is_valid() || loop2.is_valid()) 
      return w && letter(nu) && primed(w);
    
    return nu.manager()->bottom();
  }

  sdd::node det_t::phi_c(sdd::node nu, sdd::variable w) {
    sdd::node base = nu.manager()->literal(w) && !primed(w);

    std::vector<sdd::variable> state;
    std::transform(begin(variables), end(variables), std::back_inserter(state),
      [&](proposition prop) {
        return vars.variable(prop);
      }
    );

    sdd::node core = 
      exists(state, (untag(dest1(nu)) || untag(dest2(nu))) && trans);
    
    return base && core;
  }

  sdd::node det_t::phi_0(sdd::variable w) {
    return implies(w, q_w(w)) && implies(primed(w), primed(q_w(w)));
  }

  sdd::node det_t::T_s(sdd::node nu, sdd::variable w) {
    return (
      (trans && phi_r(nu, w)) || 
      phi_b(nu, w) || phi_s(nu, w) || phi_c(nu, w)
    ) && phi_0(w);
  }

  sdd::node det_t::phi_f(sdd::node t_s, sdd::node nu, sdd::variable w) 
  {
    sdd::node loop = t_s && q_w(w) && letter(nu) && primed(q_w(w));
    
    if(!loop.is_valid())
      return mgr.top();

    sdd::node implicants = mgr.bottom();
    for(auto w_j : freshes) {
      if(w_j != w && implies(iotas.at(w_j), iotas.at(w)).is_valid())
        implicants = implicants || primed(w_j);
    }

    return !(w && letter(nu) && (primed(iotas.at(w)) || implicants));
  }
  
  sdd::node det_t::T_f(sdd::node nu, sdd::variable w) {
    sdd::node t_s = T_s(nu, w);
    return t_s && phi_f(t_s, nu, w);
  }

  void det_t::fix(sdd::node nu, sdd::variable w) {
    // state variables
    std::vector<proposition> newvars = variables;
    proposition wprop = vars.proposition(w);
    if(std::find(begin(newvars), end(newvars), wprop) == end(newvars))
      newvars.push_back(wprop);

    // initial state
    sdd::node newinit = init && !w;

    // transition relation
    sdd::node newtrans = T_f(nu, w);

    // final state
    sdd::node were_final = 
      (objective && untag(dest1(nu))) || (objective && untag(dest2(nu)));

    sdd::node newobjective = objective;
    if(were_final.is_sat())
      newobjective = newobjective || w;
    else
      newobjective = newobjective && !w;
    
    // update the actual automaton
    variables = newvars;
    init = newinit;
    trans = newtrans;
    objective = newobjective;
  }

  automata det_t::determinize() {

    //init();
    std::cerr << aut << "\n";

    while(true) {
      auto nu = has_nondet_edges();
    
      if(!nu) {
        return automata {
          .inputs = std::move(aut.inputs),
          .outputs = std::move(aut.outputs),
          .variables = std::move(variables),
          .init = to_formula(init),
          .trans = to_formula(trans),
          .objective = to_formula(objective)
        };
      }

      std::cerr << "found nondet edges\n";
      
      std::optional<sdd::variable> w;
      for(auto w_j : freshes) {
        if(iotas.at(w_j) == iota(*nu)) {
          w = w_j;
          break;
        }
      }

      if(w) {
        fix(*nu, *w);
      } else {
        w = fresh();
        iotas.insert({*w, iota(*nu)});
        fix(*nu, *w);
      }
    }    
  }

  automata determinize(automata aut) {
    return det_t{aut}.determinize();
  }
}