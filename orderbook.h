#pragma once
#include <map>
#include <vector>
#include <unordered_map>
#include "order.h"
#include "order_status.h"
#include "trade.h"
#include "price_level.h"

class OrderBook {
public:
    std::vector<Trade>  add(Order order);
    const OrderStatus*  status(int order_id) const;
    void                print() const;

private:
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel>                        asks_;
    std::unordered_map<int, OrderStatus>                 statuses_;
    int next_trade_id_ = 1;
};
