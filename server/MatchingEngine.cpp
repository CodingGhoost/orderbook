#include "MatchingEngine.h"
#include <stdexcept>

namespace orderbook {

MatchingEngine::MatchingEngine() {}

void MatchingEngine::addSymbol(const std::string &symbol) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (books_.find(symbol) == books_.end()) {
    books_[symbol] = std::make_unique<OrderBook>(symbol);
  }
}

OrderBook *MatchingEngine::getOrderBook(const std::string &symbol) {
  auto it = books_.find(symbol);
  if (it != books_.end()) {
    return it->second.get();
  }
  return nullptr;
}

OrderResult MatchingEngine::addOrder(const Order &order) {
  std::lock_guard<std::mutex> lock(mutex_);

  OrderResult result;
  result.success = false;
  result.orderId = 0;

  // Check if symbol exists
  auto it = books_.find(order.symbol);
  if (it == books_.end()) {
    result.errorMessage = "Symbol not found: " + order.symbol;
    return result;
  }

  // Assign order ID
  Order workingOrder = order;
  workingOrder.id = nextOrderId_++;
  workingOrder.remainingQuantity = workingOrder.quantity;

  // Track order for cancellation
  orderLookup_[workingOrder.id] = {order.symbol, order.side};

  // Execute order
  auto trades = it->second->addOrder(workingOrder);

  // Notify listeners of trades
  for (const auto &trade : trades) {
    notifyTrade(trade);
  }

  // Notify book update
  notifyBookUpdate(order.symbol);

  result.success = true;
  result.orderId = workingOrder.id;
  return result;
}

bool MatchingEngine::cancelOrder(uint64_t orderId) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = orderLookup_.find(orderId);
  if (it == orderLookup_.end()) {
    return false; // Order not found
  }

  const auto &[symbol, side] = it->second;

  auto bookIt = books_.find(symbol);
  if (bookIt == books_.end()) {
    return false;
  }

  bool cancelled = bookIt->second->cancelOrder(orderId, side);

  if (cancelled) {
    orderLookup_.erase(it);
    notifyBookUpdate(symbol);
  }

  return cancelled;
}

void MatchingEngine::addMarketDataListener(IMarketDataListener *listener) {
  std::lock_guard<std::mutex> lock(mutex_);
  listeners_.push_back(listener);
}

void MatchingEngine::removeMarketDataListener(IMarketDataListener *listener) {
  std::lock_guard<std::mutex> lock(mutex_);
  listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener),
                   listeners_.end());
}

void MatchingEngine::notifyTrade(const Trade &trade) {
  // Note: mutex_ is already locked by caller
  for (auto *listener : listeners_) {
    listener->onTrade(trade);
  }
}

void MatchingEngine::notifyBookUpdate(const std::string &symbol) {
  // Note: mutex_ is already locked by caller
  auto it = books_.find(symbol);
  if (it == books_.end())
    return;

  auto *book = it->second.get();
  double bestBid = book->getBestBid();
  double bestAsk = book->getBestAsk();
  uint32_t bidSize = book->getBestBidSize();
  uint32_t askSize = book->getBestAskSize();

  for (auto *listener : listeners_) {
    listener->onBookUpdate(symbol, bestBid, bestAsk, bidSize, askSize);
  }
}

} // namespace orderbook
