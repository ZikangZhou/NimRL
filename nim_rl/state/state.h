// Copyright 2020 Zhou Zikang. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NIM_RL_STATE_STATE_H_
#define NIM_RL_STATE_STATE_H_

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "nim_rl/action/action.h"

namespace nim_rl {

class State {
  friend void swap(State &, State &);
  friend bool operator==(const State &, const State &);
  friend class std::hash<State>;

 public:
  using size_type = std::vector<unsigned>::size_type;
  State() = default;
  State(size_type n, unsigned val) : data_(n, val) {}
  State(std::initializer_list<unsigned> il) : data_(il) {}
  explicit State(std::istream &);
  explicit State(std::vector<unsigned> vec) : data_(std::move(vec)) {}
  State(const State &) = default;
  State(State &&state) noexcept : data_(std::move(state.data_)) {}
  State &operator=(const State &) = default;
  State &operator=(State &&) noexcept;
  State &operator=(std::initializer_list<unsigned>);
  State &operator=(std::vector<unsigned>);
  ~State() = default;
  void ApplyAction(const Action &);
  State Child(const Action &) const;
  std::vector<State> Children() const;
  void Clear() { data_.clear(); }
  std::vector<State> GetAllStates() const;
  bool IsEmpty() const { return data_.empty(); }
  bool IsTerminal() const;
  std::vector<Action> LegalActions() const;
  unsigned NimSum() const;
  bool OutOfRange(int pile_id) const {
    return pile_id >= data_.size() || pile_id < 0;
  }
  State Parent(const Action &) const;
  size_type Size() const { return data_.size(); }
  std::string ToString() const;
  void UndoAction(const Action &);
  unsigned &operator[](int);
  const unsigned &operator[](int) const;

 private:
  std::vector<unsigned> data_;
  void CheckRange(int pile_id,
                  const std::string &msg = "Pile_id is out of range.") const;
  void DoGetAllStates(const State &state, int pile_id,
                      std::vector<State> *all_states) const;
};

std::istream &operator>>(std::istream &, State &);
std::ostream &operator<<(std::ostream &, const State &);
bool operator==(const State &, const State &);
bool operator!=(const State &, const State &);

inline void State::CheckRange(int pile_id, const std::string &msg) const {
  if (OutOfRange(pile_id)) throw std::out_of_range(msg);
}

inline void swap(State &lhs, State &rhs) {
  using std::swap;
  swap(lhs.data_, rhs.data_);
}

}  // namespace nim_rl

namespace std {
using nim_rl::State;
template<>
struct hash<State> {
  std::size_t operator()(const State &state) const {
    std::vector<unsigned> state_sorted(state.data_);
    std::sort(state_sorted.begin(), state_sorted.end());
    std::size_t seed = 0;
    for (int pile_id = 0; pile_id != state_sorted.size(); ++pile_id) {
      seed ^= std::hash<unsigned>()(state_sorted[pile_id]) + 0x9e3779b9
          + (seed << 6u) + (seed >> 2u);
    }
    return seed;
  }
};
}  // namespace std

#endif  // NIM_RL_STATE_STATE_H_
