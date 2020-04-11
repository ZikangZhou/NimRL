//
// Created by 周梓康 on 2020/3/2.
//

#ifndef NIM_RL_AGENT_H_
#define NIM_RL_AGENT_H_

#include <algorithm>
#include <climits>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "nim_rl/action.h"
#include "nim_rl/state.h"

namespace std {
using nim_rl::State;
using nim_rl::Action;
template<>
struct hash<std::pair<State, Action>> {
  std::size_t operator()(const std::pair<State, Action> &state_action) const {
    std::size_t seed = 0;
    seed ^= std::hash<State>()(state_action.first) + 0x9e3779b9
        + (seed << 6u) + (seed >> 2u);
    seed ^= std::hash<Action>()(state_action.second) + 0x9e3779b9
        + (seed << 6u) + (seed >> 2u);
    return seed;
  }
};
}  // namespace std

namespace nim_rl {

constexpr double kDefaultThreshold = 1e-4;
constexpr double kDefaultAlpha = 0.5;
constexpr double kDefaultGamma = 1.0;
constexpr double kDefaultEpsilon = 1.0;
constexpr double kDefaultEpsilonDecayFactor = 0.9;
constexpr double kDefaultMinEpsilon = 0.01;
constexpr int kDefaultN = 1;

enum class ImportanceSampling {
  kWeighted,
  kNormal,
};

Action SampleAction(const std::vector<Action> &);

State SampleState(const std::vector<State> &);

class Game;

class Agent {
  friend void swap(Game &, Game &);
  friend class Game;

 public:
  using Reward = double;
  Agent() = default;
  Agent(const Agent &) = delete;
  Agent(Agent &&agent) noexcept { MoveGames(&agent); }
  Agent &operator=(const Agent &) = delete;
  Agent &operator=(Agent &&) noexcept;
  virtual ~Agent() { RemoveFromGames(); }
  std::unordered_set<Game *> GetGames() const { return games_; }
  virtual void Initialize(const std::vector<State> &) {}
  virtual void Reset() { current_state_ = State(); }
  virtual Action Step(Game *, bool is_evaluation);

 protected:
  State current_state_;
  std::unordered_set<Game *> games_;
  virtual Action Policy(const State &, bool is_evaluation) = 0;
  virtual void Update(const State &update_state, const State &current_state,
                      Reward reward) { current_state_ = current_state; }

 private:
  void AddGame(Game *game) { games_.insert(game); }
  void MoveGames(Agent *);
  void RemoveFromGames();
  void RemoveGame(Game *game) { games_.erase(game); }
};

class EpsilonGreedyPolicy {
 public:
  explicit
  EpsilonGreedyPolicy(double epsilon = kDefaultEpsilon,
                      double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                      double min_epsilon = kDefaultMinEpsilon)
      : epsilon_(epsilon),
        epsilon_decay_factor_(epsilon_decay_factor),
        min_epsilon_(min_epsilon) {}
  double GetEpsilon() const { return epsilon_; }
  double GetEpsilonDecayFactor() const { return epsilon_decay_factor_; }
  void SetEpsilon(double epsilon) { epsilon_ = epsilon; }
  void SetEpsilonDecayFactor(double decay_epsilon) {
    epsilon_decay_factor_ = decay_epsilon;
  }
  void UpdateEpsilon() {
    epsilon_ = std::max(min_epsilon_, epsilon_ * epsilon_decay_factor_);
  }

 protected:
  double epsilon_;
  double epsilon_decay_factor_;
  double min_epsilon_;
  Action EpsilonGreedy(const std::vector<Action> &legal_actions,
                       const std::vector<Action> &greedy_actions) {
    return (dist_epsilon_(rng_) < epsilon_) ? SampleAction(legal_actions)
                                            : SampleAction(greedy_actions);
  }

 private:
  std::uniform_real_distribution<> dist_epsilon_{0, 1};
  std::mt19937 rng_{std::random_device{}()};
};

class RandomAgent : public Agent {
 public:
  RandomAgent() = default;
  RandomAgent(const RandomAgent &) = delete;
  RandomAgent(RandomAgent &&) = default;
  RandomAgent &operator=(const RandomAgent &) = delete;
  RandomAgent &operator=(RandomAgent &&) = default;

 private:
  Action Policy(const State &, bool /*is_evaluation*/) override;
};

class HumanAgent : public Agent {
 public:
  explicit HumanAgent(std::istream &is = std::cin, std::ostream &os = std::cout)
      : is_(is), os_(os) {}
  HumanAgent(const HumanAgent &) = delete;
  HumanAgent(HumanAgent &&) = default;
  HumanAgent &operator=(const HumanAgent &) = delete;
  HumanAgent &operator=(HumanAgent &&) = delete;

 private:
  std::istream &is_;
  std::ostream &os_;
  Action Policy(const State &, bool /*is_evaluation*/) override;
};

class OptimalAgent : public Agent {
 public:
  OptimalAgent() = default;
  OptimalAgent(const OptimalAgent &) = delete;
  OptimalAgent(OptimalAgent &&) = default;
  OptimalAgent &operator=(const OptimalAgent &) = delete;
  OptimalAgent &operator=(OptimalAgent &&) = default;

 private:
  Action Policy(const State &, bool /*is_evaluation*/) override;
};

class RLAgent : public Agent {
 public:
  using StateAction = std::pair<State, Action>;
  using StateProb = std::pair<State, double>;
  using TimeStep = std::tuple<State, Action, Reward>;
  RLAgent() = default;
  RLAgent(const RLAgent &) = delete;
  RLAgent(RLAgent &&) = default;
  RLAgent &operator=(const RLAgent &) = delete;
  RLAgent &operator=(RLAgent &&) = default;
  virtual std::unordered_map<State,
                             Reward> GetValues() const { return values_; }
  void Initialize(const std::vector<State> &) override;
  double OptimalActionsRatio();
  void Reset() override;
  virtual void SetValues(const std::unordered_map<State, Reward> &values) {
    values_ = values;
  }
  virtual void SetValues(std::unordered_map<State, Reward> &&values) {
    values_ = std::move(values);
  }

 protected:
  std::unordered_map<State, Reward> values_;
  Reward greedy_value_ = 0.0;
  std::vector<Action> legal_actions_;
  std::vector<Action> greedy_actions_;
  Action Policy(const State &, bool is_evaluation) override;
  virtual Action PolicyImpl(const std::vector<Action> &legal_actions,
                            const std::vector<Action> &greedy_actions) = 0;
};

class DPAgent : public RLAgent {
 public:
  explicit DPAgent(double threshold = kDefaultThreshold)
      : threshold_(threshold) {}
  DPAgent(const DPAgent &) = delete;
  DPAgent(DPAgent &&) = default;
  DPAgent &operator=(const DPAgent &) = delete;
  DPAgent &operator=(DPAgent &&) = default;
  double GetThreshold() const { return threshold_; }
  std::unordered_map<StateAction,
                     std::vector<StateProb>> GetTransitions() const {
    return transitions_;
  }
  void Initialize(const std::vector<State> &) override;
  void SetThreshold(double threshold) { threshold_ = threshold; }
  template<typename T>
  void SetTransitions(T &&transitions) {
    transitions_ = std::forward<T>(transitions);
  }

 protected:
  std::unordered_map<StateAction, std::vector<StateProb>> transitions_;
  double threshold_;

 private:
  Action PolicyImpl(const std::vector<Action> &/*legal_actions*/,
                    const std::vector<Action> &greedy_actions) override {
    return SampleAction(greedy_actions);
  }
};

class PolicyIterationAgent : public DPAgent {
 public:
  explicit PolicyIterationAgent(double threshold = kDefaultThreshold)
      : DPAgent(threshold) {}
  PolicyIterationAgent(const PolicyIterationAgent &) = delete;
  PolicyIterationAgent(PolicyIterationAgent &&) = default;
  PolicyIterationAgent &operator=(const PolicyIterationAgent &) = delete;
  PolicyIterationAgent &operator=(PolicyIterationAgent &&) = default;
  void Initialize(const std::vector<State> &) override;

 private:
  void PolicyIteration(const std::vector<State> &);
};

class ValueIterationAgent : public DPAgent {
 public:
  explicit ValueIterationAgent(double threshold = kDefaultThreshold)
      : DPAgent(threshold) {}
  ValueIterationAgent(const ValueIterationAgent &) = delete;
  ValueIterationAgent(ValueIterationAgent &&) = default;
  ValueIterationAgent &operator=(const ValueIterationAgent &) = delete;
  ValueIterationAgent &operator=(ValueIterationAgent &&) = default;
  void Initialize(const std::vector<State> &) override;

 private:
  void ValueIteration(const std::vector<State> &);
};

class MonteCarloAgent : public RLAgent {
 public:
  explicit
  MonteCarloAgent(double gamma = kDefaultGamma) : gamma_(gamma) {}
  MonteCarloAgent(const MonteCarloAgent &) = delete;
  MonteCarloAgent(MonteCarloAgent &&) = default;
  MonteCarloAgent &operator=(const MonteCarloAgent &) = delete;
  MonteCarloAgent &operator=(MonteCarloAgent &&) = default;
  double GetGamma() const { return gamma_; }
  void Initialize(const std::vector<State> &) override;
  void Reset() override;
  void SetGamma(double gamma) { gamma_ = gamma; }
  Action Step(Game *, bool is_evaluation) override;

 protected:
  double gamma_;
  std::vector<TimeStep> trajectory_;
  std::unordered_map<State, double> cumulative_sums_;
  virtual void Update();

 private:
  Action PolicyImpl(const std::vector<Action> &/*legal_actions*/,
                    const std::vector<Action> &greedy_actions) override {
    return SampleAction(greedy_actions);
  }
};

class ESMonteCarloAgent : public MonteCarloAgent {
 public:
  explicit ESMonteCarloAgent(double gamma = kDefaultGamma)
      : MonteCarloAgent(gamma) {}
  ESMonteCarloAgent(const ESMonteCarloAgent &) = delete;
  ESMonteCarloAgent(ESMonteCarloAgent &&) = default;
  ESMonteCarloAgent &operator=(const ESMonteCarloAgent &) = delete;
  ESMonteCarloAgent &operator=(ESMonteCarloAgent &&) = default;
  Action Step(Game *, bool is_evaluation) override;
};

class OnPolicyMonteCarloAgent
    : public MonteCarloAgent, public EpsilonGreedyPolicy {
 public:
  explicit OnPolicyMonteCarloAgent(double gamma = kDefaultGamma,
                                   double epsilon = kDefaultEpsilon,
                                   double epsilon_decay_factor
                                   = kDefaultEpsilonDecayFactor,
                                   double min_epsilon = kDefaultMinEpsilon)
      : MonteCarloAgent(gamma),
        EpsilonGreedyPolicy(epsilon, epsilon_decay_factor, min_epsilon) {}
  OnPolicyMonteCarloAgent(const OnPolicyMonteCarloAgent &) = delete;
  OnPolicyMonteCarloAgent(OnPolicyMonteCarloAgent &&) = default;
  OnPolicyMonteCarloAgent &operator=(const OnPolicyMonteCarloAgent &) = delete;
  OnPolicyMonteCarloAgent &operator=(OnPolicyMonteCarloAgent &&) = default;

 private:
  Action PolicyImpl(const std::vector<Action> &legal_actions,
                    const std::vector<Action> &greedy_actions) override {
    return EpsilonGreedy(legal_actions, greedy_actions);
  }
};

class OffPolicyMonteCarloAgent
    : public MonteCarloAgent, public EpsilonGreedyPolicy {
 public:
  explicit OffPolicyMonteCarloAgent(double gamma = kDefaultGamma,
                                    ImportanceSampling importance_sampling
                                    = ImportanceSampling::kWeighted,
                                    double epsilon = kDefaultEpsilon,
                                    double epsilon_decay_factor
                                    = kDefaultEpsilonDecayFactor,
                                    double min_epsilon = kDefaultMinEpsilon)
      : MonteCarloAgent(gamma),
        importance_sampling_(importance_sampling),
        EpsilonGreedyPolicy(epsilon, epsilon_decay_factor, min_epsilon) {}
  OffPolicyMonteCarloAgent(const OffPolicyMonteCarloAgent &) = delete;
  OffPolicyMonteCarloAgent(OffPolicyMonteCarloAgent &&) = default;
  OffPolicyMonteCarloAgent &
  operator=(const OffPolicyMonteCarloAgent &) = delete;
  OffPolicyMonteCarloAgent &operator=(OffPolicyMonteCarloAgent &&) = default;

 private:
  ImportanceSampling importance_sampling_;
  Action PolicyImpl(const std::vector<Action> &legal_actions,
                    const std::vector<Action> &greedy_actions) override {
    return EpsilonGreedy(legal_actions, greedy_actions);
  }
  void Update() override;
};

class TDAgent : public RLAgent, public EpsilonGreedyPolicy {
 public:
  explicit TDAgent(double alpha = kDefaultAlpha,
                   double gamma = kDefaultGamma,
                   double epsilon = kDefaultEpsilon,
                   double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                   double min_epsilon = kDefaultMinEpsilon)
      : EpsilonGreedyPolicy(epsilon, epsilon_decay_factor, min_epsilon),
        alpha_(alpha),
        gamma_(gamma) {}
  TDAgent(const TDAgent &) = delete;
  TDAgent(TDAgent &&) = default;
  TDAgent &operator=(const TDAgent &) = delete;
  TDAgent &operator=(TDAgent &&) = default;
  double GetAlpha() const { return alpha_; }
  double GetGamma() const { return gamma_; }
  void SetAlpha(double alpha) { alpha_ = alpha; }
  void SetGamma(double gamma) { gamma_ = gamma; }
  Action Step(Game *, bool is_evaluation) override;

 protected:
  double alpha_;
  double gamma_;
  Action PolicyImpl(const std::vector<Action> &legal_actions,
                    const std::vector<Action> &greedy_actions) override {
    return EpsilonGreedy(legal_actions, greedy_actions);
  }
};

class QLearningAgent : public TDAgent {
 public:
  explicit
  QLearningAgent(double alpha = kDefaultAlpha,
                 double gamma = kDefaultGamma,
                 double epsilon = kDefaultEpsilon,
                 double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                 double min_epsilon = kDefaultMinEpsilon)
      : TDAgent(alpha, gamma, epsilon, epsilon_decay_factor, min_epsilon) {}
  QLearningAgent(const QLearningAgent &) = delete;
  QLearningAgent(QLearningAgent &&) = default;
  QLearningAgent &operator=(const QLearningAgent &) = delete;
  QLearningAgent &operator=(QLearningAgent &&) = default;

 private:
  void Update(const State &update_state, const State &current_state,
              Reward reward) override;
};

class SarsaAgent : public TDAgent {
 public:
  explicit SarsaAgent(double alpha = kDefaultAlpha,
                      double gamma = kDefaultGamma,
                      double epsilon = kDefaultEpsilon,
                      double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                      double min_epsilon = kDefaultMinEpsilon)
      : TDAgent(alpha, gamma, epsilon, epsilon_decay_factor, min_epsilon) {}
  SarsaAgent(const SarsaAgent &) = delete;
  SarsaAgent(SarsaAgent &&) = default;
  SarsaAgent &operator=(const SarsaAgent &) = delete;
  SarsaAgent &operator=(SarsaAgent &&) = default;

 private:
  void Update(const State &update_state, const State &current_state,
              Reward reward) override;
};

class ExpectedSarsaAgent : public TDAgent {
 public:
  explicit
  ExpectedSarsaAgent(double alpha = kDefaultAlpha,
                     double gamma = kDefaultGamma,
                     double epsilon = kDefaultEpsilon,
                     double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                     double min_epsilon = kDefaultMinEpsilon)
      : TDAgent(alpha, gamma, epsilon, epsilon_decay_factor, min_epsilon) {}
  ExpectedSarsaAgent(const ExpectedSarsaAgent &) = delete;
  ExpectedSarsaAgent(ExpectedSarsaAgent &&) = default;
  ExpectedSarsaAgent &operator=(const ExpectedSarsaAgent &) = delete;
  ExpectedSarsaAgent &operator=(ExpectedSarsaAgent &&) = default;
  void Reset() override;

 private:
  std::vector<State> next_states_;
  Action Policy(const State &, bool is_evaluation) override;
  template<typename T>
  void SetNextStates(T &&next_states) {
    next_states_ = std::forward<T>(next_states);
  }
  void Update(const State &update_state, const State &current_state,
              Reward reward) override;
};

class DoubleLearningAgent : public TDAgent {
 public:
  explicit
  DoubleLearningAgent(double alpha = kDefaultAlpha,
                      double gamma = kDefaultGamma,
                      double epsilon = kDefaultEpsilon,
                      double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                      double min_epsilon = kDefaultMinEpsilon)
      : TDAgent(alpha, gamma, epsilon, epsilon_decay_factor, min_epsilon) {}
  DoubleLearningAgent(const DoubleLearningAgent &) = delete;
  DoubleLearningAgent(DoubleLearningAgent &&) = default;
  DoubleLearningAgent &operator=(const DoubleLearningAgent &) = delete;
  DoubleLearningAgent &operator=(DoubleLearningAgent &&) = default;
  std::unordered_map<State, Reward> GetValues() const override;
  void Initialize(const std::vector<State> &) override;
  void Reset() override;
  void SetValues(const std::unordered_map<State, Reward> &values) override {
    values_ = values_2_ = values;
  }
  void SetValues(std::unordered_map<State, Reward> &&values) override {
    values_ = values_2_ = std::move(values);
  }

 protected:
  std::unordered_map<State, Reward> values_2_;
  bool flag_ = false;
  virtual void DoUpdate(const State &current_state, const State &next_state,
                        Reward reward,
                        std::unordered_map<State, Reward> *values) = 0;
  Action Policy(const State &, bool is_evaluation) override;
  void Update(const State &update_state, const State &current_state,
              Reward reward) override;

 private:
  std::bernoulli_distribution dist_flag_{};
  std::mt19937 rng_{std::random_device{}()};
};

class DoubleQLearningAgent : public DoubleLearningAgent {
 public:
  explicit
  DoubleQLearningAgent(double alpha = kDefaultAlpha,
                       double gamma = kDefaultGamma,
                       double epsilon = kDefaultEpsilon,
                       double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                       double min_epsilon = kDefaultMinEpsilon)
      : DoubleLearningAgent(alpha, gamma, epsilon, epsilon_decay_factor,
                            min_epsilon) {}
  DoubleQLearningAgent(const DoubleQLearningAgent &) = delete;
  DoubleQLearningAgent(DoubleQLearningAgent &&) = default;
  DoubleQLearningAgent &operator=(const DoubleQLearningAgent &) = delete;
  DoubleQLearningAgent &operator=(DoubleQLearningAgent &&) = default;

 private:
  void DoUpdate(const State &update_state,
                const State &current_state,
                Reward reward,
                std::unordered_map<State, Reward> *values) override;
};

class DoubleSarsaAgent : public DoubleLearningAgent {
 public:
  explicit
  DoubleSarsaAgent(double alpha = kDefaultAlpha,
                   double gamma = kDefaultGamma,
                   double epsilon = kDefaultEpsilon,
                   double epsilon_decay_factor = kDefaultEpsilonDecayFactor,
                   double min_epsilon = kDefaultMinEpsilon)
      : DoubleLearningAgent(alpha, gamma, epsilon, epsilon_decay_factor,
                            min_epsilon) {}
  DoubleSarsaAgent(const DoubleSarsaAgent &) = delete;
  DoubleSarsaAgent(DoubleSarsaAgent &&) = default;
  DoubleSarsaAgent &operator=(const DoubleSarsaAgent &) = delete;
  DoubleSarsaAgent &operator=(DoubleSarsaAgent &&) = default;

 private:
  void DoUpdate(const State &update_state,
                const State &current_state,
                Reward reward,
                std::unordered_map<State, Reward> *values) override;
};

class DoubleExpectedSarsaAgent : public DoubleLearningAgent {
 public:
  explicit DoubleExpectedSarsaAgent(double alpha = kDefaultAlpha,
                                    double gamma = kDefaultGamma,
                                    double epsilon = kDefaultEpsilon,
                                    double epsilon_decay_factor
                                    = kDefaultEpsilonDecayFactor,
                                    double min_epsilon = kDefaultMinEpsilon)
      : DoubleLearningAgent(alpha, gamma, epsilon, epsilon_decay_factor,
                            min_epsilon) {}
  DoubleExpectedSarsaAgent(const DoubleExpectedSarsaAgent &) = delete;
  DoubleExpectedSarsaAgent(DoubleExpectedSarsaAgent &&) = default;
  DoubleExpectedSarsaAgent &
  operator=(const DoubleExpectedSarsaAgent &) = delete;
  DoubleExpectedSarsaAgent &operator=(DoubleExpectedSarsaAgent &&) = default;
  void Reset() override;

 private:
  std::vector<State> next_states_;
  void DoUpdate(const State &update_state,
                const State &current_state,
                Reward reward,
                std::unordered_map<State, Reward> *values) override;
  Action Policy(const State &, bool is_evaluation) override;
  template<typename T>
  void SetNextStates(T &&next_states) {
    next_states_ = std::forward<T>(next_states);
  }
};

class NStepBootstrappingAgent : public TDAgent {
 public:
  explicit NStepBootstrappingAgent(double alpha = kDefaultAlpha,
                                   double gamma = kDefaultGamma,
                                   int n = kDefaultN,
                                   double epsilon = kDefaultEpsilon,
                                   double epsilon_decay_factor
                                   = kDefaultEpsilonDecayFactor,
                                   double min_epsilon = kDefaultMinEpsilon)
      : TDAgent(alpha, gamma, epsilon, epsilon_decay_factor, min_epsilon),
        n_(n) {}
  NStepBootstrappingAgent(const NStepBootstrappingAgent &) = delete;
  NStepBootstrappingAgent(NStepBootstrappingAgent &&) = default;
  NStepBootstrappingAgent &operator=(const NStepBootstrappingAgent &) = delete;
  NStepBootstrappingAgent &operator=(NStepBootstrappingAgent &&) = default;
  int GetN() const { return n_; }
  void Reset() override;
  void SetN(int n) { n_ = n; }
  Action Step(Game *, bool is_evaluation) override;

 protected:
  int n_;
  int current_time_ = 0;
  int terminal_time_ = INT_MAX;
  int update_time_ = 0;
  std::vector<TimeStep> trajectory_;
  virtual void Update(const State &update_state,
                      const State &current_state) = 0;
};

class NStepSarsaAgent : public NStepBootstrappingAgent {
 public:
  explicit NStepSarsaAgent(double alpha = kDefaultAlpha,
                           double gamma = kDefaultGamma,
                           int n = kDefaultN,
                           double epsilon = kDefaultEpsilon,
                           double epsilon_decay_factor
                           = kDefaultEpsilonDecayFactor,
                           double min_epsilon = kDefaultMinEpsilon)
      : NStepBootstrappingAgent(alpha, gamma, n, epsilon, epsilon_decay_factor,
                                min_epsilon) {}
  NStepSarsaAgent(const NStepSarsaAgent &) = delete;
  NStepSarsaAgent(NStepSarsaAgent &&) = default;
  NStepSarsaAgent &operator=(const NStepSarsaAgent &) = delete;
  NStepSarsaAgent &operator=(NStepSarsaAgent &&) = default;

 private:
  void Update(const State &update_state, const State &current_state) override;
};

class NStepExpectedSarsaAgent : public NStepBootstrappingAgent {
 public:
  explicit NStepExpectedSarsaAgent(double alpha = kDefaultAlpha,
                                   double gamma = kDefaultGamma,
                                   int n = kDefaultN,
                                   double epsilon = kDefaultEpsilon,
                                   double epsilon_decay_factor
                                   = kDefaultEpsilonDecayFactor,
                                   double min_epsilon = kDefaultMinEpsilon)
      : NStepBootstrappingAgent(alpha, gamma, n, epsilon, epsilon_decay_factor,
                                min_epsilon) {}
  NStepExpectedSarsaAgent(const NStepExpectedSarsaAgent &) = delete;
  NStepExpectedSarsaAgent(NStepExpectedSarsaAgent &&) = default;
  NStepExpectedSarsaAgent &operator=(const NStepExpectedSarsaAgent &) = delete;
  NStepExpectedSarsaAgent &operator=(NStepExpectedSarsaAgent &&) = default;
  void Reset() override;

 private:
  std::vector<State> next_states_;
  Action Policy(const State &, bool is_evaluation) override;
  template<typename T>
  void SetNextStates(T &&next_states) {
    next_states_ = std::forward<T>(next_states);
  }
  void Update(const State &update_state, const State &current_state) override;
};

class OffPolicyNStepSarsaAgent : public NStepBootstrappingAgent {
 public:
  explicit OffPolicyNStepSarsaAgent(double alpha = kDefaultAlpha,
                                    double gamma = kDefaultGamma,
                                    int n = kDefaultN,
                                    double epsilon = kDefaultEpsilon,
                                    double epsilon_decay_factor
                                    = kDefaultEpsilonDecayFactor,
                                    double min_epsilon = kDefaultMinEpsilon)
      : NStepBootstrappingAgent(alpha, gamma, n, epsilon, epsilon_decay_factor,
                                min_epsilon) {}
  OffPolicyNStepSarsaAgent(const OffPolicyNStepSarsaAgent &) = delete;
  OffPolicyNStepSarsaAgent(OffPolicyNStepSarsaAgent &&) = default;
  OffPolicyNStepSarsaAgent &
  operator=(const OffPolicyNStepSarsaAgent &) = delete;
  OffPolicyNStepSarsaAgent &operator=(OffPolicyNStepSarsaAgent &&) = default;

 private:
  void Update(const State &update_state, const State &current_state) override;
};

class OffPolicyNStepExpectedSarsaAgent : public NStepBootstrappingAgent {
 public:
  explicit OffPolicyNStepExpectedSarsaAgent(double alpha = kDefaultAlpha,
                                            double gamma = kDefaultGamma,
                                            int n = kDefaultN,
                                            double epsilon = kDefaultEpsilon,
                                            double epsilon_decay_factor
                                            = kDefaultEpsilonDecayFactor,
                                            double min_epsilon
                                            = kDefaultMinEpsilon)
      : NStepBootstrappingAgent(alpha, gamma, n, epsilon, epsilon_decay_factor,
                                min_epsilon) {}
  OffPolicyNStepExpectedSarsaAgent(
      const OffPolicyNStepExpectedSarsaAgent &) = delete;
  OffPolicyNStepExpectedSarsaAgent(
      OffPolicyNStepExpectedSarsaAgent &&) = default;
  OffPolicyNStepExpectedSarsaAgent &
  operator=(const OffPolicyNStepExpectedSarsaAgent &) = delete;
  OffPolicyNStepExpectedSarsaAgent &
  operator=(OffPolicyNStepExpectedSarsaAgent &&) = default;
  void Reset() override;

 private:
  std::vector<State> next_states_;
  Action Policy(const State &, bool is_evaluation) override;
  template<typename T>
  void SetNextStates(T &&next_states) {
    next_states_ = std::forward<T>(next_states);
  }
  void Update(const State &update_state, const State &current_state) override;
};

class NStepTreeBackupAgent : public NStepBootstrappingAgent {
 public:
  explicit NStepTreeBackupAgent(double alpha = kDefaultAlpha,
                                double gamma = kDefaultGamma,
                                int n = kDefaultN,
                                double epsilon = kDefaultEpsilon,
                                double epsilon_decay_factor
                                = kDefaultEpsilonDecayFactor,
                                double min_epsilon = kDefaultMinEpsilon)
      : NStepBootstrappingAgent(alpha, gamma, n, epsilon, epsilon_decay_factor,
                                min_epsilon) {}
  NStepTreeBackupAgent(const NStepTreeBackupAgent &) = delete;
  NStepTreeBackupAgent(NStepTreeBackupAgent &&) = default;
  NStepTreeBackupAgent &operator=(const NStepTreeBackupAgent &) = delete;
  NStepTreeBackupAgent &operator=(NStepTreeBackupAgent &&) = default;

 private:
  void Update(const State &update_state, const State &current_state) override;
};

std::ostream &operator<<(std::ostream &,
                         const std::unordered_map<State, Agent::Reward> &);

std::ostream &operator<<(std::ostream &,
                         const std::vector<RLAgent::TimeStep> &);

}  // namespace nim_rl

#endif  // NIM_RL_AGENT_H_