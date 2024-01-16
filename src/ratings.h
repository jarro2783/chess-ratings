#pragma once

#include <string>
#include <vector>
#include <absl/container/flat_hash_map.h>

#include "player.h"
#include "threads/threads.h"
#include "threads/waiter.h"

class MappedFile;

class RatingsCalc
{
  public:
  RatingsCalc();

  void read_games(const char* file);
  void find_ratings();

  void print_ratings(const char* file);

  private:

  //<index, played vs>
  using Opponent = std::tuple<size_t, size_t>;

  absl::flat_hash_map<std::string_view, int> players_;
  absl::flat_hash_map<int, std::string_view> player_names_;
  std::vector<Player> player_info_;
  std::vector<Opponent> opponent_info_;
  std::vector<int> game_indexes_;
  std::vector<int> opp_index_;
  std::vector<int> opp_played_;
  std::vector<double> errors_;
  std::vector<int> played_;
  int next_player_ = 0;
  int games_ = 0;

  std::vector<double> ratings_;

  ThreadPool threads_;
  std::vector<ThreadPool::ThreadJob> error_jobs_;
  std::vector<ThreadPool::ThreadJob> adjust_jobs_;
  ThreadPoolWaiter waiter_;
  ThreadPoolWaiter adjust_waiter_;

  void process_line(std::string_view line);
  double calculate_errors();
  void adjust_ratings_driver(int i, double e);
  void calculate_errors(int start, int end);
  void adjust_ratings(size_t start, size_t end);
  std::vector<ThreadPool::ThreadJob> create_error_calculation();
  std::vector<ThreadPool::ThreadJob> create_adjust_calculation();
  void init_jobs();

  int insert_player(std::string_view player, double score)
  {
    auto inserted = players_.try_emplace(player, next_player_);
    if (inserted.second)
    {
      player_names_.emplace(next_player_, player);
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

  std::unique_ptr<MappedFile> file;

  struct {
    int iteration = 0;
    double K = 1.6; 
  } adjust_state_;
};


