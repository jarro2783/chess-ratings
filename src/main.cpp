#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

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

void RatingsCalc::read_games(const char* file_name)
{
  MappedFile file(file_name);
}

int main(int argc, const char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Provide input file" << std::endl;
    return 1;
  }

  RatingsCalc calc;
  calc.read_games(argv[1]);
  return 0;
}
