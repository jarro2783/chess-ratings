#pragma once

#include <string>
#include <vector>
#include <absl/container/flat_hash_map.h>

#include "player.h"
#include "threads/threads.h"
#include "threads/waiter.h"

class RatingsCalc
{
  public:
  RatingsCalc();

  void read_games(const char* file);
  void find_ratings();

  void print_ratings(const char* file);

  private:

  //<index, played vs, score>
  using Opponent = std::tuple<int, int, double>;

  absl::flat_hash_map<std::string, int> players_;
  absl::flat_hash_map<int, std::string> player_names_;
  std::vector<Player> player_info_;
  std::vector<Opponent> opponent_info_;
  std::vector<double> errors_;
  int next_player_ = 0;
  int games_ = 0;

  std::vector<double> ratings_;

  ThreadPool threads_;
  std::vector<ThreadPool::ThreadJob> error_jobs_;
  ThreadPoolWaiter waiter_;

  void process_line(const std::string& line);
  double calculate_errors();
  void calculate_errors(int start, int end);
  void adjust_ratings(double K);
  std::vector<ThreadPool::ThreadJob> create_error_calculation();
  void init_jobs();

  int insert_player(std::string player, double score)
  {
    auto inserted = players_.emplace(player, next_player_);
    if (inserted.second)
    {
      player_names_.emplace(next_player_, std::move(player));
      ++next_player_;
    }
    
    add_score(inserted.first->second, score);

    return inserted.first->second;
  }

  void add_score(size_t player, double score)
  {
    if (player_info_.size() <= player)
    {
      player_info_.resize(player+1);
    }

    player_info_[player].add_score(score);
  }

  void add_match(int white, int black, double score)
  {
    auto& w_info = player_info_[white];
    auto& b_info = player_info_[black];

    w_info.add_matchup(black, score >= 0 ? score : 0);
    b_info.add_matchup(white, score == 0.5 ? 0.5 : score < 0 ? -score : 0);
  }
};


