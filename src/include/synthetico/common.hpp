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

#ifndef SYNTH_GAME_COMMON_HPP
#define SYNTH_GAME_COMMON_HPP

#include <black/logic/prettyprint.hpp>

namespace synth {
  
  namespace logic = black::logic;

  using FG = logic::make_fragment_t<
    logic::syntax_list<
      logic::syntax_element::eventually,
      logic::syntax_element::always
    >
  >;

  using Bool = logic::propositional;
  using QBF = logic::QBF;

  template<typename T>
  using formula = logic::formula<T>;

  using bformula = formula<Bool>;
  using qbformula = formula<QBF>;

  using proposition = logic::proposition;
  using otherwise = logic::otherwise;

  using game_t = formula<FG>::type;

  using quantifier_t = logic::qbf<QBF>::type;

  using renaming_t = std::function<proposition(proposition)>;

  qbformula rename(qbformula f, renaming_t renaming);
  std::vector<proposition> rename(std::vector<proposition>, renaming_t);

  struct primed_t {
    black::identifier label;

    bool operator==(primed_t const&) const = default;
  };

  inline std::string to_string(primed_t p) {
    return "{" + to_string(p.label) + "}'";
  }

  inline proposition primed(proposition p) {
    return p.sigma()->proposition(primed_t{p.name()});
  }
  
  inline std::vector<proposition> primed(std::vector<proposition> props) {
    return rename(props, [](auto p) { return primed(p); } );
  }

  inline qbformula primed(qbformula f) {
    return rename(f, [](auto p) { return primed(p); } );
  }

  struct fresh_t {
    proposition prop;
    size_t n;

    bool operator==(fresh_t const&) const = default;
  };

  inline std::string to_string(fresh_t f) {
    return "{" + to_string(f.prop) + ", " + std::to_string(f.n) + "}";
  }

  struct fresh_gen_t {

    proposition operator()(proposition p);

    size_t next_fresh = 0;
  };

  struct stepped_t {
    proposition prop;
    size_t step;

    bool operator==(stepped_t const&) const = default;
  };

  inline std::string to_string(stepped_t s) {
    return to_string(s.prop) + ", " + std::to_string(s.step);
  }

  proposition stepped(proposition p, size_t n);
  proposition stepped(proposition p);
  
  inline std::vector<proposition> 
  stepped(std::vector<proposition> props, size_t n) {
    return rename(props, [&](auto p) { return stepped(p, n); } );
  }
  
  inline std::vector<proposition> stepped(std::vector<proposition> props) {
    return rename(props, [](auto p) { return stepped(p); } );
  }

  inline qbformula stepped(qbformula f) {
    return rename(f, [](auto p) { return stepped(p); } );
  }
  
  formula<Bool> 
  stepped(formula<Bool> f, size_t n);
  
  std::vector<proposition> 
  stepped(std::vector<proposition> props, size_t n);


}


namespace std {
  template<>
  struct hash<::synth::stepped_t> {
    size_t operator()(::synth::stepped_t s) {
      return ::black_internal::hash_combine(
        std::hash<::black::proposition>{}(s.prop),
        std::hash<size_t>{}(s.step)
      );
    }
  };

  template<>
  struct hash<::synth::primed_t> {
    size_t operator()(::synth::primed_t p) {
      return std::hash<::black::identifier>{}(p.label);
    }
  };

  template<>
  struct hash<::synth::fresh_t> {
    size_t operator()(::synth::fresh_t s) {
      return ::black_internal::hash_combine(
        std::hash<::black::proposition>{}(s.prop),
        std::hash<size_t>{}(s.n)
      );
    }
  };
}

#endif // SYNTH_GAME_COMMON_HPP
