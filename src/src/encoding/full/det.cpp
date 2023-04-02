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

#include <black/solver/solver.hpp>
#include <black/sat/solver.hpp>
#include <black/logic/prettyprint.hpp>
#include <black/logic/cnf.hpp>

#include <unordered_map>
#include <iostream>

namespace synth {

  struct starred_t {
    proposition prop;

    bool operator==(starred_t const&) const = default;
  };

  inline std::string to_string(starred_t s) {
    return to_string(s.prop) + "*";
  }

}

template<>
struct std::hash<::synth::starred_t> {
  size_t operator()(::synth::starred_t s) {
    return std::hash<::black::proposition>{}(s.prop);
  }
};

namespace synth {

  using nu_t = std::unordered_map<proposition, bool>;

  static nu_t operator&&(nu_t nu1, nu_t const& nu2) {
    nu1.insert(begin(nu2), end(nu2));
    return nu1;
  }

  struct det_t {
    
    det_t(automata _aut) 
      : aut{std::move(_aut)}, sigma{*aut.init.sigma()} { }

    bool is_valid(qbformula f);
    bool is_sat(qbformula f);

    bformula expand(qbformula f, nu_t const& nu = nu_t{});

    qbformula apply(qbformula, nu_t const& nu);

    nu_t to_nu(black::sat::solver const&sat);
  
    bformula iota(nu_t const& nu);
    
    proposition fresh();

    nu_t source(nu_t const& nu);
    nu_t letter(nu_t const& nu);
    nu_t dest(nu_t const& nu);
    nu_t stardest(nu_t const& nu);

    bformula to_formula(nu_t const& nu);

    qbformula detformula();
    std::optional<nu_t> has_nondet_edges();
    
    nu_t fresh_nu(proposition w);

    bformula phi_r(nu_t const& nu, proposition w);
    bformula phi_b(nu_t const& nu, proposition w);
    bformula phi_s(nu_t const& nu, proposition w);
    qbformula T_s(nu_t const& nu, proposition w);
    bformula phi_v(qbformula t_s, nu_t const& nu, proposition w);
    qbformula phi_c(nu_t const& nu, proposition w);
    bformula phi_0(proposition w);
    
    qbformula T_f(nu_t const& nu, proposition w);

    void fix(nu_t const& nu, proposition w);

    void init();

    automata determinize();

    automata aut;
    std::vector<proposition> vars0;
    black::alphabet &sigma;

    std::unordered_map<proposition, bformula> iotas;
    std::vector<proposition> freshes;
    std::vector<nu_t> removed_edges;
    fresh_gen_t _fresh;
  };

  bool det_t::is_sat(qbformula f) {
    black::scope xi{sigma};
    
    auto z3 = black::sat::solver::get_solver("z3", xi);
    
    z3->assert_formula(f);
    if(z3->is_sat() == true)
      return true;
    
    return false;
  }
  
  bool det_t::is_valid(qbformula f) {
    return !is_sat(!f);
  }

  bformula det_t::expand(qbformula f, nu_t const& nu) {
    using namespace logic::fragments::QBF;

    auto result = f.match(
      [](boolean b) { return b; },
      [&](proposition p) -> bformula {
        if(nu.contains(p))
          return sigma.boolean(nu.at(p));
        return p;
      },
      [&](unary u, auto arg) {
        return logic::unary<Bool>(u.node_type(), expand(arg, nu));
      },
      [&](binary b, auto left, auto right) { 
        return logic::binary<Bool>(
          b.node_type(), expand(left, nu), expand(right, nu)
        );
      },
      [&](qbf q, auto vars, auto matrix) -> bformula {
        if(vars.empty())
          return expand(matrix, nu);
        
        proposition var = vars.back();
        vars.pop_back();

        qbformula newqbf = 
          vars.empty() ? matrix : qbf(q.node_type(), vars, matrix);

        nu_t nutrue = nu;
        nu_t nufalse = nu;
        nutrue[var] = true;
        nufalse[var] = false;

        return q.node_type().match(
          [&](qbf::type::thereis) {
            return expand(newqbf, nutrue) || expand(newqbf, nufalse);
          },
          [&](qbf::type::foreach) {
            return expand(newqbf, nutrue) && expand(newqbf, nufalse);
          }
        );
      }
    );

    return black::remove_booleans_shallow(result);
  }

  qbformula det_t::apply(qbformula f, nu_t const& nu) {
    using namespace logic::fragments::QBF;

    return f.match(
      [](boolean b) { return b; },
      [&](proposition p) -> bformula {
        if(nu.contains(p))
          return sigma.boolean(nu.at(p));
        return p;
      },
      [&](unary u, auto arg) {
        return unary(u.node_type(), apply(arg, nu));
      },
      [&](binary b, auto left, auto right) {
        return 
          binary(b.node_type(), apply(left, nu), apply(right, nu));
      },
      [&](qbf q, auto vars, auto matrix) {
        nu_t newnu = nu;
        for(auto v : vars)
          newnu.erase(v);
        
        return qbf(q.node_type(), vars, apply(matrix, newnu));
      }
    );
  }

  static proposition star(proposition prop) {
    return prop.sigma()->proposition(starred_t{prop});
  }

  [[maybe_unused]]
  static std::vector<proposition> star(std::vector<proposition> props) {
    return rename(props, [](auto p) { return star(p); });
  }

  static qbformula star(qbformula f) {
    return rename(f, [&](auto p) { return star(p); });
  }
  
  static bformula star(bformula f) {
    return *star(qbformula{f}).to<bformula>();
  }

  static proposition untag(proposition prop) {
    black::alphabet &sigma = *prop.sigma();
    
    if(auto inner = prop.name().to<primed_t>(); inner) {
      return untag(sigma.proposition(inner->label));
    }
    if(auto inner = prop.name().to<starred_t>(); inner) {
      return untag(inner->prop);
    }

    return prop;
  }

  static nu_t rename(nu_t nu, renaming_t renaming) {
    nu_t nu2;
    for(auto [key, value] : nu) {
      nu2[renaming(key)] = value;
    }

    return nu2;
  }

  [[maybe_unused]]
  static nu_t untag(nu_t const& nu) {
    return rename(nu, [](auto p) { return untag(p); });
  }

  [[maybe_unused]]
  static nu_t primed(nu_t const& nu) {
    return rename(nu, [](auto p) { return primed(p); });
  }

  [[maybe_unused]]
  static nu_t star(nu_t const&nu) {
    return rename(nu, [](auto p) { return star(p); });
  }

  [[maybe_unused]]
  static nu_t filter(nu_t const&nu, std::vector<proposition> const& props) {
    nu_t nu2;
    for(auto prop : props) 
      if(nu.contains(prop))
        nu2[prop] = nu.at(prop);

    return nu2;
  }

  nu_t det_t::to_nu(black::sat::solver const&sat) {
    nu_t nu;
    for(auto const& set : {
      aut.inputs, aut.outputs, aut.variables, 
      primed(aut.variables), star(aut.variables)
    }) {
      for(auto prop : set) {
        if(sat.value(prop) == true)
          nu[prop] = true;
        if(sat.value(prop) == false)
          nu[prop] = false;
      }
    }

    return nu;
  }

  nu_t det_t::source(nu_t const&nu) {
    return filter(nu, aut.variables);
  }
  
  nu_t det_t::letter(nu_t const&nu) {
    std::vector<proposition> letters;
    letters.insert(begin(letters), begin(aut.inputs), end(aut.inputs));
    letters.insert(begin(letters), begin(aut.outputs), end(aut.outputs));

    return filter(nu, letters);
  }

  nu_t det_t::dest(nu_t const&nu) {
    return filter(nu, primed(aut.variables));
  }

  nu_t det_t::stardest(nu_t const&nu) {
    return filter(nu, star(aut.variables));
  }

  bformula det_t::to_formula(nu_t const&nu) {
    std::vector<bformula> lits;
    for(auto [key, value] : nu) {
      if(value)
        lits.push_back(key);
      else
        lits.push_back(!key);
    }

    return big_and(sigma, lits);
  }

  qbformula det_t::detformula() {
    return aut.trans && rename(aut.trans, [](proposition p) {
      if(is_primed(p))
        return star(untag(p));
      return p;
    });
  }

  std::optional<nu_t> det_t::has_nondet_edges() {
    black::scope xi{sigma};
    
    auto z3 = black::sat::solver::get_solver("z3", xi);
    z3->assert_formula(detformula());

    for(auto var : aut.variables) {
      if(z3->is_sat_with(primed(var) && !star(var)))
        return to_nu(*z3);
    }

    return {};
  }

  proposition det_t::fresh() {
    proposition w = _fresh(sigma.proposition("fresh"));
    freshes.push_back(w);
    
    return w;
  }

  bformula det_t::iota(nu_t const&nu) {
    auto _iota = [&](nu_t part) {
      for(auto w : freshes) {
        if(part.contains(w) && part[w] == true)
          return iotas.at(w);
      }
      return to_formula(part);
    };

    nu_t dest1 = filter(untag(dest(nu)), vars0);
    nu_t dest2 = filter(untag(stardest(nu)), vars0);

    return _iota(dest1) || _iota(dest2);
  }

  nu_t det_t::fresh_nu(proposition w) {
    nu_t nu;
    for(auto prop : aut.variables)
      if(prop != w)
        nu.insert({prop, false});
    
    nu.insert({w, true});
    return nu;
  }

  bformula det_t::phi_r(nu_t const&nu, proposition w) {
    return 
    !w && !primed(w) &&
    !(
      to_formula(source(nu)) && to_formula(letter(nu)) && to_formula(dest(nu))
    ) && !(
      to_formula(source(nu)) && to_formula(letter(nu)) &&
      to_formula(primed(untag(stardest(nu))))
    );
  }

  bformula det_t::phi_b(nu_t const&nu, proposition w) {
    return to_formula(source(nu)) && to_formula(letter(nu)) &&
           primed(to_formula(fresh_nu(w)));
  }

  bformula det_t::phi_s(nu_t const&nu, proposition w) {
    auto loop = implies(to_formula(untag(dest(nu))), to_formula(source(nu)));
    auto starloop = 
      implies(to_formula(untag(stardest(nu))), to_formula(source(nu)));

    if(is_valid(loop) || is_valid(starloop)) {
      std::cerr << " - self loop: true\n";
      return w && to_formula(letter(nu)) && primed(w);
    }
    std::cerr << " - self loop: false\n";

    return sigma.bottom();
  }

  qbformula det_t::T_s(nu_t const&nu, proposition w) {
    return 
      ((aut.trans && phi_r(nu, w)) || phi_b(nu, w) || phi_s(nu, w) || 
      phi_c(nu, w)) && phi_0(w);
  }

  bformula det_t::phi_v(qbformula t_s, nu_t const&nu, proposition w) {
    std::vector<proposition> implicants;

    for(auto w_j : freshes) {
      if(w_j != w && is_valid(implies(iotas.at(w_j), iotas.at(w))))
        implicants.push_back(primed(w_j));
    }

    bformula result = !(
      w && to_formula(letter(nu)) && 
      (primed(iotas.at(w)) || big_or(sigma, implicants))
    );
    
    nu_t fw = fresh_nu(w);
    qbformula loop = apply(t_s, fw && letter(nu) && primed(fw));
    
    if(!is_valid(loop))
      result = sigma.top();

    return result;
  }

  qbformula det_t::phi_c(nu_t const&nu, proposition w) {
    using namespace logic::fragments::QBF;
    bformula base = w && !primed(w);

    qbformula startrans = rename(aut.trans, [&](proposition p) {
      auto it = std::find(begin(aut.variables), end(aut.variables), p);
      if(it != end(aut.variables))
        return star(p);
      return p;
    });

    qbformula core = 
      thereis(star(aut.variables), 
        (to_formula(star(untag(dest(nu)))) || 
         to_formula(star(untag(stardest(nu))))) && startrans
      );
    
    return base && core;
  }

  bformula det_t::phi_0(proposition w) {
    std::vector<bformula> lits;
    for(auto x : aut.variables)
      if(x != w)
        lits.push_back(!x);
    
    return implies(w, big_and(sigma, lits)) && 
           implies(primed(w), primed(big_and(sigma, lits)));
  }

  qbformula det_t::T_f(nu_t const& nu, proposition w) {
    qbformula t_s = T_s(nu, w);
    return (t_s && phi_v(t_s, nu, w)) && phi_0(w);
  }

  void det_t::fix(nu_t const& nu, proposition w) {
    // state variables
    std::vector<proposition> vars = aut.variables;
    if(std::find(begin(vars), end(vars), w) == end(vars))
      vars.push_back(w);

    // initial state
    bformula init = aut.init && !w;

    // transition relation
    qbformula trans = T_f(nu, w);

    // final state
    bformula were_final = 
      (aut.objective && to_formula(untag(dest(nu)))) ||
      (aut.objective && to_formula(untag(stardest(nu))));

    bformula objective = aut.objective;
    if(this->is_sat(were_final))
      objective = objective || w;
    else
      objective = objective && !w;

    aut.variables = vars;
    aut.init = init;
    aut.trans = expand(trans);
    aut.objective = objective;
  }

  void det_t::init() {
    using namespace logic::fragments::QBF;
    
    proposition x_I = _fresh(sigma.proposition("init_fresh"));

    std::vector<proposition> vars = aut.variables;
    vars.push_back(x_I);

    bformula init = x_I && big_and(sigma, aut.variables, [](auto p) {
      return !p;
    });

    qbformula startrans = rename(aut.trans, [&](proposition p) {
      auto it = std::find(begin(aut.variables), end(aut.variables), p);
      if(it != end(aut.variables))
        return star(p);
      return p;
    });

    qbformula trans = 
      !primed(x_I) &&
      implies(!x_I, aut.trans) &&
      implies(x_I, thereis(star(aut.variables), star(aut.init) && startrans));

    bformula objective = aut.objective && !x_I;

    aut.variables = vars;
    aut.init = init;
    aut.trans = trans;
    aut.objective = objective;
  }

  automata det_t::determinize() 
  {
    using namespace logic::fragments::QBF;

    //init();
    vars0 = aut.variables;

    std::cerr << aut << "\n";

    while(true) {
      auto nu = has_nondet_edges();
    
      if(!nu) {
        // std::cerr << "determinized: \n";
        // std::cerr << aut << "\n";
        // aut.trans = expand(aut.trans);
        return aut;
      }

      std::cerr << "found nondet edges: " << to_string(to_formula(*nu)) << "\n";
      // black_assert(
      //   std::find(begin(removed_edges), end(removed_edges), *nu) == 
      //     end(removed_edges)
      // );
      
      std::optional<proposition> w;
      for(auto w_j : freshes) {
        std::cerr << "- iota check\n";
        std::cerr << "  - iota(nu): " << to_string(iota(*nu)) << "\n";
        std::cerr << "  - iota(" << to_string(w_j) << "): "
                  << to_string(iotas.at(w_j)) << "\n";
        if(is_valid(iff(iotas.at(w_j), iota(*nu)))) {
          //std::cerr << "  - found w: " << to_string(w_j) << "\n";
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

      removed_edges.push_back(*nu);

      std::cerr << aut << "\n";
    }
  }  

  automata determinize(automata aut) {
    return det_t{aut}.determinize();
  }

}