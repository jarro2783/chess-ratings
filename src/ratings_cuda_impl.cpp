#include "player.h"
#include "ratings_cuda.h"

// This is here because nvcc doesn't like abseil

RatingsCuda::RatingsCuda(const std::vector<Player>& player_info,
    const std::vector<std::tuple<int, int>>& opponents)
: players_(player_info.size())
{
  errors_.resize(players_);

  std::ranges::for_each(opponents, [this](auto& opp)
  {
    opp_index.push_back(std::get<0>(opp));
    opp_played.push_back(std::get<1>(opp));
  });

  int current_index = 0;
  std::ranges::for_each(player_info, [this, &current_index](auto& p)
  {
    scores.push_back(p.score());
    indices_.push_back(current_index);
    played_.push_back(p.played());
    current_index += p.opponents();
  });

  indices_.push_back(current_index);

  ratings_.resize(players_, 1);
}
