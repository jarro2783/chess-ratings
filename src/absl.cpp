#include <string_view>
#include <absl/container/flat_hash_map.h>

int main(int argc, char** argv)
{
  absl::flat_hash_map<std::string_view, int> m;

  const char* a = "abc";
  char b[4]{};

  std::copy(a, a+4, b);

  m.emplace(std::string_view(a), 0);
  m.emplace(std::string_view(b), 1);

  std::cout << m.size() << std::endl;
}
