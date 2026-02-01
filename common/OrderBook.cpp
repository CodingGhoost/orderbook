#include "OrderBook.h"
#include <algorithm>
#include <stdexcept>

namespace orderbook {

Limit::Limit(double price) : price_(price) {}

void Limit::addOrder(const Order &order) { orders_.push_back(order); }

bool Limit::removeOrder(uint64_t orderId) {
  auto it = std::find_if(orders_.begin(), orders_.end(),
                         [orderId](const Order &o) { return o.id == orderId; });

  if (it != orders_.end()) {
    orders_.erase(it);
    return true;
  }
  return false;
}

uint32_t Limit::getTotalQuantity() const {
  uint32_t total = 0;
  for (const auto &order : orders_) {
    total += order.remainingQuantity;
  }
  return total;
}

bool Limit::isEmpty() const { return orders_.empty(); }

// ============================================================================
// OrderBook Implementation
// ============================================================================

OrderBook::OrderBook(const std::string &symbol) : symbol_(symbol) {}

std::vector<Trade> OrderBook::addOrder(const Order &order) {
  std::vector<Trade> trades;
  Order workingOrder = order;

  if (workingOrder.side == Side::BUY) {
    // Match against asks (lowest price first)
    auto it = asks_.begin();
    while (it != asks_.end() && workingOrder.remainingQuantity > 0) {
      double askPrice = it->first;

      // Check if order can match at this price
      bool canMatch = (workingOrder.type == OrderType::MARKET) ||
                      (workingOrder.price >= askPrice);

      if (!canMatch)
        break;

      auto &limit = it->second;
      auto &limitOrders = limit->getOrders();

      // Match against orders at this price level (FIFO)
      auto orderIt = limitOrders.begin();
      while (orderIt != limitOrders.end() &&
             workingOrder.remainingQuantity > 0) {
        uint32_t matchQty = std::min(workingOrder.remainingQuantity,
                                     orderIt->remainingQuantity);

        // Create trade
        Trade trade;
        trade.makerOrderId = orderIt->id;
        trade.takerOrderId = workingOrder.id;
        trade.symbol = symbol_;
        trade.price = askPrice; // Trade at maker's price
        trade.quantity = matchQty;
        trade.takerSide = Side::BUY;
        trades.push_back(trade);

        // Update quantities
        workingOrder.remainingQuantity -= matchQty;
        orderIt->remainingQuantity -= matchQty;

        // Remove fully filled orders
        if (orderIt->remainingQuantity == 0) {
          orderIt = limitOrders.erase(orderIt);
        } else {
          ++orderIt;
        }
      }

      // Remove empty price levels
      if (limit->isEmpty()) {
        it = asks_.erase(it);
      } else {
        ++it;
      }
    }
  } else { // Side::SELL
    // Match against bids (highest price first)
    auto it = bids_.begin();
    while (it != bids_.end() && workingOrder.remainingQuantity > 0) {
      double bidPrice = it->first;

      // Check if order can match at this price
      bool canMatch = (workingOrder.type == OrderType::MARKET) ||
                      (workingOrder.price <= bidPrice);

      if (!canMatch)
        break;

      auto &limit = it->second;
      auto &limitOrders = limit->getOrders();

      // Match against orders at this price level (FIFO)
      auto orderIt = limitOrders.begin();
      while (orderIt != limitOrders.end() &&
             workingOrder.remainingQuantity > 0) {
        uint32_t matchQty = std::min(workingOrder.remainingQuantity,
                                     orderIt->remainingQuantity);

        // Create trade
        Trade trade;
        trade.makerOrderId = orderIt->id;
        trade.takerOrderId = workingOrder.id;
        trade.symbol = symbol_;
        trade.price = bidPrice; // Trade at maker's price
        trade.quantity = matchQty;
        trade.takerSide = Side::SELL;
        trades.push_back(trade);

        // Update quantities
        workingOrder.remainingQuantity -= matchQty;
        orderIt->remainingQuantity -= matchQty;

        // Remove fully filled orders
        if (orderIt->remainingQuantity == 0) {
          orderIt = limitOrders.erase(orderIt);
        } else {
          ++orderIt;
        }
      }

      // Remove empty price levels
      if (limit->isEmpty()) {
        it = bids_.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Add remaining quantity to the book (if limit order)
  if (workingOrder.remainingQuantity > 0 &&
      workingOrder.type == OrderType::LIMIT) {
    if (workingOrder.side == Side::BUY) {
      auto &limit = bids_[workingOrder.price];
      if (!limit) {
        limit = std::make_unique<Limit>(workingOrder.price);
      }
      limit->addOrder(workingOrder);
    } else {
      auto &limit = asks_[workingOrder.price];
      if (!limit) {
        limit = std::make_unique<Limit>(workingOrder.price);
      }
      limit->addOrder(workingOrder);
    }
  }

  return trades;
}

bool OrderBook::cancelOrder(uint64_t orderId, Side side) {
  // TODO: Implement order lookup map for O(1) cancellation
  // For now, linear search through all price levels

  if (side == Side::BUY) {
    for (auto it = bids_.begin(); it != bids_.end(); ++it) {
      if (it->second->removeOrder(orderId)) {
        // Remove empty price levels
        if (it->second->isEmpty()) {
          bids_.erase(it);
        }
        return true;
      }
    }
  } else {
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
      if (it->second->removeOrder(orderId)) {
        // Remove empty price levels
        if (it->second->isEmpty()) {
          asks_.erase(it);
        }
        return true;
      }
    }
  }

  return false;
}

double OrderBook::getBestBid() const {
  if (bids_.empty())
    return 0.0;
  return bids_.begin()->first;
}

double OrderBook::getBestAsk() const {
  if (asks_.empty())
    return 0.0;
  return asks_.begin()->first;
}

uint32_t OrderBook::getBestBidSize() const {
  if (bids_.empty())
    return 0;
  return bids_.begin()->second->getTotalQuantity();
}

uint32_t OrderBook::getBestAskSize() const {
  if (asks_.empty())
    return 0;
  return asks_.begin()->second->getTotalQuantity();
}

} // namespace orderbook
