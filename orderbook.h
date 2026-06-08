#pragma once
#include <map>
#include <deque>
#include <vector>
#include <unordered_map>
#include "order.h"
#include "trade.h"

struct PriceLevel {
    int64_t           price;
    std::deque<Order> orders;
    int               total_volume = 0;

    explicit PriceLevel(int64_t p) : price(p) {}

    void add_order(const Order& o) {
        orders.push_back(o);
        total_volume += o.qty;
    }

    void decrement_front(int amount) {
        orders.front().qty -= amount;
        total_volume -= amount;
        if (orders.front().qty == 0) orders.pop_front();
    }

    bool empty() const { return orders.empty(); }

    Order& front() { return orders.front(); }

    int volume() const { return total_volume; }
};

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
