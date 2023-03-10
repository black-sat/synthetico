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

#include <synthetico/synthetico.hpp>

#include <iostream>
#include <string>

enum class algorithm {
  classic,
  novel
};

int main(int argc, char **argv) {

  using namespace std::literals;

  auto error = [&](std::string err) {
    std::cerr << argv[0] << ": error: " + err + "\n";
    std::cerr << argv[0] << ": usage: " << argv[0];
    std::cerr << " <classic|novel> <formula> [input 1] [input 2] ..."
                 " [input n]\n";
    exit(1);
  };

  if(argc < 3)
    error("insufficient command-line arguments");

  algorithm algo;

  if(argv[1] == "classic"s)
    algo = algorithm::classic;
  else if(argv[1] == "novel"s)
    algo = algorithm::novel;
  else
    error("unknown algorithm");

  black::alphabet sigma;

  synth::spec spec = *synth::parse(sigma, argc - 1, argv + 1, error);

  black::tribool result = black::tribool::undef;

  try {
    std::cerr << "Solving spec with '" << argv[1] << "' algorithm...\n";
    switch(algo){ 
      case algorithm::classic:
        result = is_realizable_classic(spec);
        break;
      case algorithm::novel:
        result = is_realizable_novel(spec);
        break;
    }
  } catch(std::exception const& ex) {
    std::cerr << argv[0] << ": uncaught exception: " << ex.what() << "\n";
  }

  if(result == true)
    std::cout << "REALIZABLE\n";
  else if(result == false)
    std::cout << "UNREALIZABLE\n";
  else 
    std::cout << "UNKNOWN\n";


  return 0;
}
