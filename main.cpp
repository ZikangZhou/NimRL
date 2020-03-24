#include "game.h"

int main() {
  Game game(State{10, 10, 10});
  OptimalAgent optimal_agent;
  RandomAgent random_agent;
  QLearningAgent ql_agent1(0.99, 1.0, 1.0, 0.9);
  QLearningAgent ql_agent2(0.99, 1.0, 1.0, 0.9);
  SarsaAgent sarsa_agent1(0.5, 1.0, 1.0, 0.9);
  SarsaAgent sarsa_agent2(0.5, 1.0, 1.0, 0.9);
  ExpectedSarsaAgent expected_sarsa_agent1(0.5, 1.0, 1.0, 0.9);
  ExpectedSarsaAgent expected_sarsa_agent2(0.5, 1.0, 1.0, 0.9);
  DoubleQLearningAgent double_ql_agent1(0.5, 1.0, 1.0, 0.9);
  DoubleQLearningAgent double_ql_agent2(0.5, 1.0, 1.0, 0.9);

  game.SetFirstPlayer(&ql_agent1);
  game.SetSecondPlayer(&ql_agent2);
  game.Train(20000);
  std::cout << ql_agent1.GetQValues() << std::endl;
  ql_agent1.SetEpsilon(0.0);
  game.SetSecondPlayer(&optimal_agent);
  game.Play(10000);

  game.SetFirstPlayer(&sarsa_agent1);
  game.SetSecondPlayer(&sarsa_agent2);
  game.Train(50000);
  std::cout << sarsa_agent1.GetQValues() << std::endl;
  sarsa_agent1.SetEpsilon(0.0);
  game.SetSecondPlayer(&optimal_agent);
  game.Play(10000);

  game.SetFirstPlayer(&expected_sarsa_agent1);
  game.SetSecondPlayer(&expected_sarsa_agent2);
  game.Train(50000);
  std::cout << expected_sarsa_agent1.GetQValues() << std::endl;
  expected_sarsa_agent1.SetEpsilon(0.0);
  game.SetSecondPlayer(&optimal_agent);
  game.Play(10000);

  game.SetFirstPlayer(&double_ql_agent1);
  game.SetSecondPlayer(&double_ql_agent2);
  game.Train(50000);
  std::cout << double_ql_agent1.GetQValues() << std::endl;
  double_ql_agent1.SetEpsilon(0.0);
  game.SetSecondPlayer(&optimal_agent);
  game.Play(10000);
  return 0;
}
