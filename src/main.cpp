#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <string>
#include <unordered_map>
#include <vector>

class MappedFile
{
  MappedFile(const char* file)
  {
    auto fd = open(file, O_RDONLY);

    if (fd == -1)
    {
      throw "Unable to open input file";
    }

    stat sb;
    fstat(fd, &sb);

    mem_ = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mem_ == nullptr)
    {
      throw "Unable to mmap input file";
    }
  }

  void* mem_ = nullptr;
};

class RatingsCalc
{
  public:
  RatingsCalc() = default;

  void read_games(const char* file);

  private:
  std::unordered_map<std::string, int> players_;
  std::unordered_map<int, std::string> player_names_;
  std::vector<int> scores_;
};

void RatingsCalc::read_games(const char* file)
{
}


int main(int argc, char** argv)
{
  return 0;
}
