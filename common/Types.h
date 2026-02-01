#pragma once

#include "OrderEntry.pb.h" // For Side and OrderType enums
#include <cstdint>
#include <string>


namespace orderbook {

// Note: Side and OrderType enums are now defined in OrderEntry.proto

// Represents a single order in the book
struct Order {
  uint64_t id;
  std::string symbol;
  Side side;
  OrderType type;
  double price;
  uint32_t quantity;
  uint32_t remainingQuantity;
  uint64_t timestamp; // For FIFO priority
};

// Represents a trade that occurred
struct Trade {
  uint64_t makerOrderId;
  uint64_t takerOrderId;
  std::string symbol;
  double price;
  uint32_t quantity;
  Side takerSide;
};

// Result of adding an order
struct OrderResult {
  bool success;
  std::string errorMessage;
  uint64_t orderId;
};

} // namespace orderbook
