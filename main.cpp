#include <iostream>
#include "orderbook.h"

int main() {
    OrderBook book;
    int       next_id = 1;

    auto status_str = [](Status s) -> const char* {
        switch (s) {
            case Status::Open:            return "Open";
            case Status::PartiallyFilled: return "PartiallyFilled";
            case Status::Filled:          return "Filled";
            case Status::Cancelled:       return "Cancelled";
        }
        return "";
    };

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
            std::cout << "  -> TRADE"
                      << "  id="     << t.id
                      << "  buyer="  << t.buyer_id
                      << "  seller=" << t.seller_id
                      << "  px="     << t.price
                      << "  qty="    << t.qty
                      << "  agg="    << (t.aggressor == Side::Buy ? "BUY" : "SELL")
                      << "\n";
            filled += t.qty;
        }
        if (!o.rests() && filled < qty)
            std::cout << "  -> CANCELLED  residual qty=" << (qty - filled) << "\n";

        if (const auto* s = book.status(o.id))
            std::cout << "  -> STATUS   " << status_str(s->status)
                      << "  filled=" << s->filled_qty << "/" << s->original_qty << "\n";
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
