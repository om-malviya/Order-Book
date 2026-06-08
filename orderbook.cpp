#include "orderbook.h"
#include <iostream>
#include <algorithm>

std::vector<Trade> OrderBook::add(Order order) {
    const int original_qty = order.qty;
    statuses_[order.id] = {order.id, Status::Open, original_qty, 0};

    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        while (order.qty > 0 && !asks_.empty()) {
            auto best = asks_.begin();
            if (!order.crosses(best->first)) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                int resting_id = level.front().id;
                int fill       = std::min(order.qty, level.front().qty);
                trades.push_back({next_trade_id_++, order.id, resting_id, level.price, fill, order.side});
                order.qty -= fill;
                level.decrement_front(fill);

                auto& rs = statuses_[resting_id];
                rs.filled_qty += fill;
                rs.status = (rs.filled_qty == rs.original_qty) ? Status::Filled
                                                                : Status::PartiallyFilled;
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
                int resting_id = level.front().id;
                int fill       = std::min(order.qty, level.front().qty);
                trades.push_back({next_trade_id_++, resting_id, order.id, level.price, fill, order.side});
                order.qty -= fill;
                level.decrement_front(fill);

                auto& rs = statuses_[resting_id];
                rs.filled_qty += fill;
                rs.status = (rs.filled_qty == rs.original_qty) ? Status::Filled
                                                                : Status::PartiallyFilled;
            }
            if (level.empty()) bids_.erase(best);
        }
        if (order.qty > 0 && order.rests()) {
            auto& level = asks_.try_emplace(order.price, order.price).first->second;
            level.add_order(order);
        }
    }

    auto& os      = statuses_[order.id];
    os.filled_qty = original_qty - order.qty;
    if (os.filled_qty == original_qty)
        os.status = Status::Filled;
    else if (!order.rests())
        os.status = Status::Cancelled;
    else if (os.filled_qty > 0)
        os.status = Status::PartiallyFilled;

    return trades;
}

const OrderStatus* OrderBook::status(int order_id) const {
    auto it = statuses_.find(order_id);
    return (it != statuses_.end()) ? &it->second : nullptr;
}

void OrderBook::print() const {
    std::cout << "\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it)
        std::cout << "  ASK  " << it->first << "   " << it->second.volume() << "\n";
    std::cout << "  ---\n";
    for (const auto& [price, level] : bids_)
        std::cout << "  BID  " << price << "   " << level.volume() << "\n";
    std::cout << "\n";
}
