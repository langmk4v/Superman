#pragma once

#include <vector>

namespace superman {
  struct SourceCode;

  class Application {
    std::vector<SourceCode> inputs;

  public:
    Application();

    ~Application();

    int main(int argc, char** argv);

  private:
  };
} // namespace superman