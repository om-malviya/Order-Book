#pragma once
#include <deque>
#include <cstdint>
#include "order.h"

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
