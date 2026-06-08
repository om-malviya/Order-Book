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
};

struct Trade {
    int     buyer_id;
    int     seller_id;
    int64_t price;
    int     qty;
};

struct Book {
    std::map<int64_t, std::deque<Order>, std::greater<int64_t>> bids;
    std::map<int64_t, std::deque<Order>>                        asks;
};

std::vector<Trade> match(Book& book, Order order) {
    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        while (order.qty > 0 && !book.asks.empty()) {
            auto best = book.asks.begin();
            if (order.type == Type::Limit && order.price < best->first) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int fill = std::min(order.qty, resting.qty);
                trades.push_back({order.id, resting.id, best->first, fill});
                order.qty   -= fill;
                resting.qty -= fill;
                if (resting.qty == 0) level.pop_front();
            }
            if (level.empty()) book.asks.erase(best);
        }
        if (order.qty > 0 && order.type == Type::Limit)
            book.bids[order.price].push_back(order);

    } else {
        while (order.qty > 0 && !book.bids.empty()) {
            auto best = book.bids.begin();
            if (order.type == Type::Limit && order.price > best->first) break;

            auto& level = best->second;
            while (order.qty > 0 && !level.empty()) {
                Order& resting = level.front();
                int fill = std::min(order.qty, resting.qty);
                trades.push_back({resting.id, order.id, best->first, fill});
                order.qty   -= fill;
                resting.qty -= fill;
                if (resting.qty == 0) level.pop_front();
            }
            if (level.empty()) book.bids.erase(best);
        }
        if (order.qty > 0 && order.type == Type::Limit)
            book.asks[order.price].push_back(order);
    }

    return trades;
}

void print_book(const Book& book) {
    std::cout << "\n";
    for (auto it = book.asks.rbegin(); it != book.asks.rend(); ++it) {
        int total = 0;
        for (const auto& o : it->second) total += o.qty;
        std::cout << "  ASK  " << it->first << "   " << total << "\n";
    }
    std::cout << "  ---\n";
    for (const auto& [price, level] : book.bids) {
        int total = 0;
        for (const auto& o : level) total += o.qty;
        std::cout << "  BID  " << price << "   " << total << "\n";
    }
    std::cout << "\n";
}

int main() {
    Book book;
    int  next_id = 1;

    auto submit = [&](Side side, int64_t price, int qty, Type type = Type::Limit) {
        Order o{next_id++, side, price, qty, type};
        std::cout << (side == Side::Buy ? "BUY " : "SELL");
        if (type == Type::Market)
            std::cout << "  id=" << o.id << "  px=MKT  qty=" << qty << "\n";
        else
            std::cout << "  id=" << o.id << "  px=" << price << "  qty=" << qty << "\n";

        auto trades = match(book, o);
        int filled = 0;
        for (const auto& t : trades) {
            std::cout << "  -> TRADE  buyer=" << t.buyer_id
                      << "  seller="          << t.seller_id
                      << "  px="              << t.price
                      << "  qty="             << t.qty << "\n";
            filled += t.qty;
        }
        if (type == Type::Market && filled < qty)
            std::cout << "  -> CANCELLED  residual qty=" << (qty - filled) << "\n";
    };

    submit(Side::Buy,  100, 10);
    submit(Side::Buy,  100,  5);   // same price as id=1, FIFO test
    submit(Side::Buy,   99,  8);
    submit(Side::Sell, 101,  3);
    print_book(book);

    submit(Side::Sell, 100, 20);   // sweeps both 100 bids, rests remainder
    print_book(book);

    submit(Side::Buy,  101,  6);   // crosses two ask levels
    print_book(book);

    // market order scenarios
    submit(Side::Buy,   99,  5);   // resting bid
    submit(Side::Sell, 102,  4);   // resting ask
    print_book(book);

    submit(Side::Buy,  0, 10, Type::Market);   // sweeps all asks
    print_book(book);

    submit(Side::Sell, 0, 20, Type::Market);   // sweeps all bids, partial fill + cancel
    print_book(book);

    return 0;
}
