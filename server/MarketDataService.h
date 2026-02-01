#pragma once

#include "../common/IMatchingEngine.h"
#include "MarketData.grpc.pb.h"
#include "MarketData.pb.h"
#include <grpcpp/grpcpp.h>
#include <map>
#include <mutex>
#include <vector>

using grpc::Server;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

namespace orderbook {

// Separate the gRPC service from the listener
class MarketDataServiceImpl final : public IMarketDataListener {
public:
  MarketDataServiceImpl() = default;

  // IMarketDataListener interface
  void onTrade(const Trade &trade) override;
  void onBookUpdate(const std::string &symbol, double bestBid, double bestAsk,
                    uint32_t bidSize, uint32_t askSize) override;

  // gRPC service method (called manually from a wrapper)
  Status subscribe(ServerContext *context, const SubscribeRequest *request,
                   ServerWriter<MarketUpdate> *writer);

private:
  struct Subscriber {
    ServerWriter<MarketUpdate> *writer;
    ServerContext *context;
  };

  std::mutex mutex_;
  std::map<std::string, std::vector<Subscriber>> subscribers_;
  uint64_t nextSeq_ = 1;
};

} // namespace orderbook
