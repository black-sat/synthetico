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

#include "synthetico/synthetico.hpp"

#include <iostream>
#include <sstream>
#include <string>

enum class algorithm {
  classic,
  novel,
  bdd
};

static char *argv0 = nullptr;

[[noreturn]]
static void error(std::string err) {
  std::cerr << argv0 << ": error: " + err + "\n";
  std::cerr << argv0 << ": usage: " << argv0;
  std::cerr << " <classic|novel|bdd> <formula> [input 1] [input 2] ..."
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
  else if(algos == "bdd"s)
      return algorithm::bdd;
  else
    error("unknown algorithm");
}

static std::string to_string(algorithm algo) {
  switch(algo) {
    case algorithm::classic:
      return "classic";
    case algorithm::novel:
      return "novel";
    case algorithm::bdd:
      return "bdd";
  }
  black_unreachable();
}

static int solve(synth::spec spec, algorithm algo) {
  black::tribool result = black::tribool::undef;

  try {
    std::cerr << "Solving spec '" << to_string(to_formula(spec)) << "' "
              << "with the '" << to_string(algo) << "' algorithm...\n";
    switch(algo){ 
      case algorithm::classic:
        result = is_realizable_classic(spec);
        break;
      case algorithm::novel:
        result = is_realizable_novel(spec);
        break;
      case algorithm::bdd:
        result = is_realizable_bdd(spec);
        break;
    }
  } catch(std::exception const& ex) {
    std::cerr << argv0 << ": uncaught exception: " << ex.what() << "\n";
  }

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

static std::string to_cmdline(synth::spec spec) {
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
    synth::spec spec = synth::random_spec(sigma, gen, *nvars, *size);
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

int main(int argc, char **argv) {
  using namespace std::literals;

  argv0 = argv[0]; 

  if(argc >= 2 && argv[1] == "random"s)
    return random(argc - 1, argv + 1);
  
  return formula(argc, argv);
}
