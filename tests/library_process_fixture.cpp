#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::puts("KytyPS5 process fixture\nversion 1");
    return 0;
  }
  const char *mode = "";
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::strcmp(argv[i], "--user-name") == 0)
      mode = argv[i + 1];
    if (std::strcmp(argv[i], "--game") == 0)
      std::printf("Game path: %s\n", argv[i + 1]);
  }
  std::printf("\033[31mstdout ");
  std::fwrite("\xE2", 1, 1, stdout);
  std::fflush(stdout);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::printf("\x82\xAC\033[0m\n");
  std::fflush(stdout);
  std::fprintf(stderr, "stderr captured\n");
  std::fflush(stderr);
  if (std::strcmp(mode, "slow") == 0) {
    for (;;)
      std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::printf("final partial line");
  std::fflush(stdout);
  return std::strcmp(mode, "fail") == 0 ? 7 : 0;
}
