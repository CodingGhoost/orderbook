#pragma once

#include "Types.h"
#include <list>
#include <map>
#include <memory>
#include <vector>


namespace orderbook {

// Represents a single price level (a "Limit")
// Contains all orders at this price in FIFO order
class Limit {
public:
  Limit(double price);

  // Add an order to this price level
  void addOrder(const Order &order);

  // Remove an order by ID
  bool removeOrder(uint64_t orderId);

  // Get total quantity at this level
  uint32_t getTotalQuantity() const;

  // Check if this level is empty
  bool isEmpty() const;

  // Get the price of this level
  double getPrice() const { return price_; }

  // Get all orders (for matching)
  std::list<Order> &getOrders() { return orders_; }
  const std::list<Order> &getOrders() const { return orders_; }

private:
  double price_;
  std::list<Order> orders_; // FIFO queue of orders at this price
};

// The main order book for a single symbol
// Maintains sorted lists of bids and asks
class OrderBook {
public:
  explicit OrderBook(const std::string &symbol);

  // Add an order and return any trades that occur
  std::vector<Trade> addOrder(const Order &order);

  // Cancel an order
  bool cancelOrder(uint64_t orderId, Side side);

  // Get best bid price (highest buy price)
  double getBestBid() const;

  // Get best ask price (lowest sell price)
  double getBestAsk() const;

  // Get total quantity at best bid
  uint32_t getBestBidSize() const;

  // Get total quantity at best ask
  uint32_t getBestAskSize() const;

  // Get the symbol this book is for
  const std::string &getSymbol() const { return symbol_; }

private:
  std::string symbol_;

  // Bids: price -> Limit (sorted descending by price)
  std::map<double, std::unique_ptr<Limit>, std::greater<double>> bids_;

  // Asks: price -> Limit (sorted ascending by price)
  std::map<double, std::unique_ptr<Limit>> asks_;

  // Tracking orderId -> (price, side) for fast cancellation
  // std::unordered_map<uint64_t, std::pair<double, Side>> orderLookup_;
};

} // namespace orderbook
