#include "Config.h"
#include "MarketDataGrpcService.h"
#include "MarketDataService.h"
#include "MatchingEngine.h"
#include "OrderEntryService.h"
#include "Simulator.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char **argv) {
  std::string configFile = "config/orderbooks.json";
  if (argc > 1) {
    configFile = argv[1];
  }

  std::cout << "Loading config from: " << configFile << std::endl;

  // Load configuration
  orderbook::Config config;
  try {
    config = orderbook::Config::loadFromFile(configFile);
  } catch (const std::exception &e) {
    std::cerr << "Failed to load config: " << e.what() << std::endl;
    return 1;
  }

  // Create matching engine
  auto engine = std::make_unique<orderbook::MatchingEngine>();

  // Initialize order books from config
  for (const auto &symConfig : config.symbols) {
    std::cout << "Initializing " << symConfig.symbol << std::endl;
    engine->addSymbol(symConfig.symbol);

    auto *book = engine->getOrderBook(symConfig.symbol);

    // Add initial bids
    for (const auto &bid : symConfig.initialDepth.bids) {
      orderbook::Order order;
      order.symbol = symConfig.symbol;
      order.side = orderbook::Side::BUY;
      order.type = orderbook::OrderType::LIMIT;
      order.price = bid.price;
      order.quantity = bid.quantity;
      order.timestamp = 0;
      engine->addOrder(order);
    }

    // Add initial asks
    for (const auto &ask : symConfig.initialDepth.asks) {
      orderbook::Order order;
      order.symbol = symConfig.symbol;
      order.side = orderbook::Side::SELL;
      order.type = orderbook::OrderType::LIMIT;
      order.price = ask.price;
      order.quantity = ask.quantity;
      order.timestamp = 0;
      engine->addOrder(order);
    }

    std::cout << "  Best Bid: " << book->getBestBid()
              << " | Best Ask: " << book->getBestAsk() << std::endl;
  }

  // Create gRPC services
  orderbook::OrderEntryServiceImpl orderEntryService(engine.get());
  orderbook::MarketDataServiceImpl marketDataService;
  orderbook::MarketDataGrpcService marketDataGrpcService(&marketDataService);

  // Register market data listener
  engine->addMarketDataListener(&marketDataService);

  // Start simulator if enabled
  std::unique_ptr<orderbook::Simulator> simulator;
  if (config.simulation.enabled) {
    std::cout << "Starting simulator..." << std::endl;
    simulator = std::make_unique<orderbook::Simulator>();

    std::vector<std::string> symbols;
    for (const auto &sym : config.symbols) {
      symbols.push_back(sym.symbol);
    }

    simulator->start(engine.get(), symbols, config.simulation.intervalMs,
                     config.simulation.minOrderSize,
                     config.simulation.maxOrderSize,
                     config.simulation.priceVariance);
  }

  // Build and start gRPC server
  std::string serverAddress("0.0.0.0:50051");
  ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&orderEntryService);
  builder.RegisterService(&marketDataGrpcService);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << serverAddress << std::endl;

  server->Wait();

  return 0;
}
