#pragma once

#include <string>
#include <vector>

namespace orderbook {

struct PriceLevel {
  double price;
  uint32_t quantity;
};

struct InitialDepth {
  std::vector<PriceLevel> bids;
  std::vector<PriceLevel> asks;
};

struct SymbolConfig {
  std::string symbol;
  uint32_t maxDepthLevels;
  InitialDepth initialDepth;
};

struct SimulationConfig {
  bool enabled;
  int intervalMs;
  int minOrderSize;
  int maxOrderSize;
  double priceVariance;
};

struct Config {
  std::vector<SymbolConfig> symbols;
  SimulationConfig simulation;

  static Config loadFromFile(const std::string &filename);
};

} // namespace orderbook
