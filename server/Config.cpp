#include "Config.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>


using json = nlohmann::json;

namespace orderbook {

Config Config::loadFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open config file: " + filename);
  }

  json j;
  file >> j;

  Config config;

  // Parse symbols
  for (const auto &symbolJson : j["symbols"]) {
    SymbolConfig symConfig;
    symConfig.symbol = symbolJson["symbol"];
    symConfig.maxDepthLevels = symbolJson.value("maxDepthLevels", 10);

    // Parse bids
    for (const auto &bidJson : symbolJson["initialDepth"]["bids"]) {
      PriceLevel level;
      level.price = bidJson["price"];
      level.quantity = bidJson["quantity"];
      symConfig.initialDepth.bids.push_back(level);
    }

    // Parse asks
    for (const auto &askJson : symbolJson["initialDepth"]["asks"]) {
      PriceLevel level;
      level.price = askJson["price"];
      level.quantity = askJson["quantity"];
      symConfig.initialDepth.asks.push_back(level);
    }

    config.symbols.push_back(symConfig);
  }

  // Parse simulation config
  if (j.contains("simulation")) {
    config.simulation.enabled = j["simulation"].value("enabled", true);
    config.simulation.intervalMs = j["simulation"].value("intervalMs", 1000);

    auto sizeRange =
        j["simulation"].value("orderSizeRange", std::vector<int>{10, 100});
    config.simulation.minOrderSize = sizeRange[0];
    config.simulation.maxOrderSize = sizeRange[1];

    config.simulation.priceVariance =
        j["simulation"].value("priceVariance", 0.5);
  }

  return config;
}

} // namespace orderbook
