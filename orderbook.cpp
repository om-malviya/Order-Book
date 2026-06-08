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
                trades.push_back({next_trade_id_++, order.id, resting.id, best->first, fill, order.side});
                order.qty   -= fill;
                resting.qty -= fill;
                if (resting.qty == 0) level.pop_front();
            }
            if (level.empty()) asks_.erase(best);
        }
        if (order.qty > 0 && order.rests())
            bids_[order.price].push_back(order);

    } else {
        while (order.qty > 0 && !bids_.empty()) {
            auto best = bids_.begin();
            if (!order.crosses(best->first)) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int fill = std::min(order.qty, resting.qty);
                trades.push_back({next_trade_id_++, resting.id, order.id, best->first, fill, order.side});
                order.qty   -= fill;
                resting.qty -= fill;
                if (resting.qty == 0) level.pop_front();
            }
            if (level.empty()) bids_.erase(best);
        }
        if (order.qty > 0 && order.rests())
            asks_[order.price].push_back(order);
    }

    return trades;
}

void OrderBook::print() const {
    std::cout << "\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        int total = 0;
        for (const auto& o : it->second) total += o.qty;
        std::cout << "  ASK  " << it->first << "   " << total << "\n";
    }
    std::cout << "  ---\n";
    for (const auto& [price, level] : bids_) {
        int total = 0;
        for (const auto& o : level) total += o.qty;
        std::cout << "  BID  " << price << "   " << total << "\n";
    }
    std::cout << "\n";
}
