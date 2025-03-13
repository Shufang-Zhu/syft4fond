//
// Created by Shufang Zhu on 10/02/2025.
//

#ifndef EXPLICITSTATEDFACUDD_H
#define EXPLICITSTATEDFACUDD_H

#include <cuddObj.hh>

#include"ExplicitStateDfa.h"


namespace Syft {

  class ExplicitStateDfaCudd {

    private:

      std::shared_ptr<VarMgr> var_mgr_;
      size_t initial_state_;
      size_t state_count_;
      std::vector<size_t> final_states_;
      std::unordered_map<std::size_t, std::vector<std::pair<CUDD::BDD, std::size_t>>> transition_function_;
      std::vector<std::string> variable_names_;
      bool sink_state_exist_ = false;
      size_t sink_state_ = 0;

      ExplicitStateDfaCudd(std::shared_ptr<VarMgr> var_mgr);


    public:

      static ExplicitStateDfaCudd from_explicit_dfa(std::shared_ptr<Syft::VarMgr> var_mgr, const ExplicitStateDfa &explicit_dfa);

      static ExplicitStateDfaCudd from_defined_dfa(std::shared_ptr<VarMgr> var_mgr,
                                                   size_t initial_state,
                                                   size_t sink_state,
                                                   bool sink_state_exist,
                                                   size_t state_count,
                                                   std::vector<size_t> final_states,
                                                   std::unordered_map<std::size_t, std::vector<std::pair<CUDD::BDD, std::size_t> > > transition_function,
                                                   std::vector<std::string> variable_names);

      std::shared_ptr<VarMgr> var_mgr() const;

      std::size_t initial_state() const;

      bool sink_state_exist() const;

      std::size_t sink_state() const;

      std::vector<size_t> final_states() const;

      std::size_t state_count() const;

      std::vector<std::string> variable_names() const;

      std::size_t bdd_nodes_count() const;

      typedef std::pair<CUDD::BDD, std::size_t> Transition;

      std::unordered_map<std::size_t, std::vector<Transition>> transition_function() const;

      std::vector<std::pair<CUDD::BDD, std::size_t>> get_incoming_transitions(std::size_t state) const;

      std::vector<std::pair<CUDD::BDD, std::size_t>> get_outgoing_transitions(std::size_t state) const;

      std::vector<std::pair<CUDD::BDD, std::size_t>> get_nonsink_outgoing_transitions(std::size_t state) const;

      void dfa_print() const;

  };
}



#endif //EXPLICITSTATEDFACUDD_H
