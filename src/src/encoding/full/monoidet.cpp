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

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace synth {
  struct eps_t { 
    bool operator==(eps_t const&) const = default;
  };

  inline std::string to_string(eps_t) {
    return "|eps|";
  }

  struct tag_t {
    black::identifier name;
    size_t primed = 0;
    size_t step = 0;

    bool operator==(tag_t const&) const = default;
  };

  inline std::string to_string(tag_t t) {
    return to_string(t.name) + ":" + std::to_string(t.step) + ":" + 
           std::to_string(t.primed);
  }
}

template<>
struct std::hash<::synth::tag_t> {
  size_t operator()(::synth::tag_t t) const {
    return std::hash<::black::identifier>{}(t.name) + t.step + t.primed;
  }
};

template<>
struct std::hash<::synth::eps_t> {
  size_t operator()(::synth::eps_t) const {
    return 42;
  }
};

namespace synth {

  class var_manager_t {
  public:
    black::alphabet &sigma;
    sdd::manager *mgr;
    std::unordered_map<black::identifier, sdd::variable> vars;
    std::unordered_map<sdd::variable, black::identifier> props;
    unsigned next_var = 1;

    var_manager_t(black::alphabet &_sigma, sdd::manager *_mgr) 
      : sigma{_sigma}, mgr{_mgr} { }

    sdd::variable variable(black::identifier name) 
    {
      if(vars.contains(name))
        return vars[name];

      sdd::variable var = new_var();

      vars.insert({name, var});
      props.insert({var, name});

      return var;
    }

    sdd::variable variable(black::proposition prop) {
      return variable(prop.name());
    }

    black::identifier name(sdd::variable var) const {
      return props.at(var);
    }
  
  private:
    sdd::variable new_var() {
      while(next_var > mgr->var_count())
        mgr->add_var_after_last();
      return next_var++;
    }
  };

  struct automaton_t {
    std::unordered_set<sdd::variable> letters;
    std::unordered_set<sdd::variable> variables;

    sdd::node init;
    sdd::node trans;
    sdd::node objective;
  };

  struct monoidet_t 
  {
    monoidet_t(automaton _aut)
      : sigma{*_aut.init.sigma()}, 
        mgr{_aut.inputs.size() + _aut.outputs.size() + _aut.variables.size()}, 
        vars{sigma, &mgr},
        aut{abstract(_aut)} { }

    sdd::node to_sdd(synth::qbformula f);

    using node_cache_t = std::unordered_map<sdd::node, synth::bformula>;
    synth::bformula to_formula(sdd::node n, node_cache_t &cache);
    synth::bformula to_formula(sdd::node n);

    automaton_t abstract(automaton aut);

    sdd::variable untag(sdd::variable);
    sdd::variable primed(sdd::variable);
    sdd::variable step(sdd::variable, size_t n);
    std::vector<sdd::variable> untag(std::vector<sdd::variable>);
    std::vector<sdd::variable> primed(std::vector<sdd::variable>);
    std::vector<sdd::variable> step(std::vector<sdd::variable>, size_t);
    sdd::node untag(sdd::node);
    sdd::node primed(sdd::node);
    sdd::node step(sdd::node, size_t n);

    sdd::node T_eps(sdd::node trans);
    sdd::node T_step(sdd::node last, sdd::node t_eps, size_t n);
    
    [[noreturn]] void determinize();


    black::alphabet &sigma;
    sdd::manager mgr;
    var_manager_t vars;
    automaton_t aut;
  };

  sdd::node monoidet_t::to_sdd(synth::qbformula f) {
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

  synth::bformula monoidet_t::to_formula(sdd::node n, node_cache_t &cache) {
    if(cache.contains(n))
      return cache.at(n);
    
    auto result = [&]() -> synth::bformula {
      if(n.is_valid())
        return sigma.top();
      if(n.is_unsat())
        return sigma.bottom();
      
      if(n.is_literal()) {
        sdd::literal lit = n.literal();
        auto prop = sigma.proposition(vars.name(lit.variable()));
        if(lit)
          return prop;
        return !prop;
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

  automaton_t monoidet_t::abstract(automaton orig) {
    std::unordered_set<sdd::variable> letters;
    std::unordered_set<sdd::variable> variables;

    for(auto set : {orig.inputs, orig.outputs})
      for(auto prop : set)
        letters.insert(vars.variable(prop));

    for(auto prop : orig.variables)
      variables.insert(vars.variable(prop));

    return automaton_t {
      .letters = letters,
      .variables = variables,
      .init = to_sdd(orig.init),
      .trans = to_sdd(orig.trans),
      .objective = to_sdd(orig.objective)
    };
  }

  sdd::variable monoidet_t::untag(sdd::variable var) {
    auto name = vars.name(var);
    if(auto tag = name.to<tag_t>())
      return vars.variable(tag->name);
    
    return vars.variable(name);
  }

  sdd::variable monoidet_t::primed(sdd::variable var) {
    auto name = vars.name(var);
    if(auto tag = name.to<tag_t>()) {
      tag->primed++;
      return vars.variable(*tag);
    }
    return vars.variable(tag_t{name, false, 0});
  }

  sdd::variable monoidet_t::step(sdd::variable var, size_t n) {
    auto name = vars.name(var);
    if(auto tag = name.to<tag_t>()) {
      tag->step = n;
      return vars.variable(*tag);
    }
    return vars.variable(tag_t{name, false, n});
  }

  std::vector<sdd::variable> 
  monoidet_t::untag(std::vector<sdd::variable> v) {
    std::transform(begin(v), end(v), begin(v), [&](sdd::variable var) {
      return untag(var);
    });
    return v;
  }

  std::vector<sdd::variable> 
  monoidet_t::primed(std::vector<sdd::variable> v) {
    std::transform(begin(v), end(v), begin(v), [&](sdd::variable var) {
      return primed(var);
    });
    return v;
  }

  std::vector<sdd::variable> 
  monoidet_t::step(std::vector<sdd::variable> v, size_t n) {
    std::transform(begin(v), end(v), begin(v), [&](sdd::variable var){
      return step(var, n);
    });
    return v;
  }

  sdd::node monoidet_t::untag(sdd::node node) {
    return node.rename([&](sdd::variable var) {
      return untag(var);
    });
  }

  sdd::node monoidet_t::primed(sdd::node node) {
    return node.rename([&](sdd::variable var) {
      return primed(var);
    });
  }

  sdd::node monoidet_t::step(sdd::node node, size_t n) {
    return node.rename([&](sdd::variable var) {
      return step(var, n);
    });
  }

  synth::bformula monoidet_t::to_formula(sdd::node n) {
    node_cache_t cache;
    return to_formula(n, cache);
  }

  sdd::node monoidet_t::T_eps(sdd::node trans) {
    sdd::variable eps = vars.variable("eps");

    sdd::node loop = mgr.top();
    for(auto var : aut.variables)
      loop = loop && iff(mgr.literal(var), primed(var));
    
    return implies(eps, loop) && implies(!eps, trans);
  }

  sdd::node monoidet_t::T_step(sdd::node last, sdd::node t_eps, size_t n) {
    if(n == 0) {
      return t_eps.rename([&](sdd::variable var) {
        if(aut.letters.contains(var))
          return step(var, 0);
        return var;
      });
    }

    std::vector<sdd::variable> temps;
    std::unordered_map<sdd::variable, sdd::variable> srcmap;
    std::unordered_map<sdd::variable, sdd::variable> destmap;
    std::unordered_map<sdd::variable, sdd::variable> lettermap;
    
    for(auto var : aut.variables) {
      sdd::variable temp = primed(primed(var));
      temps.push_back(temp);
      srcmap.insert({var, temp});
      destmap.insert({primed(var), temp});
    }

    for(auto var : aut.letters)
      lettermap.insert({var, step(var, n)});
    
    return 
      exists(temps, 
        last.rename(destmap) && t_eps.rename(srcmap).rename(lettermap)
      );
  }

  void monoidet_t::determinize() {
    
    sdd::node t_eps = T_eps(aut.trans);
    sdd::node t_step = mgr.top();

    size_t n = 0;
    while(true) {
      std::cerr << "n: " << n << "\n";
      t_step = T_step(t_step, t_eps, n);
    }
  }

  automaton monoidet(automaton aut) {
    monoidet_t{aut}.determinize();
  }

}