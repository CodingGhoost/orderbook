#pragma once

#include "../common/IMatchingEngine.h"
#include "../common/OrderBook.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>


namespace orderbook {

class MatchingEngine : public IMatchingEngine {
public:
  MatchingEngine();
  ~MatchingEngine() override = default;

  // IMatchingEngine interface
  OrderResult addOrder(const Order &order) override;
  bool cancelOrder(uint64_t orderId) override;
  void addMarketDataListener(IMarketDataListener *listener) override;
  void removeMarketDataListener(IMarketDataListener *listener) override;

  // Additional methods for initialization
  void addSymbol(const std::string &symbol);
  OrderBook *getOrderBook(const std::string &symbol);

private:
  void notifyTrade(const Trade &trade);
  void notifyBookUpdate(const std::string &symbol);

  std::unordered_map<std::string, std::unique_ptr<OrderBook>> books_;
  std::vector<IMarketDataListener *> listeners_;
  std::mutex mutex_; // Protects books_ and listeners_
  std::atomic<uint64_t> nextOrderId_{1};

  // Track orders for cancellation (orderId -> symbol, side)
  std::unordered_map<uint64_t, std::pair<std::string, Side>> orderLookup_;
};

} // namespace orderbook
