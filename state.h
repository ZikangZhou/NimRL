//
// Created by 周梓康 on 2020/3/3.
//

#ifndef NIM_STATE_H_
#define NIM_STATE_H_

#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class State {
  friend void swap(State &, State &);
  friend bool operator==(const State &, const State &);

 public:
  typedef std::vector<unsigned>::size_type size_type;

  State() = default;

  explicit State(std::istream &);

  State(std::initializer_list<unsigned>);

  explicit State(const std::vector<unsigned> &);

  State(const State &state);

  State(State &&) noexcept;

  State &operator=(const State &);

  State &operator=(State &&) noexcept;

  State &operator=(std::initializer_list<unsigned>);

  State &operator=(const std::vector<unsigned> &);

  ~State() = default;

  void clear() { state_.clear(); }

  bool empty() const { return state_.empty(); }

  void RemoveObjects(size_type pile_id, unsigned num_objects);

  size_type size() const { return state_.size(); }

  unsigned &operator[](size_type);

  const unsigned &operator[](size_type) const;

 private:
  std::vector<unsigned> state_;

  void CheckEmpty(const std::string &msg) {
    if (state_.empty()) {
      std::cerr << msg << std::endl;
    }
  }

  void CheckRange(size_type pile_id, const std::string &msg) const {
    if (pile_id >= state_.size()) {
      throw std::out_of_range(msg);
    }
  }
};

inline void swap(State &lhs, State &rhs) {
  using std::swap;
  swap(lhs.state_, rhs.state_);
}

std::istream &operator>>(std::istream &, State &);

std::ostream &operator<<(std::ostream &, const State &);

bool operator==(const State &, const State &);

bool operator!=(const State &, const State &);

#endif  // NIM_STATE_H_