//
// Created by Shufang Zhu on 10/03/2025.
//

#include "DependencyComposition.h"

namespace Syft {
  DependencyComposition::DependencyComposition(const SymbolicStateDfa &domain_sdfa, const ExplicitStateDfaCudd &goal_dfa_cudd) : domain_(domain_sdfa), goal_(goal_dfa_cudd) {
    var_mgr_ = domain_sdfa.var_mgr();
    fluents_ = var_mgr_->get_state_variables(domain_sdfa.automaton_id());

    initial_state_ = goal_dfa_cudd.initial_state();
    states_ = std::vector<size_t>(goal_dfa_cudd.state_count());
    std::iota(states_.begin(), states_.end(), 0);
    final_states_ = goal_dfa_cudd.final_states();
    transition_function_ = goal_dfa_cudd.transition_function();
    variable_names_ = goal_dfa_cudd.variable_names();

    if(goal_dfa_cudd.sink_state_exist()) {
      sink_state_ == goal_dfa_cudd.sink_state();
    } else {
      sink_state_ = states_.size();
    }
    ExplicitStateDfaCudd::Transition outgoings_of_sink = std::make_pair(var_mgr_->cudd_mgr()->bddOne(), sink_state_);
    transition_function_[sink_state_].emplace_back(outgoings_of_sink);
  }


  ExplicitStateDfaCudd DependencyComposition::compose() {
    for (size_t state : states_) {
      processing_queue_.emplace_back(state, 0); // start processing each state with the first fluent
    }
    while (!processing_queue_.empty()) {
      auto [state, fluent_index] = processing_queue_.back();
      processing_queue_.pop_back();
      std::cout << "state: " << state <<" fluent: " << fluent_index << std::endl;
      if (state == sink_state_ || fluent_index >= fluents_.size()) {
        continue; // skip processing sink state or out of fluents
      }
      CUDD::BDD f = fluents_[fluent_index];
      CUDD::BDD Bf = domain_.transition_function()[fluent_index];

      Inconsistency res = check_bad(state, f, Bf);


      if (res == clean) {
          state_clean(state);
          final_states_.erase(std::remove(final_states_.begin(), final_states_.end(), state), final_states_.end());
        } else if (res == remove_0){
          state_remove_0(state, f, Bf);
          processing_queue_.emplace_back(state, fluent_index + 1);
        } else if (res == remove_1) {
          state_remove_1(state, f, Bf);
          processing_queue_.emplace_back(state, fluent_index + 1);

        } else if (res == split) {
          std::size_t state_0 = states_.size();
          std::size_t state_1 = states_.size() + 1;
          state_split(state, f, Bf);
          final_states_.erase(std::remove(final_states_.begin(), final_states_.end(), state), final_states_.end());
          processing_queue_.emplace_back(state_0, fluent_index + 1);
          processing_queue_.emplace_back(state_1, fluent_index + 1);
        }
      }

    for (auto it = transition_function_.begin(); it != transition_function_.end(); ) {
      auto& transitions = it->second;

      transitions.erase(
          std::remove_if(transitions.begin(), transitions.end(),
                         [this](const std::pair<CUDD::BDD, std::size_t>& t) { return (t.first == var_mgr_->cudd_mgr()->bddZero()); }),
          transitions.end());

      if (transitions.empty()) {
        it = transition_function_.erase(it);
      } else {
        ++it;
      }
    }

    ExplicitStateDfaCudd dfa = ExplicitStateDfaCudd::from_defined_dfa(std::move(var_mgr_), initial_state_, sink_state_,
                                                                      true, states_.size(),
                                                                      std::move(final_states_),
                                                                      std::move(transition_function_),
                                                                      std::move(variable_names_));

    return dfa;

  }

  void DependencyComposition::state_clean(size_t s) {

    for (auto& [key, transitions] : transition_function_) {
      for (auto& transition : transitions) {
        if (transition.second == s) {
          transition.second = sink_state_;
        }
      }
    }
    transition_function_.erase(s);
  }

  void DependencyComposition::state_remove_0(size_t s, CUDD::BDD f, CUDD::BDD Bf) {
    // remove 0 from outgoing edges of s
    std::vector<ExplicitStateDfaCudd::Transition>& outgoings = transition_function_[s];
    std::vector<ExplicitStateDfaCudd::Transition> new_transitions;
    // check whether there is an edge from s to sink
    bool transition_to_sink_exists = false;
    std::size_t transition_to_sink_index = -1;
    std::size_t index = 0;
    for (auto& p : outgoings) {
      if (p.second == sink_state_) {
        transition_to_sink_exists = true;
        transition_to_sink_index = index;
        break;
      }
      ++index;
    }

    ExplicitStateDfaCudd::Transition transition_to_sink = std::make_pair(var_mgr_->cudd_mgr()->bddZero(), sink_state_);
    for (size_t i = 0; i <  outgoings.size(); i++) {
      if (outgoings.at(i).second != sink_state_) {
        outgoings[i].first = outgoings[i].first & f;
        if (transition_to_sink_exists) {
          outgoings[transition_to_sink_index].first = outgoings[transition_to_sink_index].first | (outgoings[i].first & !f);
        } else {
          transition_to_sink.first = transition_to_sink.first | (outgoings[i].first & !f);
        }
      }
    }
    if (!transition_to_sink_exists) {
      outgoings.emplace_back(transition_to_sink);
    }

    // remove 0 from incoming edges of s
    for (auto& [curr, edges] : transition_function_) {
      for (std::size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].second == s) {
          transition_function_[curr][i].first = transition_function_[curr][i].first & Bf;
          // check whether curr has an outgoing edge leading to sink
          // if yes, redirect the removed edge to sink by disjuncting the removed edge with the existing edge to sink
          // otherwise create an edge to sink that encodes the removed edge
          transition_to_sink_index = -1;
          index = 0;
          transition_to_sink_exists = false;
          for (auto& p : transition_function_[curr]) {
            if (p.second == sink_state_) {
              transition_to_sink_exists = true;
              transition_to_sink_index = index;
              break;
            }
            ++index;
          }
          if (transition_to_sink_exists) {
            transition_function_[curr][transition_to_sink_index].first = transition_function_[curr][transition_to_sink_index].first | (outgoings[i].first & !Bf);
          } else {
            transition_function_[curr].emplace_back(std::make_pair((outgoings[i].first & !Bf), s));
          }
        }
      }
    }

  }

  void DependencyComposition::state_remove_1(size_t s, CUDD::BDD f, CUDD::BDD Bf) {
    // remove 1 from outgoing edges of s
    std::vector<ExplicitStateDfaCudd::Transition>& outgoings = transition_function_[s];
    std::vector<ExplicitStateDfaCudd::Transition> new_transitions;
    // check whether there is an edge from s to sink
    bool transition_to_sink_exists = false;
    std::size_t transition_to_sink_index = -1;
    std::size_t index = 0;
    for (auto& p : outgoings) {
      if (p.second == sink_state_) {
        transition_to_sink_exists = true;
        transition_to_sink_index = index;
        break;
      }
      ++index;
    }

    ExplicitStateDfaCudd::Transition transition_to_sink = std::make_pair(var_mgr_->cudd_mgr()->bddZero(), sink_state_);
    for (size_t i = 0; i <  outgoings.size(); i++) {
      if (outgoings.at(i).second != sink_state_) {
        outgoings[i].first = outgoings[i].first & !f;
        if (transition_to_sink_exists) {
          outgoings[transition_to_sink_index].first = outgoings[transition_to_sink_index].first | (outgoings[i].first & f);
        } else {
          transition_to_sink.first = transition_to_sink.first | (outgoings[i].first & f);
        }
      }
    }
    if (!transition_to_sink_exists) {
      outgoings.emplace_back(transition_to_sink);
    }

    // remove 0 from incoming edges of s
    for (auto& [curr, edges] : transition_function_) {
      for (std::size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].second == s) {
          transition_function_[curr][i].first = transition_function_[curr][i].first & !Bf;
          // check whether curr has an outgoing edge leading to sink
          // if yes, redirect the removed edge to sink by disjuncting the removed edge with the existing edge to sink
          // otherwise create an edge to sink that encodes the removed edge
          transition_to_sink_index = -1;
          index = 0;
          transition_to_sink_exists = false;
          for (auto& p : transition_function_[curr]) {
            if (p.second == sink_state_) {
              transition_to_sink_exists = true;
              transition_to_sink_index = index;
              break;
            }
            ++index;
          }
          if (transition_to_sink_exists) {
            transition_function_[curr][transition_to_sink_index].first = transition_function_[curr][transition_to_sink_index].first | (outgoings[i].first & Bf);
          } else {
            transition_function_[curr].emplace_back(std::make_pair((outgoings[i].first & Bf), s));
          }
        }
      }
    }
  }

  void DependencyComposition::state_split(size_t s, CUDD::BDD f, CUDD::BDD Bf) {
    std::size_t state_0 = states_.size();
    std::size_t state_1 = states_.size() + 1;
    states_.push_back(state_0);
    states_.push_back(state_1);
    if (find(final_states_.begin(), final_states_.end(), s) != final_states_.end()) {
      final_states_.push_back(state_0);
      final_states_.push_back(state_1);
    }

    std::vector<ExplicitStateDfaCudd::Transition> outgoings = transition_function_[s];
    bool transition_to_sink_exists = false;
    std::size_t transition_to_sink_index = -1;
    std::size_t index = 0;
    for (auto& p : outgoings) {
      if (p.second == sink_state_) {
        transition_to_sink_exists = true;
        transition_to_sink_index = index;
        break;
      }
      ++index;
    }

    // outgoings of state_0
    std::vector<ExplicitStateDfaCudd::Transition> outgoings_0;
    ExplicitStateDfaCudd::Transition state_0_to_sink, state_1_to_sink;
    if (transition_to_sink_exists) {
      state_0_to_sink = transition_function_[s][transition_to_sink_index];
      state_1_to_sink = transition_function_[s][transition_to_sink_index];
    } else {
      state_0_to_sink = std::make_pair(var_mgr_->cudd_mgr()->bddZero(), sink_state_);
      state_1_to_sink = std::make_pair(var_mgr_->cudd_mgr()->bddZero(), sink_state_);
    }

    for (auto transition : outgoings) {
      outgoings_0.emplace_back(std::make_pair(transition.first & !f, transition.second));
      state_0_to_sink.first = state_0_to_sink.first | (transition.first & f);
    }
    if (!transition_to_sink_exists) {
      outgoings_0.emplace_back(state_0_to_sink);
    }
    transition_function_[state_0] = outgoings_0;

    // outgoings of state_1
    std::vector<ExplicitStateDfaCudd::Transition> outgoings_1;

    for (auto transition : outgoings) {
      outgoings_1.emplace_back(std::make_pair(transition.first & f, transition.second));
      state_1_to_sink.first = state_1_to_sink.first | (transition.first & !f);
    }
    if (!transition_to_sink_exists) {
      outgoings_1.emplace_back(state_1_to_sink);
    }
    transition_function_[state_1] = outgoings_1;


    // redirect all the incoming edges of s to state 0 and state 1
    for (auto& [curr, edges] : transition_function_) {
      for (std::size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].second == s) {
          CUDD::BDD transition_to_state_0 = transition_function_[curr][i].first & !Bf;
          CUDD::BDD transition_to_state_1 = transition_function_[curr][i].first & Bf;
          transition_function_[curr].erase(transition_function_[curr].begin() + i);
          transition_function_[curr].emplace_back(std::make_pair(transition_to_state_0, state_0));
          transition_function_[curr].emplace_back(std::make_pair(transition_to_state_1, state_1));
        }
      }
    }
    transition_function_.erase(s);
  }


  DependencyComposition::Inconsistency DependencyComposition::check_bad(size_t state, CUDD::BDD f, CUDD::BDD Bf) {
    std::vector<ExplicitStateDfaCudd::Transition> incomings = get_incoming_transitions(state);
    std::vector<ExplicitStateDfaCudd::Transition> outgoings = get_nonsink_outgoing_transitions(state);
    CUDD::BDD B_I_s = var_mgr_->cudd_mgr()->bddZero();
    for (std::size_t i = 0; i < incomings.size(); i++) {
      B_I_s = B_I_s + incomings[i].first;
    }
    std::cout << "B_I_s: " << B_I_s << std::endl;
    CUDD::BDD B_O_s = var_mgr_->cudd_mgr()->bddZero();
    for (std::size_t i = 0; i < outgoings.size(); i++) {
      B_O_s = B_O_s + outgoings[i].first;
    }
    std::cout << "B_O_s: " << B_O_s << std::endl;
    CUDD::BDD quantification_cube = var_mgr_->input_cube() & var_mgr_->output_cube() & var_mgr_->state_cube(domain_.automaton_id());
    std::cout << "quantification_cube: " << quantification_cube << std::endl;
    std::cout << "Bf: " << Bf << std::endl;
    // c0
    CUDD::BDD c0 = (B_I_s & !Bf).ExistAbstract(quantification_cube);
    bool c_0 = (c0 == var_mgr_->cudd_mgr()->bddOne());

    // c1
    CUDD::BDD c1 = (B_I_s & Bf).ExistAbstract(quantification_cube);
    bool c_1 = (c1 == var_mgr_->cudd_mgr()->bddOne());

    // d0
    CUDD::BDD d0 = (B_O_s & !f).ExistAbstract(quantification_cube);
    bool d_0 = (d0 == var_mgr_->cudd_mgr()->bddOne());

    // d1
    CUDD::BDD d1 = (B_O_s & f).ExistAbstract(quantification_cube);
    bool d_1 = (d1 == var_mgr_->cudd_mgr()->bddOne());

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
    std::cout << key << std::endl;
    if (answer_index.find(key) != answer_index.end()) {
      return answer_index[key];
    }

    return undefined;

  }
  std::vector<ExplicitStateDfaCudd::Transition> DependencyComposition::get_outgoing_transitions(std::size_t state) const {
    return transition_function_.at(state);
  }

  std::vector<ExplicitStateDfaCudd::Transition> DependencyComposition::get_nonsink_outgoing_transitions(std::size_t state) const {

      std::vector<ExplicitStateDfaCudd::Transition> transitions;
      for(auto transition : transition_function_.at(state)) {
        if(transition.second != sink_state_) {
          transitions.push_back(transition);
        }
      }
      return transitions;
  }

  std::vector<ExplicitStateDfaCudd::Transition> DependencyComposition::get_incoming_transitions(std::size_t state) const {
    std::vector<ExplicitStateDfaCudd::Transition> transitions;
    for (const auto& [curr, vec] : transition_function_) {
      for (const auto& [condition, succ] : vec) {
        if (succ == state) {
          ExplicitStateDfaCudd::Transition edge = std::make_pair(condition, curr);
          transitions.push_back(edge);
        }
      }
    }
    return transitions;
  }

}