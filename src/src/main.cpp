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

#include <black/logic/logic.hpp>
#include <black/logic/parser.hpp>
#include <black/logic/prettyprint.hpp>
#include <black/sat/solver.hpp>

#include <synthetico/synthetico.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <sdd++/sdd++.hpp>
#include <sdd/sdd.h>

enum class algorithm {
  classic,
  novel
};

static char *argv0 = nullptr;

[[noreturn]]
static void error(std::string err) {
  std::cerr << argv0 << ": error: " + err + "\n";
  std::cerr << argv0 << ": usage: " << argv0;
  std::cerr << " <classic|novel> <formula> [input 1] [input 2] ..."
                " [input n]\n";
  std::cerr << argv0 << ": usage: " << argv0;
  std::cerr << " <random> <n formulas> <n vars> <size> <seed>\n";

  exit(1);
}

static algorithm to_algo(std::string algos) {
  using namespace std::literals;

  if(algos == "classic"s)
    return algorithm::classic;
  else if(algos == "novel"s)
    return algorithm::novel;
  else
    error("unknown algorithm");
}

static std::string to_string(algorithm algo) {
  switch(algo) {
    case algorithm::classic:
      return "classic";
    case algorithm::novel:
      return "novel";
  }
  black_unreachable();
}

static int solve(synth::spec spec, algorithm algo) {
  black::tribool result = black::tribool::undef;

  //try {
    std::cerr << "Solving spec '" << to_string(spec.formula) << "' "
              << "with the '" << to_string(algo) << "' algorithm...\n";
    switch(algo){ 
      case algorithm::classic:
        result = is_realizable_classic(spec);
        break;
      case algorithm::novel:
        result = is_realizable_novel(spec);
        break;
    }
  // } catch(std::exception const& ex) {
  //   std::cerr << argv0 << ": uncaught exception: " << ex.what() << "\n";  
  // }

  if(result == true)
    std::cout << "REALIZABLE\n";
  else if(result == false)
    std::cout << "UNREALIZABLE\n";
  else 
    std::cout << "UNKNOWN\n";


  return 0;
}

template<typename T>
static std::optional<T> from_string(std::string s) {
  T v;
  
  std::stringstream str(s);
  
  str >> v;
  if(!str)
    return {};

  return v;
}

static std::string to_cmdline(synth::purepast_spec spec) {
   std::string cmdline = "'" + to_string(to_formula(spec)) + "'";

  for(auto in : spec.inputs) 
    cmdline += " " + to_string(in);

  return cmdline;
}

static int random(int argc, char **argv){
  if(argc < 5)
    error("insufficient command-line arguments");

  auto nformulas = from_string<size_t>(argv[1]); 
  if(!nformulas)
    error("invalid number of formulas");
  
  auto nvars = from_string<size_t>(argv[2]); 
  if(!nvars)
    error("invalid number of variables");

  auto size = from_string<size_t>(argv[3]); 
  if(!size)
    error("invalid formula size");

  auto seed = from_string<uint32_t>(argv[4]); 
  if(!seed)
    error("invalid seed");

  std::mt19937 gen(*seed);
  black::alphabet sigma;

  for(size_t i = 0; i < *nformulas; i++) {
    synth::purepast_spec spec = synth::random_spec(sigma, gen, *nvars, *size);
    std::cout << to_cmdline(spec) << "\n";
  }

  return 0;
}

static int formula(int argc, char **argv) {
  if(argc < 3)
    error("insufficient command-line arguments");

  algorithm algo = to_algo(argv[1]);
  
  black::alphabet sigma;

  synth::spec spec = *synth::parse(sigma, argc - 1, argv + 1, error);

  return solve(spec, algo);
}

using fsize_cache_t = std::unordered_map<synth::qbformula, size_t>;

static size_t fsize(synth::qbformula f, fsize_cache_t &cache) {
  using namespace black::logic::fragments::QBF;

  if(cache.contains(f))
    return cache.at(f);

  auto result = f.match(
    [](boolean) { return 1; },
    [](proposition) { return 1; },
    [&](unary, auto arg) {
      return 1 + fsize(arg, cache);
    },
    [&](binary, auto left, auto right) {
      return 1 + fsize(left, cache) + fsize(right, cache);
    },
    [&](qbf, auto, auto matrix) {
      return 1 + fsize(matrix, cache);
    }
  );

  cache.insert({f, result});
  return result;
}

static size_t fsize(synth::qbformula f) {
  fsize_cache_t cache;
  return fsize(f, cache);
}

struct sdd_vars_t {
  std::unordered_map<black::proposition, sdd::variable> vars;
  std::unordered_map<sdd::variable, black::proposition> props;
} ;

static sdd::variable prop_to_var(
  sdd::manager *mgr, sdd_vars_t &vars, black::proposition p
) {
  if(vars.vars.contains(p))
    return vars.vars[p];
  
  unsigned newvar = unsigned(mgr->var_count()) + 1;
  mgr->add_var_after_last();
  sdd::variable var = newvar;
  vars.vars.insert({p, var});
  vars.props.insert({var, p});

  return var;
}

static 
sdd::node to_sdd(sdd::manager *mgr, sdd_vars_t &vars, synth::qbformula f) {
  using namespace black::logic::fragments::QBF;

  return f.match(
    [&](boolean, bool value) {
      return value ? mgr->top() : mgr->bottom();
    },
    [&](proposition p) {
      return mgr->literal(prop_to_var(mgr, vars, p));
    },
    [&](negation, auto arg) {
      return !to_sdd(mgr, vars, arg);
    },
    [&](conjunction c) {
      sdd::node result = mgr->top();
      for(auto op : c.operands())
        result = result && to_sdd(mgr, vars, op);
      return result;
    },
    [&](disjunction c) {
      sdd::node result = mgr->bottom();
      for(auto op : c.operands())
        result = result || to_sdd(mgr, vars, op);
      return result;
    },
    [&](implication, auto left, auto right) {
      return to_sdd(mgr, vars, !left || right);
    },
    [&](iff, auto left, auto right) {
      return to_sdd(mgr, vars, implies(left, right) && implies(right, left));
    },
    [&](qbf q, auto qvars, auto matrix) {
      sdd::node sddmatrix = to_sdd(mgr, vars, matrix);
      if(qvars.empty())
        return sddmatrix;
      
      sdd::variable var = prop_to_var(mgr, vars, qvars.back());
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

using node_cache_t = std::unordered_map<sdd::node, synth::bformula>;

[[noreturn]] static void unreachable() {
  __builtin_unreachable();
}

static synth::bformula to_formula(
  black::alphabet &sigma, sdd::node n, node_cache_t &cache
) {
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
        return sigma.proposition(long(lit));
      return !sigma.proposition(long(lit));
    }

    if(n.is_decision()) {
      auto elements = n.elements();
      return big_or(sigma, elements, [&](auto elem) {
        return to_formula(sigma, elem.prime, cache) &&
               to_formula(sigma, elem.sub, cache);
      });
    }
    unreachable();
  }();
  
  cache.insert({n, result});
  return result;
}

static synth::bformula to_formula(black::alphabet &sigma, sdd::node n) {
  node_cache_t cache;
  return to_formula(sigma, n, cache);
}

static int test(int argc, char **argv) {
  using namespace black::logic::fragments::QBF;

  if(argc < 2)
    error("insufficient command-line arguments");

  alphabet sigma;
  scope xi{sigma};

  synth::spec spec = *synth::parse(sigma, argc, argv, error);

  synth::automata aut = encode(spec);

  sdd::manager mgr(1);

  auto test = thereis(aut.variables, 
    thereis(synth::primed(aut.variables), 
      aut.trans
    )
  );

  // auto test1 = mgr.literal(3) || mgr.literal(4);
  // auto test2 = !(!mgr.literal(3) && !mgr.literal(4));

  // if(test1 == test2)
  //   std::cerr << "test1 == test2\n";
  // else
  //   std::cerr << "test1 != test2\n";


  // auto parsed = black::parse_formula(sigma, argv[1], error);
  // auto casted = parsed->to<synth::bformula>();
  // if(!casted)
  //   error("Please give me only Boolean formulas");

  // auto test = *casted;

  //std::cerr << "test: " << to_string(test) << "\n";
  
  std::cerr << "computing SDD...\n";
  sdd_vars_t vars;
  sdd::node node = to_sdd(&mgr, vars, test);
  std::cerr << "done!\n";

  std::cerr << "SDD size: " << node.size() << "\n";
  std::cerr << "SDD count: " << node.count() << "\n";


  std::cerr << "reconstructing formula...\n";
  synth::bformula f = to_formula(sigma, node);
  std::cerr << "done!\n";

  std::cerr << "formula size: " << fsize(f) << "\n";
  std::cerr << "formula: " << to_string(f) << "\n";

  std::cerr << "solving formula...\n";
  auto solution = node.model();
  std::cerr << "done: " << (solution.has_value() ? "SAT\n" : "UNSAT\n");

  if(!solution)
    return 0;

  std::cerr << "solution:\n";
  for(auto lit : *solution) {
    sdd::variable var = lit.variable();
    black::proposition p = vars.props.at(var);
    if(lit)
      std::cerr << "- " << to_string(p) << "\n";
    else
      std::cerr << "- " << to_string(!p) << "\n";
  }

  return 0;
}

int main(int argc, char **argv) {
  using namespace std::literals;

  argv0 = argv[0]; 

  if(argc >= 2 && argv[1] == "random"s)
    return random(argc - 1, argv + 1);

  if(argc >= 2 && argv[1] == "test"s)
    return test(argc - 1, argv + 1);
  
  return formula(argc, argv);
}
