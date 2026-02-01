#pragma once

#include "MatchingEngine.h"
#include "OrderEntry.grpc.pb.h"
#include "OrderEntry.pb.h"
#include <grpcpp/grpcpp.h>


using grpc::Server;
using grpc::ServerContext;
using grpc::Status;

namespace orderbook {

class OrderEntryServiceImpl final : public orderbook::OrderEntry::Service {
public:
  explicit OrderEntryServiceImpl(MatchingEngine *engine);

  Status PlaceOrder(ServerContext *context, const PlaceOrderRequest *request,
                    PlaceOrderResponse *response) override;

  Status CancelOrder(ServerContext *context, const CancelOrderRequest *request,
                     CancelOrderResponse *response) override;

private:
  MatchingEngine *engine_;
};

} // namespace orderbook
