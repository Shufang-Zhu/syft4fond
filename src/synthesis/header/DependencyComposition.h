//
// Created by Shufang Zhu on 10/03/2025.
//

#ifndef DEPENDENCYCOMPOSITION_H
#define DEPENDENCYCOMPOSITION_H

#include "SymbolicStateDfa.h"
#include "ExplicitStateDfaCudd.h"

namespace Syft {
  class DependencyComposition {
  private:
    std::vector<CUDD::BDD> fluents_;
    SymbolicStateDfa domain_;
    ExplicitStateDfaCudd goal_;
    std::shared_ptr<VarMgr> var_mgr_;
    size_t initial_state_;
    size_t state_count_;
    std::vector<size_t> final_states_;
    std::unordered_map<std::size_t, std::vector<std::pair<CUDD::BDD, std::size_t>>> transition_function_;
    std::vector<std::string> variable_names_;
    enum Inconsistency {
      split,
      remove_1,
      remove_0,
      clean,
      undefined
    };
    Inconsistency check_bad(size_t state, CUDD::BDD f, CUDD::BDD Bf);
    void prune(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_remove_0(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_remove_1(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_clean(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_split(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf);

  public:
    DependencyComposition(const SymbolicStateDfa &domain_sdfa, const ExplicitStateDfaCudd &goal_dfa_cudd);
    SymbolicStateDfa compose(const SymbolicStateDfa& domain_sdfa, const ExplicitStateDfaCudd& goal_dfa_cudd);
  };
}



#endif //DEPENDENCYCOMPOSITION_H
