#pragma once

#include <tuple>
#include <vector>

#include <absl/container/flat_hash_map.h>

class Player
{
  public:
  void add_score(double score)
  {
    total_score_ += score;
    played_ += 1;
  }

  void add_matchup(int other, double score)
  {
    auto& m = accum_matchups_[other];
    m.first += 1;
    m.second += score;
  }

  void finalize(std::vector<std::tuple<size_t, size_t>>& global_matchups)
  {
    std::for_each(accum_matchups_.begin(), accum_matchups_.end(), [this](auto& scores)
    {
      matchups_.emplace_back(scores.first, scores.second.first);
    });

    std::sort(matchups_.begin(), matchups_.end(), [](auto& l, auto& r)
    {
      return std::get<0>(l) < std::get<0>(r);
    });

    opponent_start_ = global_matchups.size();
    opponents_ = matchups_.size();

    global_matchups.insert(global_matchups.end(), matchups_.begin(), matchups_.end());
  }

  const auto& matchups() const
  {
    return matchups_;
  }

  double score() const
  {
    return total_score_;
  }

  int played() const
  {
    return played_;
  }

  int first_opponent() const
  {
    return opponent_start_;
  }

  int opponents() const
  {
    return opponents_;
  }

  private:
  double total_score_ = 0;
  double played_ = 0;

  absl::flat_hash_map<size_t, std::pair<size_t, double>> accum_matchups_;
  std::vector<std::tuple<size_t, size_t>> matchups_;

  int opponent_start_ = 0;
  int opponents_ = 0;
};
