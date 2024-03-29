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
  int i;
  for (i = 0; i != 100000; ++i)
  {
    timer.start();
    double e = calculate_errors();
    if (i %50 == 0)
    {
      timer.stop("calculate_errors");
      std::cout << "K = " << adjust_state_.K << std::endl;
    }
    if (i % 100 == 0)
    {
      std::cout << "Total error = " << e << std::endl;
    }

    if (e < 0.5)
    {
      break;
    }

    //timer.start();
    adjust_ratings_driver(i, e);
    //timer.stop("adjust_ratings");
  }

  std::cout << "Done ratings in " << i << " iterations" << std::endl;

  std::cout << "Job times" << std::endl;
  std::ranges::for_each(job_times_, [](auto t) {
    std::cout << t << std::endl;
  });
}

void RatingsCalc::adjust_ratings_driver(int i, double e)
{
  adjust_state_.iteration = i;
  adjust_state_.K = next_k(adjust_state_.K, e, i);
  adjust_waiter_.run_and_wait(threads_);
}

double RatingsCalc::calculate_errors()
{
  //return calculate_errors(0, ratings_.size());
  waiter_.run_and_wait(threads_);

  auto abs = std::views::transform(errors_, [](auto e) { return std::fabs(e); });
  return std::accumulate(abs.begin(), abs.end(), 0.0);
}

void do_error_calc(double rating, int j, double& score, const std::vector<int>& opp_index, const std::vector<int>& opp_played, const std::vector<double>& ratings)
{
    auto denom = (rating + ratings[opp_index[j]]);
    auto numerator = opp_played[j] * rating;
    score += numerator / denom;
}

void RatingsCalc::calculate_errors(int start, int end)
{
  for (auto p : std::views::iota(start, end))
  [[likely]]
  {
    auto& player = player_info_[p];
    auto rating = ratings_[p];
    double score = 0;
    // add the expected score against each opponent
    auto first = game_indexes_[p];
    auto last = game_indexes_[p+1];
    auto distance = last - first;
    distance &= 0xFFFFFFFE;

    auto end = first + distance;

    auto j = first;
    for (j = first; j < end; j += 2)
    [[likely]]
    {
      do_error_calc(rating, j, score, opp_index_, opp_played_, ratings_);
      do_error_calc(rating, j+1, score, opp_index_, opp_played_, ratings_);
    }

    if (j != last)
    {
      do_error_calc(rating, j, score, opp_index_, opp_played_, ratings_);
    }

    double e = player.score() - score;
    errors_[p] = e;
  }
}

std::vector<ThreadPool::ThreadJob> RatingsCalc::create_adjust_calculation()
{
  std::vector<ThreadPool::ThreadJob> jobs;
  double players = player_info_.size();
  auto job_count = 8;
  double ratio = players / job_count;

  size_t begin = 0;
  for (int i = 1; i < job_count+1; ++i)
  {
    size_t end = ratio * i;
    std::cout << "Adjust ratings: " << begin << "--" << end << std::endl;
    jobs.push_back([begin, end, this](){
      adjust_ratings(begin, end);
    });
    begin = end;
  }

  return jobs;
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
  int job_count = player_info_.size() / 10000;
  std::cout << job_count << " jobs" << std::endl;

  auto iter = accum_games.begin();
  auto end_iter = accum_games.end();

  double interval = static_cast<double>(total_games) / job_count;

  job_times_.resize(job_count);
  for (int i = 0; i != job_count; ++i)
  {
    double next_index = interval * (i+1);

    auto job_end = std::find_if(iter, end_iter, [next_index](auto v) {
      return v > next_index;
    });

    int begin = iter - accum_games.begin();
    int end = job_end - accum_games.begin();

    std::cout << begin << "--" << end << std::endl;
    jobs.push_back([this, begin, end, i] () {
      Timer timer;
      timer.start();
      calculate_errors(begin, end);
      auto elapsed = timer.stop();
      job_times_[i] = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    });

    iter = job_end;
  }

  return jobs;
}

void RatingsCalc::adjust_ratings(size_t start, size_t end)
{
  auto K = adjust_state_.K;

  for (auto p : std::views::iota(start, end))
  {
    double e = errors_[p] / played_[p];
    ratings_[p] = ratings_[p] * std::pow(10, K * e);
  }
}

void RatingsCalc::process_line(std::string_view line)
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
    throw "Line: '" + std::string(line) + "' malformed";
  }

  char outcome = result[0];

  switch(outcome)
  {
    case 'w':
    case 'b':
    case 'd':
    break;
    default:
    throw "Invalid result for " + std::string(line);
  }

  auto w_id = insert_player(white, outcome == 'd' ? 0.5 : outcome == 'w' ? 1 : 0);
  auto b_id = insert_player(black, outcome == 'd' ? 0.5 : outcome == 'b' ? 1 : 0);

  add_match(w_id, b_id, outcome == 'd' ? 0.5 : outcome == 'w' ? 1 : -1);

  ++games_;
}

void RatingsCalc::read_games(const char* file_name)
{
  Timer timer;
  timer.start();

  file = std::make_unique<MappedFile>(file_name);

  auto length = file->length();
  auto* memory = file->memory();

  std::cout << "File is " << length << " bytes" << std::endl;

  int i = 0;

  const char* line_begin = memory;
  while (i < length)
  {
    if (i % (1024 * 1024) == 0)
    {
      std::cout << "." << std::flush;
    }


    if (memory[i] == '\n')
    {
      process_line(std::string_view(line_begin, &memory[i]));
      line_begin = &memory[i+1];
    }

    ++i;
  }

  if (line_begin < &memory[i])
  {
    process_line(std::string_view(line_begin, &memory[i]));
  }

  ratings_.resize(players_.size(), 1);
  errors_.resize(players_.size(), 0);
  game_indexes_.push_back(0);
  std::for_each(player_info_.begin(), player_info_.end(), [this](auto& info)
  {
    info.finalize(opponent_info_);
    played_.push_back(info.played());
    game_indexes_.push_back(info.first_opponent() + info.opponents());
  });

  std::ranges::for_each(opponent_info_, [this](auto& info)
  {
    auto [index, played] = info;
    opp_index_.push_back(index);
    opp_played_.push_back(played);
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

  adjust_jobs_ = create_adjust_calculation();
  adjust_waiter_.set_jobs(adjust_jobs_);
}
