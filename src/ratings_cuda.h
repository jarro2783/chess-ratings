#include <vector>

class Player;

class RatingsCuda
{
  public:
  RatingsCuda(const std::vector<Player>& player_info,
    const std::vector<std::tuple<int, int>>& opponents);

  void find_ratings();

  const std::vector<double>& ratings() const { return ratings_; }
  const std::vector<double>& errors() const { return errors_; }

  private:

  int players_ = 0;

  std::vector<int> opp_played;
  std::vector<int> opp_index;
  std::vector<double> scores;
  std::vector<double> ratings_;
  std::vector<int> indices_;
  std::vector<int> played_;
  std::vector<double> errors_;

  double* d_ratings;
  double* d_scores;
  double* d_errors;
  int* d_indices;
  int* d_opp_played;
  int* d_opp_index;
  int* d_played;
};
