#pragma once

#include "../common/Types.h"
#include "MatchingEngine.h"
#include <atomic>
#include <random>
#include <string>
#include <thread>
#include <vector>


namespace orderbook {

class Simulator {
public:
  Simulator();
  ~Simulator();

  void start(MatchingEngine *engine, const std::vector<std::string> &symbols,
             int intervalMs, int minOrderSize, int maxOrderSize,
             double priceVariance);

  void stop();

private:
  void simulationLoop();
  void generateRandomOrder();

  MatchingEngine *engine_;
  std::vector<std::string> symbols_;
  int intervalMs_;
  int minOrderSize_;
  int maxOrderSize_;
  double priceVariance_;

  std::thread simulatorThread_;
  std::atomic<bool> running_{false};
  std::mt19937 rng_;
};

} // namespace orderbook
