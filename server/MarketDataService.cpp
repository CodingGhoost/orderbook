#include "MarketDataService.h"
#include <chrono>
#include <thread>

namespace orderbook {

Status MarketDataServiceImpl::subscribe(ServerContext *context,
                                        const SubscribeRequest *request,
                                        ServerWriter<MarketUpdate> *writer) {
  std::string symbol = request->symbol();

  // Register subscriber
  {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[symbol].push_back({writer, context});
  }

  // Keep connection alive until client disconnects
  while (!context->IsCancelled()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Cleanup on disconnect
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &subs = subscribers_[symbol];
    subs.erase(std::remove_if(subs.begin(), subs.end(),
                              [writer](const Subscriber &s) {
                                return s.writer == writer;
                              }),
               subs.end());
  }

  return Status::OK;
}

void MarketDataServiceImpl::onTrade(const Trade &trade) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = subscribers_.find(trade.symbol);
  if (it == subscribers_.end())
    return;

  MarketUpdate update;
  update.set_seq(nextSeq_++);
  update.set_symbol(trade.symbol);

  auto *tradeUpdate = update.mutable_trade_update();
  tradeUpdate->set_price(trade.price);
  tradeUpdate->set_quantity(trade.quantity);
  tradeUpdate->set_taker_side(trade.takerSide == Side::BUY ? orderbook::BUY
                                                           : orderbook::SELL);

  // Send to all subscribers
  for (auto &sub : it->second) {
    if (!sub.context->IsCancelled()) {
      sub.writer->Write(update);
    }
  }
}

void MarketDataServiceImpl::onBookUpdate(const std::string &symbol,
                                         double bestBid, double bestAsk,
                                         uint32_t bidSize, uint32_t askSize) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = subscribers_.find(symbol);
  if (it == subscribers_.end())
    return;

  MarketUpdate update;
  update.set_seq(nextSeq_++);
  update.set_symbol(symbol);

  auto *bookUpdate = update.mutable_book_update();
  bookUpdate->set_best_bid(bestBid);
  bookUpdate->set_best_ask(bestAsk);
  bookUpdate->set_bid_size(bidSize);
  bookUpdate->set_ask_size(askSize);

  // Send to all subscribers
  for (auto &sub : it->second) {
    if (!sub.context->IsCancelled()) {
      sub.writer->Write(update);
    }
  }
}

} // namespace orderbook
