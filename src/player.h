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

  void finalize()
  {
    std::for_each(accum_matchups_.begin(), accum_matchups_.end(), [this](auto& scores)
    {
      matchups_.emplace_back(scores.first, scores.second.first, scores.second.second);
    });

    std::sort(matchups_.begin(), matchups_.end(), [](auto& l, auto& r)
    {
      return std::get<0>(l) < std::get<0>(r);
    });

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

  private:
  double total_score_ = 0;
  double played_ = 0;

  absl::flat_hash_map<int, std::pair<int, double>> accum_matchups_;
  std::vector<std::tuple<int, int, double>> matchups_;
};
