#include "orderbook.h"
#include <iostream>
#include <algorithm>

std::vector<Trade> OrderBook::add(Order order) {
    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        while (order.qty > 0 && !asks_.empty()) {
            auto best = asks_.begin();
            if (!order.crosses(best->first)) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int fill = std::min(order.qty, resting.qty);
                trades.push_back({next_trade_id_++, order.id, resting.id, level.price, fill, order.side});
                order.qty -= fill;
                level.decrement_front(fill);
            }
            if (level.empty()) asks_.erase(best);
        }
        if (order.qty > 0 && order.rests()) {
            auto& level = bids_.try_emplace(order.price, order.price).first->second;
            level.add_order(order);
        }

    } else {
        while (order.qty > 0 && !bids_.empty()) {
            auto best = bids_.begin();
            if (!order.crosses(best->first)) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int fill = std::min(order.qty, resting.qty);
                trades.push_back({next_trade_id_++, resting.id, order.id, level.price, fill, order.side});
                order.qty -= fill;
                level.decrement_front(fill);
            }
            if (level.empty()) bids_.erase(best);
        }
        if (order.qty > 0 && order.rests()) {
            auto& level = asks_.try_emplace(order.price, order.price).first->second;
            level.add_order(order);
        }
    }

    return trades;
}

void OrderBook::print() const {
    std::cout << "\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        std::cout << "  ASK  " << it->first << "   " << it->second.volume() << "\n";
    }
    std::cout << "  ---\n";
    for (const auto& [price, level] : bids_) {
        std::cout << "  BID  " << price << "   " << level.volume() << "\n";
    }
    std::cout << "\n";
}
