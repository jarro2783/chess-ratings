#include <cmath>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <fstream>

#include <absl/container/flat_hash_map.h>

class Timer
{
  public:
  void start()
  {
    start_ = clock_type::now();
  }

  void stop(const char* prefix)
  {
    auto end = clock_type::now();
    std::cout << prefix << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - start_) << std::endl;
  }

  private:
  using clock_type = std::chrono::high_resolution_clock;
  using time_point = std::chrono::time_point<clock_type>;
  time_point start_;
};

class MappedFile
{
  public:
  MappedFile(const char* file)
  {
    fd_ = open(file, O_RDONLY);

    if (fd_ == -1)
    {
      throw "Unable to open input file";
    }

    struct stat sb;
    fstat(fd_, &sb);

    length_ = sb.st_size;

    mem_ = mmap(nullptr, length_, PROT_READ, MAP_PRIVATE, fd_, 0);

    if (mem_ == nullptr)
    {
      throw "Unable to mmap input file";
    }
  }

  ~MappedFile()
  {
    munmap(mem_, length_);
    close(fd_);
  }

  int length()
  {
    return length_;
  }

  const char* memory()
  {
    return reinterpret_cast<const char*>(mem_);
  }

  private:

  int fd_ = 0;
  void* mem_ = nullptr;
  int length_ = 0;
};

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

class RatingsCalc
{
  public:
  RatingsCalc() = default;

  void read_games(const char* file);
  void find_ratings();

  void print_ratings(const char* file);

  private:
  absl::flat_hash_map<std::string, int> players_;
  absl::flat_hash_map<int, std::string> player_names_;
  std::vector<Player> player_info_;
  std::vector<double> errors_;
  int next_player_ = 0;
  int games_ = 0;

  std::vector<double> ratings_;

  void process_line(const std::string& line);
  double calculate_errors();
  void adjust_ratings(double K);

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

  void add_score(int player, double score)
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

void RatingsCalc::process_line(const std::string& line)
{
  std::string_view white;
  std::string_view black;
  std::string_view result;

  int i = 0;

  using std::operator""sv;
  for (auto part : std::views::split(line, ":"sv))
  { 
    switch(i)
    {
      case 0:
        white = std::string_view(part.begin(), part.end());
        break;
      case 1:
        black = std::string_view(part.begin(), part.end());
        break;
      case 2:
        result = std::string_view(part.begin(), part.end());
        break;
    }
    ++i;
  }
  
  if (i != 3)
  {
    throw "Line: '" + line + "' malformed";
  }

  char outcome = result[0];

  switch(outcome)
  {
    case 'w':
    case 'b':
    case 'd':
    break;
    default:
    throw "Invalid result for " + line;
  }

  auto w_id = insert_player(std::string(white), outcome == 'd' ? 0.5 : outcome == 'w' ? 1 : 0);
  auto b_id = insert_player(std::string(black), outcome == 'd' ? 0.5 : outcome == 'b' ? 1 : 0);

  add_match(w_id, b_id, outcome == 'd' ? 0.5 : outcome == 'w' ? 1 : -1);

  ++games_;
}

namespace
{

double next_k(double previous, int iterations)
{
  if (previous > 2 && iterations < 10)
  {
    return previous / 2;
  }

  if (iterations % 23 == 0)
  {
    return 31;
  }

  return 1.6;
}

}

void RatingsCalc::find_ratings()
{
  Timer timer;
  double K = 50;
  for (int i = 0; i != 10000; ++i)
  {
    timer.start();
    double e = calculate_errors();
    if (i == 0)
    {
      timer.stop("calculate_errors");
    }
    if (i % 100 == 0)
    {
      std::cout << "Total error = " << e << std::endl;
    }

    if (e < 1.0)
    {
      break;
    }

    K = next_k(K, i);
    timer.start();
    adjust_ratings(K);
    timer.stop("adjust_ratings");
  }

  std::cout << "Done ratings" << std::endl;

}

double RatingsCalc::calculate_errors()
{
  double total = 0;
  for (auto p : std::views::iota(0u, player_info_.size()))
  {
    auto& player = player_info_[p];
    auto rating = ratings_[p];
    double rating_inv = 1/rating;
    double score = 0;
    // add the expected score against each opponent
    for (auto& opponent : player.matchups())
    {
      score +=  std::get<1>(opponent) / (1 + (ratings_[std::get<0>(opponent)] * rating_inv));
      //score = std::get<1>(opponent) / (1 + std::pow(10, (ratings_[std::get<0>(opponent)] - rating)/400));
    }

    double e = player.score() - score;
    total += std::abs(e);
    errors_[p] = e;
  }

  return total;
}

void RatingsCalc::adjust_ratings(double K)
{
  for (auto p : std::views::iota(0u, player_info_.size()))
  {
    auto e = errors_[p] / player_info_[p].played();
    ratings_[p] = ratings_[p] * std::pow(10, K * e);
  }
}

void RatingsCalc::read_games(const char* file_name)
{
  Timer timer;
  timer.start();

  MappedFile file(file_name);

  auto length = file.length();
  auto* memory = file.memory();

  std::cout << "File is " << length << " bytes" << std::endl;

  int i = 0;

  std::string line;
  while (i != length)
  {
    char current = memory[i];

    if (current == '\n')
    {
      process_line(line);
      line.clear();
    }
    else
    {
      line += current;
    }

    ++i;
  }

  if (!line.empty())
  {
    process_line(line);
  }

  ratings_.resize(players_.size(), 1);
  errors_.resize(players_.size(), 0);
  std::for_each(player_info_.begin(), player_info_.end(), [](auto& info)
  {
    info.finalize();
  });

  timer.stop("read_games");

  std::cout << players_.size() << " players" << std::endl;
  std::cout << games_ << " games" << std::endl;

  #if 0
  std::ofstream out("playerinfo.txt");
  out << "Players" << std::endl;
  for (auto& i : player_names_)
  {
    out << i.first << ": " << i.second << std::endl;
  }

  out << "Player matches" << std::endl;
  int p_num = 0;
  for (auto& p : player_info_)
  {
    out << p_num << " played " << p.played() << " scored " << p.score() << std::endl;
    for (auto& [player, games, score] : p.matchups())
    {
      out << "    played " << player << " " << games << " " << score << std::endl;
    }
    ++p_num;
  }
  #endif
}

void RatingsCalc::print_ratings(const char* file)
{
  std::vector<std::tuple<double, std::string>> ratings;

  for (auto p : std::views::iota(0u, ratings_.size()))
  {
    ratings.emplace_back(ratings_[p], player_names_[p]);
  }

  std::sort(ratings.begin(), ratings.end(), [](const auto& a, const auto& b) {
    return std::get<0>(b) < std::get<0>(a);
  });

  std::ofstream out(file);

  for (const auto& [r, n] : ratings)
  {
    out << n << ": " << std::log10(r) * 400 + 1500 << std::endl;
  }
}

int main(int argc, const char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Provide input file" << std::endl;
    return 1;
  }


  try
  {
    RatingsCalc calc;
    calc.read_games(argv[1]);

    Timer timer;
    timer.start();
    calc.find_ratings();
    timer.stop("find_ratings");
    calc.print_ratings("ratings-out.txt");
  } catch(const std::string& e)
  {
    std::cerr << "Exception caught " << e << std::endl;
    return 1;
  }

  return 0;
}
