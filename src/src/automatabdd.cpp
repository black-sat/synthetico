//
// Created by shuzhu on 08/04/24.
//

#include "synthetico/automatabdd.hpp"

#include <black/logic/prettyprint.hpp>

#include <unordered_set>
#include "iostream"

namespace synth {

    namespace {
        struct encoderbdd {

            static formula<pLTL> nnf(formula<pLTL> f);

            void collect(spec sp);

            static proposition ground(formula<pLTL> y);

            static formula<pLTL> lift(proposition p);

            static bformula snf(formula<pLTL> f);

            static CUDD::BDD formula_to_bdd(bformula f, std::shared_ptr<varmgr> var_mgr);

            std::vector<logic::yesterday<pLTL>> yreqs;
            std::vector<logic::w_yesterday<pLTL>> zreqs;
            std::vector<proposition> variables;

            automatabdd encodebdd(spec sp, std::shared_ptr<varmgr> var_mgr);
        };

        formula<pLTL> encoderbdd::nnf(formula<pLTL> f) {
            return f.match(
                    [](logic::boolean b) { return b; },
                    [](proposition p) { return p; },
                    [](logic::disjunction<pLTL>, auto left, auto right) {
                        return nnf(left) || nnf(right);
                    },
                    [](logic::conjunction<pLTL>, auto left, auto right) {
                        return nnf(left) && nnf(right);
                    },
                    [](logic::implication<pLTL>, auto left, auto right) {
                        return nnf(!left || right);
                    },
                    [](logic::iff<pLTL>, auto left, auto right) {
                        return nnf(implies(left, right) && implies(right, left));
                    },
                    [](logic::yesterday<pLTL>, auto arg) {
                        return Y(nnf(arg));
                    },
                    [](logic::w_yesterday<pLTL>, auto arg) {
                        return Z(nnf(arg));
                    },
                    [](logic::once<pLTL>, auto arg) {
                        return O(nnf(arg));
                    },
                    [](logic::historically<pLTL>, auto arg) {
                        return H(nnf(arg));
                    },
                    [](logic::since<pLTL>, auto left, auto right) {
                        return S(nnf(left), nnf(right));
                    },
                    [](logic::triggered<pLTL>, auto left, auto right) {
                        return T(nnf(left), nnf(right));
                    },
                    [](logic::negation<pLTL>, auto arg) {
                        return arg.match(
                                [](logic::boolean b) { return b.sigma()->boolean(!b.value()); },
                                [](logic::proposition p) { return !p; },
                                [](logic::negation<pLTL>, auto op) { return op; },
                                [](logic::disjunction<pLTL>, auto left, auto right) {
                                    return nnf(!left) && nnf(!right);
                                },
                                [](logic::conjunction<pLTL>, auto left, auto right) {
                                    return nnf(!left) || nnf(!right);
                                },
                                [](logic::implication<pLTL>, auto left, auto right) {
                                    return nnf(left) && nnf(!right);
                                },
                                [](logic::iff<pLTL>, auto left, auto right) {
                                    return nnf(!implies(left,right)) || nnf(!implies(right,left));
                                },
                                [](logic::yesterday<pLTL>, auto op) {
                                    return Z(nnf(!op));
                                },
                                [](logic::w_yesterday<pLTL>, auto op) {
                                    return Y(nnf(!op));
                                },
                                [](logic::once<pLTL>, auto op) {
                                    return H(nnf(!op));
                                },
                                [](logic::historically<pLTL>, auto op) {
                                    return O(nnf(!op));
                                },
                                [](logic::since<pLTL>, auto left, auto right) {
                                    return T(nnf(!left), nnf(!right));
                                },
                                [](logic::triggered<pLTL>, auto left, auto right) {
                                    return S(nnf(!left), nnf(!right));
                                }
                        );
                    }
            );
        }

        void encoderbdd::collect(spec sp) {

            std::unordered_set<proposition> _variables;
            std::unordered_set<logic::yesterday<pLTL>> _yreqs;
            std::unordered_set<logic::w_yesterday<pLTL>> _zreqs;

            sp.type.match(
                    [&](game_t::eventually) {
                        _variables.insert(ground(Y(sp.formula)));
                        _yreqs.insert(Y(sp.formula));
                    },
                    [&](game_t::always) {
                        _variables.insert(ground(Z(sp.formula)));
                        _zreqs.insert(Z(sp.formula));
                    }
            );

            transform(sp.formula, [&](auto child) {
                child.match(
                        [&](logic::yesterday<pLTL> y) {
                            _variables.insert(ground(y));
                            _yreqs.insert(y);
                        },
                        [&](logic::w_yesterday<pLTL> z) {
                            _variables.insert(ground(z));
                            _zreqs.insert(z);
                        },
                        [&](logic::since<pLTL> s) {
                            _variables.insert(ground(Y(s)));
                            _yreqs.insert(Y(s));
                        },
                        [&](logic::triggered<pLTL> s) {
                            _variables.insert(ground(Z(s)));
                            _zreqs.insert(Z(s));
                        },
                        [&](logic::once<pLTL> o) {
                            _variables.insert(ground(Y(o)));
                            _yreqs.insert(Y(o));
                        },
                        [&](logic::historically<pLTL> h) {
                            _variables.insert(ground(Z(h)));
                            _zreqs.insert(Z(h));
                        },
                        [](otherwise) { }
                );
            });

            variables.insert(variables.begin(), _variables.begin(), _variables.end());
            yreqs.insert(yreqs.begin(), _yreqs.begin(), _yreqs.end());
            zreqs.insert(zreqs.begin(), _zreqs.begin(), _zreqs.end());
        }

        proposition encoderbdd::ground(formula<pLTL> f) {
            return f.sigma()->proposition(f);
        }

        formula<pLTL> encoderbdd::lift(proposition p) {
            auto name = p.name().to<formula<pLTL>>();
            black_assert(name);

            return *name;
        }

        bformula encoderbdd::snf(formula<pLTL> f) {
            return f.match(
                    [](logic::boolean b) { return b; },
                    [](logic::proposition p) { return p; },
                    [](logic::negation<pLTL>, auto arg) {
                        return !snf(arg);
                    },
                    [](logic::disjunction<pLTL>, auto left, auto right) {
                        return snf(left) || snf(right);
                    },
                    [](logic::conjunction<pLTL>, auto left, auto right) {
                        return snf(left) && snf(right);
                    },
                    [](logic::yesterday<pLTL> y) {
                        return ground(y);
                    },
                    [](logic::w_yesterday<pLTL> z) {
                        return ground(z);
                    },
                    [](logic::once<pLTL>, auto arg) {
                        return snf(arg) || ground(Y(O(arg)));
                    },
                    [](logic::historically<pLTL>, auto arg) {
                        return snf(arg) && ground(Z(H(arg)));
                    },
                    [](logic::since<pLTL>, auto left, auto right) {
                        return snf(right) || (snf(left) && ground(Y(S(left, right))));
                    },
                    [](logic::triggered<pLTL>, auto left, auto right) {
                        return snf(right) && (snf(left) || ground(Z(T(left, right))));
                    },
                    [](logic::implication<pLTL>) -> bformula { black_unreachable(); },
                    [](logic::iff<pLTL>) -> bformula { black_unreachable(); }
            );
        }

        CUDD::BDD encoderbdd::formula_to_bdd(bformula f, std::shared_ptr<varmgr> var_mgr) {
            CUDD::BDD con_bdd = f.match(
                    [&var_mgr](logic::boolean b) {
                        std::string name = to_string(b);
                        if (name == "True")
                            return var_mgr->cudd_mgr()->bddOne();
                        else
                            return var_mgr->cudd_mgr()->bddZero(); },
                    [&var_mgr](logic::proposition p) {
                        std::string name = to_string(p);
                        return var_mgr->name_to_variable(name); },
                    [&var_mgr](logic::negation<Bool>, auto arg) {
                        return !formula_to_bdd(arg, var_mgr);
                    },
                    [&var_mgr](logic::disjunction<Bool>, auto left, auto right) {
                        return formula_to_bdd(left, var_mgr) | formula_to_bdd(right, var_mgr);
                    },
                    [&var_mgr](logic::conjunction<Bool>, auto left, auto right) {
                        return formula_to_bdd(left, var_mgr) & formula_to_bdd(right, var_mgr);
                    },
                    [](logic::implication<Bool>) -> CUDD::BDD { black_unreachable(); },
                    [](logic::iff<Bool>) -> CUDD::BDD { black_unreachable(); }
            );
            return con_bdd;
        }

        automatabdd encoderbdd::encodebdd(spec sp, std::shared_ptr<varmgr> var_mgr) {
            logic::alphabet &sigma = *sp.formula.sigma();
            sp.formula = encoderbdd::nnf(sp.formula);

            collect(sp);
            std::vector<std::string> ins, outs;
            for (auto prop : sp.inputs){
                ins.push_back(to_string(prop));
            }
            for (auto prop : sp.outputs){
                outs.push_back(to_string(prop));
            }

            var_mgr->create_named_variables(ins);
            var_mgr->create_named_variables(outs);

            var_mgr->partition_variables(ins, outs);
            std::vector<int> initial_state;

            for (size_t i = 0; i < zreqs.size(); i++){
                initial_state.push_back(1);
            }
            for (size_t i = 0; i < yreqs.size(); i++){
                initial_state.push_back(0);
            }

            black_assert(initial_state.size() == variables.size());

            std::vector<std::string> state_vars;
            for (auto prop : variables){
                state_vars.push_back(to_string(prop));
            }

            for (size_t i = 0; i < variables.size(); i++){
                black_assert(state_vars[i] == to_string(variables[i]));
            }

            size_t automata_id = var_mgr->create_named_state_variables(state_vars);

            bformula init =
                    big_and(sigma, zreqs, [](auto req) {
                        return ground(req);
                    }) &&
                    big_and(sigma, yreqs, [](auto req) {
                        return !ground(req);
                    });


            bformula trans = big_and(sigma, variables, [](proposition var) {
                auto req = lift(var).to<logic::unary<pLTL>>();
                std::cerr << to_string(var) << "\n";
                std::cerr << to_string(req->argument()) << "\n";
                std::cerr << to_string(snf(req->argument())) << "\n";
                std::cerr << to_string(primed(var)) << "\n";
                black_assert(req);
                std::cerr << to_string(logic::iff(primed(var), snf(req->argument()))) << "\n";
                return logic::iff(primed(var), snf(req->argument()));
            });


            std::vector<CUDD::BDD> transition_function;

            for (auto var : variables){
                auto req = lift(var).to<logic::unary<pLTL>>();
                auto con = snf(req->argument());

                std::cerr << to_string(con) << "\n";

                transition_function.push_back(formula_to_bdd(con, var_mgr));
                //TODO
                //convert propositional formula con to BDD
            }

            bformula objective = sp.type.match(
                    [&](game_t::eventually) {
                        return ground(Y(sp.formula));
                    },
                    [&](game_t::always) {
                        return ground(Z(sp.formula));
                    }
            );

            CUDD::BDD final_states = sp.type.match(
                    [&](game_t::eventually) {
                        std::string name = to_string(ground(Y(sp.formula)));
                        return var_mgr->name_to_variable(name);
                    },
                    [&](game_t::always) {
                        std::string name = to_string(ground(Z(sp.formula)));
                        return var_mgr->name_to_variable(name);
                    }
            );

            return automatabdd{sp.inputs, sp.outputs, variables, init, trans, objective, var_mgr, automata_id, initial_state, transition_function, final_states};
        }
    }

    automatabdd encodebdd(spec sp, std::shared_ptr<varmgr> var_mgr) {
        return encoderbdd{}.encodebdd(sp, var_mgr);
    }

    std::ostream &operator<<(std::ostream &str, automatabdd aut) {
        str << "inputs:\n";
        for(auto in : aut.inputs) {
            str << "- " << to_string(in) << "\n";
        }

        str << "\noutputs:\n";
        for(auto out : aut.outputs) {
            str << "- " << to_string(out) << "\n";
        }

        str << "\ninit:\n";
        str << "- " << to_string(aut.init) << "\n";

        str << "\ntrans:\n";
        str << "- " << to_string(aut.trans) << "\n";

        str << "\nobjective:\n";
        str << "- " << to_string(aut.objective) << "\n";

        return str;
    }
}
