#include <iostream>
#include <map>
#include <deque>
#include <vector>
#include <cstdint>
#include <algorithm>

enum class Side { Buy, Sell };
enum class Type { Limit, Market };

struct Order {
    int      id;
    Side     side;
    int64_t  price;
    int      qty;
    Type     type = Type::Limit;

    bool crosses(int64_t opposite_price) const {
        if (type == Type::Market) return true;
        return (side == Side::Buy) ? price >= opposite_price
                                   : price <= opposite_price;
    }

    bool rests() const { return type == Type::Limit; }
};

struct Trade {
    int     buyer_id;
    int     seller_id;
    int64_t price;
    int     qty;
};

class OrderBook {
public:
    std::vector<Trade> add(Order order) {
        std::vector<Trade> trades;

        if (order.side == Side::Buy) {
            while (order.qty > 0 && !asks_.empty()) {
                auto best = asks_.begin();
                if (!order.crosses(best->first)) break;

                auto& level = best->second;
                while (order.qty > 0 && !level.empty()) {
                    Order& resting = level.front();
                    int fill = std::min(order.qty, resting.qty);
                    trades.push_back({order.id, resting.id, best->first, fill});
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
                    trades.push_back({resting.id, order.id, best->first, fill});
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

    void print() const {
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

private:
    std::map<int64_t, std::deque<Order>, std::greater<int64_t>> bids_;
    std::map<int64_t, std::deque<Order>>                        asks_;
};

int main() {
    OrderBook book;
    int       next_id = 1;

    auto submit = [&](Side side, int64_t price, int qty, Type type = Type::Limit) {
        Order o{next_id++, side, price, qty, type};
        std::cout << (side == Side::Buy ? "BUY " : "SELL");
        if (type == Type::Market)
            std::cout << "  id=" << o.id << "  px=MKT  qty=" << qty << "\n";
        else
            std::cout << "  id=" << o.id << "  px=" << price << "  qty=" << qty << "\n";

        auto trades = book.add(o);
        int filled = 0;
        for (const auto& t : trades) {
            std::cout << "  -> TRADE  buyer=" << t.buyer_id
                      << "  seller="          << t.seller_id
                      << "  px="              << t.price
                      << "  qty="             << t.qty << "\n";
            filled += t.qty;
        }
        if (!o.rests() && filled < qty)
            std::cout << "  -> CANCELLED  residual qty=" << (qty - filled) << "\n";
    };

    submit(Side::Buy,  100, 10);
    submit(Side::Buy,  100,  5);
    submit(Side::Buy,   99,  8);
    submit(Side::Sell, 101,  3);
    book.print();

    submit(Side::Sell, 100, 20);
    book.print();

    submit(Side::Buy,  101,  6);
    book.print();

    submit(Side::Buy,   99,  5);
    submit(Side::Sell, 102,  4);
    book.print();

    submit(Side::Buy,  0, 10, Type::Market);
    book.print();

    submit(Side::Sell, 0, 20, Type::Market);
    book.print();

    return 0;
}
