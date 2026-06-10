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
}
