#include "MarketData.grpc.pb.h"
#include "MarketData.pb.h"
#include "OrderEntry.grpc.pb.h"
#include "OrderEntry.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class OrderBookClient {
public:
  OrderBookClient(std::shared_ptr<Channel> channel)
      : orderStub_(orderbook::OrderEntry::NewStub(channel)),
        mdStub_(orderbook::MarketData::NewStub(channel)) {}

  void subscribe(const std::string &symbol) {
    std::thread([this, symbol]() {
      ClientContext context;
      orderbook::SubscribeRequest request;
      request.set_symbol(symbol);

      orderbook::MarketUpdate update;
      auto reader = mdStub_->Subscribe(&context, request);

      std::cout << "Subscribed to " << symbol << std::endl;

      while (reader->Read(&update)) {
        std::lock_guard<std::mutex> lock(printMutex_);

        if (update.has_book_update()) {
          const auto &book = update.book_update();
          std::cout << "[" << symbol << "] "
                    << "Bid: " << book.best_bid() << " (" << book.bid_size()
                    << ") | "
                    << "Ask: " << book.best_ask() << " (" << book.ask_size()
                    << ")" << std::endl;
        } else if (update.has_trade_update()) {
          const auto &trade = update.trade_update();
          std::string side =
              (trade.taker_side() == orderbook::BUY) ? "BUY" : "SELL";
          std::cout << "[" << symbol << "] TRADE: " << trade.quantity() << " @ "
                    << trade.price() << " (Taker: " << side << ")" << std::endl;
        }
      }

      Status status = reader->Finish();
      if (!status.ok()) {
        std::cout << "Subscription ended: " << status.error_message()
                  << std::endl;
      }
    }).detach();
  }

  void placeOrder(const std::string &symbol, orderbook::Side side,
                  orderbook::OrderType type, double price, uint32_t quantity) {
    ClientContext context;
    orderbook::PlaceOrderRequest request;
    request.set_symbol(symbol);
    request.set_side(side);
    request.set_type(type);
    request.set_price(price);
    request.set_quantity(quantity);

    orderbook::PlaceOrderResponse response;
    Status status = orderStub_->PlaceOrder(&context, request, &response);

    if (status.ok()) {
      if (response.success()) {
        std::cout << "Order placed: ID=" << response.order_id() << std::endl;
      } else {
        std::cout << "Order failed: " << response.error_message() << std::endl;
      }
    } else {
      std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
  }

  void cancelOrder(uint64_t orderId) {
    ClientContext context;
    orderbook::CancelOrderRequest request;
    request.set_order_id(orderId);

    orderbook::CancelOrderResponse response;
    Status status = orderStub_->CancelOrder(&context, request, &response);

    if (status.ok()) {
      if (response.success()) {
        std::cout << "Order cancelled: ID=" << orderId << std::endl;
      } else {
        std::cout << "Cancel failed: " << response.error_message() << std::endl;
      }
    } else {
      std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
  }

private:
  std::unique_ptr<orderbook::OrderEntry::Stub> orderStub_;
  std::unique_ptr<orderbook::MarketData::Stub> mdStub_;
  std::mutex printMutex_;
};

void printHelp() {
  std::cout
      << "\nAvailable commands:\n"
      << "  subscribe <symbol>                  - Subscribe to market data\n"
      << "  buy <symbol> <price> <quantity>     - Place limit buy order\n"
      << "  sell <symbol> <price> <quantity>    - Place limit sell order\n"
      << "  market_buy <symbol> <quantity>      - Place market buy order\n"
      << "  market_sell <symbol> <quantity>     - Place market sell order\n"
      << "  cancel <order_id>                   - Cancel an order\n"
      << "  help                                - Show this help\n"
      << "  exit                                - Quit\n"
      << std::endl;
}

int main(int argc, char **argv) {
  std::string serverAddress = "localhost:50051";
  if (argc > 1) {
    serverAddress = argv[1];
  }

  auto channel =
      grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
  OrderBookClient client(channel);

  std::cout << "Connected to " << serverAddress << std::endl;
  printHelp();

  std::string line;
  std::cout << "> " << std::flush;

  while (std::getline(std::cin, line)) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "subscribe") {
      std::string symbol;
      iss >> symbol;
      if (!symbol.empty()) {
        client.subscribe(symbol);
      } else {
        std::cout << "Usage: subscribe <symbol>" << std::endl;
      }
    } else if (command == "buy") {
      std::string symbol;
      double price;
      uint32_t quantity;
      iss >> symbol >> price >> quantity;
      if (!symbol.empty() && price > 0 && quantity > 0) {
        client.placeOrder(symbol, orderbook::BUY, orderbook::LIMIT, price,
                          quantity);
      } else {
        std::cout << "Usage: buy <symbol> <price> <quantity>" << std::endl;
      }
    } else if (command == "sell") {
      std::string symbol;
      double price;
      uint32_t quantity;
      iss >> symbol >> price >> quantity;
      if (!symbol.empty() && price > 0 && quantity > 0) {
        client.placeOrder(symbol, orderbook::SELL, orderbook::LIMIT, price,
                          quantity);
      } else {
        std::cout << "Usage: sell <symbol> <price> <quantity>" << std::endl;
      }
    } else if (command == "market_buy") {
      std::string symbol;
      uint32_t quantity;
      iss >> symbol >> quantity;
      if (!symbol.empty() && quantity > 0) {
        client.placeOrder(symbol, orderbook::BUY, orderbook::MARKET, 0.0,
                          quantity);
      } else {
        std::cout << "Usage: market_buy <symbol> <quantity>" << std::endl;
      }
    } else if (command == "market_sell") {
      std::string symbol;
      uint32_t quantity;
      iss >> symbol >> quantity;
      if (!symbol.empty() && quantity > 0) {
        client.placeOrder(symbol, orderbook::SELL, orderbook::MARKET, 0.0,
                          quantity);
      } else {
        std::cout << "Usage: market_sell <symbol> <quantity>" << std::endl;
      }
    } else if (command == "cancel") {
      uint64_t orderId;
      iss >> orderId;
      if (orderId > 0) {
        client.cancelOrder(orderId);
      } else {
        std::cout << "Usage: cancel <order_id>" << std::endl;
      }
    } else if (command == "help") {
      printHelp();
    } else if (command == "exit") {
      break;
    } else if (!command.empty()) {
      std::cout << "Unknown command. Type 'help' for available commands."
                << std::endl;
    }

    std::cout << "> " << std::flush;
  }

  std::cout << "Goodbye!" << std::endl;
  return 0;
}
