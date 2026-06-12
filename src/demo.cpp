#include "demo.h"
#include "orderbook.h"
#include "cli.h"
#include <iostream>

void demo::run() {
    OrderBook          book;
    int                next_id = 1;
    std::vector<Trade> all_trades;

    auto submit = [&](Side side, int64_t price, int qty, Type type = Type::Limit) {
        Order o{next_id++, side, price, qty, type};
        cli::order_submitted(o);
        for (const auto& t : book.add(o)) {
            cli::trade(t);
            all_trades.push_back(t);
        }
        if (const auto* s = book.status(o.id))
            cli::order_status(*s);
    };

    // --- 1. Build a two-sided resting book ---
    std::cout << "\n--- Building resting depth ---\n";
    submit(Side::Buy,  10000, 10);
    submit(Side::Buy,  10000,  5);
    submit(Side::Buy,   9900,  8);
    submit(Side::Sell, 10100,  3);
    book.print();

    // --- 2. Aggressive sell crosses the bid; partial fills, remainder rests ---
    std::cout << "--- Sell 20 at 100.00 crosses the 100.00 bid ---\n";
    submit(Side::Sell, 10000, 20);
    book.print();

    // --- 3. Aggressive buy sweeps the ask at 101.00 ---
    std::cout << "--- Buy 6 at 101.00 sweeps the ask ---\n";
    submit(Side::Buy,  10100,  6);
    book.print();

    // --- 4. Add more resting depth on both sides ---
    std::cout << "--- Adding resting depth ---\n";
    submit(Side::Buy,   9900,  5);
    submit(Side::Sell, 10200,  4);
    book.print();

    // --- 5. Market buy takes all available liquidity ---
    std::cout << "--- Market buy 10 takes best ask ---\n";
    submit(Side::Buy,  0, 10, Type::Market);
    book.print();

    // --- 6. Market sell drains remaining bids ---
    std::cout << "--- Market sell 20 drains bids ---\n";
    submit(Side::Sell, 0, 20, Type::Market);
    book.print();

    // --- Session summary ---
    cli::render_trades(all_trades);
}
