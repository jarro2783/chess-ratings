#include "mapped_file.h"
#include "ratings.h"
#include "timer.h"

#include "threads/threads.h"

#include <fstream>
#include <ranges>

namespace
{

double next_k(double previous, double error, int iteration)
{
  if (error < 1)
  {
    return 1.1;
  }

  if (iteration % 21 == 0)
  {
    return 33;
  }

  return 1.6;
}

}

void RatingsCalc::find_ratings()
{
  Timer timer;
  double K = 50;
  int i;
  for (i = 0; i != 100000; ++i)
  {
    timer.start();
    double e = calculate_errors();
    if (i %50 == 0)
    {
      timer.stop("calculate_errors");
      std::cout << "K = " << K << std::endl;
    }
    if (i % 100 == 0)
    {
      std::cout << "Total error = " << e << std::endl;
    }

    if (e < 0.1)
    {
      break;
    }

    //timer.start();
    adjust_ratings(K);
    K = next_k(K, e, i);
    //timer.stop("adjust_ratings");
  }

  std::cout << "Done ratings in " << i << " iterations" << std::endl;

}

double RatingsCalc::calculate_errors()
{
  //return calculate_errors(0, ratings_.size());
  waiter_.run_and_wait(threads_);

  auto abs = std::views::transform(errors_, [](auto e) { return std::fabs(e); });
  return std::accumulate(abs.begin(), abs.end(), 0.0);
}

void RatingsCalc::calculate_errors(int start, int end)
{
  for (auto p : std::views::iota(start, end))
  {
    auto& player = player_info_[p];
    auto rating = ratings_[p];
    //double rating_inv = 1/rating;
    double score = 0;
    // add the expected score against each opponent
    auto first = player.first_opponent();
    auto last = first + player.opponents();
    for (int j = first; j != last; ++j)
    {
      const auto& opponent = opponent_info_[j];
      score +=  std::get<1>(opponent) * rating / (rating + ratings_[std::get<0>(opponent)]);
    }

    double e = player.score() - score;
    errors_[p] = e;
  }
}

std::vector<ThreadPool::ThreadJob> RatingsCalc::create_error_calculation()
{
  //add up the pairings to split by pairings instead of players
  std::vector<int> accum_games;
  int games = 0;

  for (size_t i = 0; i != player_info_.size(); ++i)
  {
    games += player_info_[i].played();
    accum_games.push_back(games);
  }

  auto total_games = accum_games.back();

  std::vector<ThreadPool::ThreadJob> jobs;
  int cpus = threads_.pool_size();
  std::cout << cpus << " jobs" << std::endl;

  auto iter = accum_games.begin();
  auto end_iter = accum_games.end();

  double interval = static_cast<double>(total_games) / cpus;
  int start_index = 0;

  for (int i = 0; i != cpus; ++i)
  {
    int next_index = start_index + interval;

    auto job_end = std::find_if(iter, end_iter, [next_index](auto v) {
      return v > next_index;
    });

    int begin = iter - accum_games.begin();
    int end = job_end - accum_games.begin();

    std::cout << begin << "--" << end << std::endl;
    jobs.push_back([this, begin, end]() {
      calculate_errors(begin, end);
    });

    iter = job_end;
    start_index = next_index;
  }

  return jobs;
}

void RatingsCalc::adjust_ratings(double K)
{
  for (auto p : std::views::iota(0u, player_info_.size()))
  {
    auto e = errors_[p] / player_info_[p].played();
    ratings_[p] = ratings_[p] * std::pow(10, K * e);
  }
}

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
  std::for_each(player_info_.begin(), player_info_.end(), [this](auto& info)
  {
    info.finalize(opponent_info_);
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

  init_jobs();
}

void RatingsCalc::print_ratings(const char* file)
{
  std::vector<std::tuple<double, double, std::string>> ratings;

  for (auto p : std::views::iota(0u, ratings_.size()))
  {
    ratings.emplace_back(ratings_[p], errors_[p], player_names_[p]);
  }

  std::sort(ratings.begin(), ratings.end(), [](const auto& a, const auto& b) {
    return std::get<0>(b) < std::get<0>(a);
  });

  std::ofstream out(file);

  for (const auto& [r, e, n] : ratings)
  {
    out << n << ": " << std::log10(r) * 400 + 1500 << ", " << e <<  std::endl;
  }
}

RatingsCalc::RatingsCalc() {}

void RatingsCalc::init_jobs()
{
  error_jobs_ = create_error_calculation();
  waiter_.set_jobs(error_jobs_);
}
