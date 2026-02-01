#pragma once

#include "MarketData.grpc.pb.h"
#include "MarketData.pb.h"
#include "MarketDataService.h"
#include <grpcpp/grpcpp.h>


using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

namespace orderbook {

// gRPC service wrapper that delegates to MarketDataServiceImpl
class MarketDataGrpcService final : public orderbook::MarketData::Service {
public:
  explicit MarketDataGrpcService(MarketDataServiceImpl *impl) : impl_(impl) {}

  Status Subscribe(ServerContext *context, const SubscribeRequest *request,
                   ServerWriter<MarketUpdate> *writer) override {
    return impl_->subscribe(context, request, writer);
  }

private:
  MarketDataServiceImpl *impl_;
};

} // namespace orderbook
