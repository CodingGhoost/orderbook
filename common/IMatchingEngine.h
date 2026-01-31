#pragma once

#include "Types.h"
#include <functional>
#include <vector>

namespace orderbook {

// Interface for receiving updates from the matching engine
// This follows the Observer pattern - the gRPC server will implement this
class IMarketDataListener {
public:
  virtual ~IMarketDataListener() = default;

  // Called when a trade occurs
  virtual void onTrade(const Trade &trade) = 0;

  // Called when the best bid/ask changes
  virtual void onBookUpdate(const std::string &symbol, double bestBid,
                            double bestAsk, uint32_t bidSize,
                            uint32_t askSize) = 0;
};

// Main interface for the matching engine
// This is what you'll implement - the actual matching logic
class IMatchingEngine {
public:
  virtual ~IMatchingEngine() = default;

  // Add a new order to the book
  // Returns: OrderResult with success status and orderId
  virtual OrderResult addOrder(const Order &order) = 0;

  // Cancel an existing order
  virtual bool cancelOrder(uint64_t orderId) = 0;

  // Register a listener to receive market data updates
  virtual void addMarketDataListener(IMarketDataListener *listener) = 0;

  // Remove a listener
  virtual void removeMarketDataListener(IMarketDataListener *listener) = 0;
};

} // namespace orderbook
