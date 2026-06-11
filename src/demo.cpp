#include "demo.h"
#include "orderbook.h"
#include "cli.h"

void demo::run() {
    OrderBook book;
    int       next_id = 1;

    auto submit = [&](Side side, int64_t price, int qty, Type type = Type::Limit) {
        Order o{next_id++, side, price, qty, type};
        cli::order_submitted(o);
        for (const auto& t : book.add(o))
            cli::trade(t);
        if (const auto* s = book.status(o.id))
            cli::order_status(*s);
    };

    submit(Side::Buy,  10000, 10);
    submit(Side::Buy,  10000,  5);
    submit(Side::Buy,   9900,  8);
    submit(Side::Sell, 10100,  3);
    book.print();

    submit(Side::Sell, 10000, 20);
    book.print();

    submit(Side::Buy,  10100,  6);
    book.print();

    submit(Side::Buy,   9900,  5);
    submit(Side::Sell, 10200,  4);
    book.print();

    submit(Side::Buy,  0, 10, Type::Market);
    book.print();

    submit(Side::Sell, 0, 20, Type::Market);
    book.print();
}
