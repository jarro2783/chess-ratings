#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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


