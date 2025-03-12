//
// Created by Shufang Zhu on 10/03/2025.
//

#include "DependencyComposition.h"

namespace Syft {
  DependencyComposition::DependencyComposition(const SymbolicStateDfa &domain_sdfa, const ExplicitStateDfaCudd &goal_dfa_cudd) : domain_(domain_sdfa), goal_(goal_dfa_cudd) {
    var_mgr_ = domain_sdfa.var_mgr();
    fluents_ = var_mgr_->get_state_variables(domain_sdfa.automaton_id());

    initial_state_ = goal_dfa_cudd.initial_state();
    state_count_ = goal_dfa_cudd.state_count();
    final_states_ = goal_dfa_cudd.final_states();
    transition_function_ = goal_dfa_cudd.transition_function();
    variable_names_ = goal_dfa_cudd.variable_names();
  }


  SymbolicStateDfa DependencyComposition::compose(const SymbolicStateDfa &domain_sdfa, const ExplicitStateDfaCudd &goal_dfa_cudd) {
    std::vector<size_t> states;
    while (states.size() != 0) {
      int s = states.back();
      states.pop_back();
      for (int i = 0; i < fluents_.size(); i++) {
        CUDD::BDD f = fluents_[i];
        CUDD::BDD Bf = domain_sdfa.transition_function()[i];
        Inconsistency res = check_bad(s, f, Bf);
        //TODO
        prune(states, res, s, f, Bf);
      }
    }
  }

  void DependencyComposition::state_clean(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf) {

  }

  void DependencyComposition::state_remove_0(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf) {

  }

  void DependencyComposition::state_remove_1(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf) {

  }

  void DependencyComposition::state_split(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf) {

  }

  void DependencyComposition::prune(std::vector<size_t> states, Inconsistency res, size_t s, CUDD::BDD f, CUDD::BDD Bf) {
    switch (res) {
      case clean: state_clean(states, res, s, f, Bf); break;
      case remove_0: state_remove_0(states, res, s, f, Bf); break;
      case remove_1: state_remove_1(states, res, s, f, Bf); break;
      case split: state_split(states, res, s, f, Bf); break;
    }
  }


  DependencyComposition::Inconsistency DependencyComposition::check_bad(size_t state, CUDD::BDD f, CUDD::BDD Bf) {
    std::vector<std::pair<CUDD::BDD, std::size_t>> incomings = goal_.get_incoming_transitions(state);
    std::vector<std::pair<CUDD::BDD, std::size_t>> outgoings = goal_.get_nonsink_outgoing_transitions(state);
    CUDD::BDD B_I_s = var_mgr_->cudd_mgr()->bddZero();
    for (std::size_t i = 0; i < incomings.size(); i++) {
      B_I_s = B_I_s + incomings[i].first;
    }
    CUDD::BDD B_O_s = var_mgr_->cudd_mgr()->bddZero();
    for (std::size_t i = 0; i < outgoings.size(); i++) {
      B_O_s = B_O_s + outgoings[i].first;
    }
    CUDD::BDD quantification_cube = var_mgr_->input_cube() & var_mgr_->output_cube() & var_mgr_->state_cube(domain_.automaton_id());

    // c0
    CUDD::BDD c0 = (B_I_s & !Bf).ExistAbstract(quantification_cube);
    bool c_0 = c0 == var_mgr_->cudd_mgr()->bddZero()? true : false;

    // c1
    CUDD::BDD c1 = (B_I_s & Bf).ExistAbstract(quantification_cube);
    bool c_1 = c1 == var_mgr_->cudd_mgr()->bddZero()? true : false;

    // d0
    CUDD::BDD d0 = (B_O_s & !f).ExistAbstract(quantification_cube);
    bool d_0 = d0 == var_mgr_->cudd_mgr()->bddZero()? true : false;

    // d1
    CUDD::BDD d1 = (B_O_s & f).ExistAbstract(quantification_cube);
    bool d_1 = d1 == var_mgr_->cudd_mgr()->bddZero()? true : false;

    std::unordered_map<std::string, Inconsistency> answer_index = {
      {"1111", split},
      {"1110", remove_1},
      {"1101", remove_0},
      {"1010", remove_1},
      {"1011", remove_1},
      {"1001", clean},
      {"0111", remove_0},
      {"0110", clean},
      {"0101", clean}
    };

    std::string key = std::to_string(c_0) + std::to_string(c_1) + std::to_string(d_0) + std::to_string(d_1);
    if (answer_index.find(key) != answer_index.end()) {
      return answer_index[key];
    }

    return undefined;

  }


}