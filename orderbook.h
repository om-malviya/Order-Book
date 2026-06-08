#pragma once
#include <map>
#include <deque>
#include <vector>
#include "order.h"
#include "trade.h"

class OrderBook {
public:
    std::vector<Trade> add(Order order);
    void print() const;

private:
    std::map<int64_t, std::deque<Order>, std::greater<int64_t>> bids_;
    std::map<int64_t, std::deque<Order>>                        asks_;
    int next_trade_id_ = 1;
};
