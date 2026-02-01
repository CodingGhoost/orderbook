#include "Simulator.h"
#include <chrono>
#include <iostream>

namespace orderbook {

Simulator::Simulator() : rng_(std::random_device{}()) {}

Simulator::~Simulator() { stop(); }

void Simulator::start(MatchingEngine *engine,
                      const std::vector<std::string> &symbols, int intervalMs,
                      int minOrderSize, int maxOrderSize,
                      double priceVariance) {
  if (running_)
    return;

  engine_ = engine;
  symbols_ = symbols;
  intervalMs_ = intervalMs;
  minOrderSize_ = minOrderSize;
  maxOrderSize_ = maxOrderSize;
  priceVariance_ = priceVariance;

  running_ = true;
  simulatorThread_ = std::thread(&Simulator::simulationLoop, this);
}

void Simulator::stop() {
  if (running_) {
    running_ = false;
    if (simulatorThread_.joinable()) {
      simulatorThread_.join();
    }
  }
}

void Simulator::simulationLoop() {
  while (running_) {
    generateRandomOrder();
    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
  }
}

void Simulator::generateRandomOrder() {
  if (symbols_.empty())
    return;

  // Pick random symbol
  std::uniform_int_distribution<size_t> symbolDist(0, symbols_.size() - 1);
  const std::string &symbol = symbols_[symbolDist(rng_)];

  // Get current book state
  auto *book = engine_->getOrderBook(symbol);
  if (!book)
    return;

  double bestBid = book->getBestBid();
  double bestAsk = book->getBestAsk();

  if (bestBid == 0.0 || bestAsk == 0.0)
    return;

  // Generate random order
  Order order;
  order.symbol = symbol;

  // 50% chance buy, 50% sell
  std::uniform_int_distribution<int> sideDist(0, 1);
  order.side = (sideDist(rng_) == 0) ? Side::BUY : Side::SELL;

  // 80% limit, 20% market
  std::uniform_int_distribution<int> typeDist(0, 9);
  order.type = (typeDist(rng_) < 8) ? OrderType::LIMIT : OrderType::MARKET;

  // Random quantity
  std::uniform_int_distribution<int> qtyDist(minOrderSize_, maxOrderSize_);
  order.quantity = qtyDist(rng_);

  // Price with variance
  if (order.type == OrderType::LIMIT) {
    std::uniform_real_distribution<double> priceDist(-priceVariance_,
                                                     priceVariance_);
    if (order.side == Side::BUY) {
      order.price = bestBid + priceDist(rng_);
    } else {
      order.price = bestAsk + priceDist(rng_);
    }

    // Ensure price is positive
    if (order.price <= 0)
      order.price = (order.side == Side::BUY) ? bestBid : bestAsk;
  } else {
    order.price = 0.0; // Market orders don't need price
  }

  order.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

  // Submit order
  auto result = engine_->addOrder(order);

  if (result.success) {
    std::cout << "[SIMULATOR] " << (order.side == Side::BUY ? "BUY" : "SELL")
              << " " << symbol << " " << order.quantity << " @ "
              << (order.type == OrderType::LIMIT ? std::to_string(order.price)
                                                 : "MARKET")
              << " -> Order ID: " << result.orderId << std::endl;
  }
}

} // namespace orderbook
