#include "OrderEntryService.h"

namespace orderbook {

OrderEntryServiceImpl::OrderEntryServiceImpl(MatchingEngine *engine)
    : engine_(engine) {}

Status OrderEntryServiceImpl::PlaceOrder(ServerContext *context,
                                         const PlaceOrderRequest *request,
                                         PlaceOrderResponse *response) {
  Order order;
  order.symbol = request->symbol();
  order.side = (request->side() == orderbook::BUY) ? Side::BUY : Side::SELL;
  order.type = (request->type() == orderbook::LIMIT) ? OrderType::LIMIT
                                                     : OrderType::MARKET;
  order.price = request->price();
  order.quantity = request->quantity();
  order.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

  auto result = engine_->addOrder(order);

  response->set_success(result.success);
  response->set_order_id(result.orderId);
  response->set_error_message(result.errorMessage);

  return Status::OK;
}

Status OrderEntryServiceImpl::CancelOrder(ServerContext *context,
                                          const CancelOrderRequest *request,
                                          CancelOrderResponse *response) {
  bool success = engine_->cancelOrder(request->order_id());

  response->set_success(success);
  if (!success) {
    response->set_error_message("Order not found");
  }

  return Status::OK;
}

} // namespace orderbook
