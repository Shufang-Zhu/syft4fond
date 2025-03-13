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
    std::vector<size_t> final_states_;
    std::vector<size_t> states_;
    size_t sink_state_;
    std::unordered_map<std::size_t, std::vector<ExplicitStateDfaCudd::Transition>> transition_function_;
    std::vector<std::string> variable_names_;
    enum Inconsistency {
      split,
      remove_1,
      remove_0,
      clean,
      undefined
    };
    std::vector<std::pair<size_t, std::size_t>> processing_queue_;
    Inconsistency check_bad(size_t state, CUDD::BDD f, CUDD::BDD Bf);
    void state_remove_0(size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_remove_1(size_t s, CUDD::BDD f, CUDD::BDD Bf);
    void state_clean(size_t s);
    void state_split(size_t s, CUDD::BDD f, CUDD::BDD Bf);
    std::vector<std::pair<CUDD::BDD, std::size_t>> get_incoming_transitions(std::size_t state) const;

    std::vector<std::pair<CUDD::BDD, std::size_t>> get_outgoing_transitions(std::size_t state) const;

    std::vector<std::pair<CUDD::BDD, std::size_t>> get_nonsink_outgoing_transitions(std::size_t state) const;

  public:
    DependencyComposition(const SymbolicStateDfa &domain_sdfa, const ExplicitStateDfaCudd &goal_dfa_cudd);
    ExplicitStateDfaCudd compose();
  };
}



#endif //DEPENDENCYCOMPOSITION_H
