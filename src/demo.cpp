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

    // --- 1. Build a two-sided resting book (5 asks, 5 bid levels) ---
    std::cout << "\n--- Building resting depth ---\n";
    submit(Side::Sell, 10250,  2);   // 102.50 thin offer
    submit(Side::Sell, 10200,  4);   // 102.00
    submit(Side::Sell, 10150,  7);   // 101.50
    submit(Side::Sell, 10100,  6);   // 101.00
    submit(Side::Sell, 10050,  3);   // 100.50 near-touch
    submit(Side::Buy,  10000, 10);   // 100.00 first order
    submit(Side::Buy,  10000,  5);   // 100.00 second order (FIFO)
    submit(Side::Buy,   9950,  8);   //  99.50
    submit(Side::Buy,   9900,  6);   //  99.00
    submit(Side::Buy,   9850,  4);   //  98.50
    submit(Side::Buy,   9800,  2);   //  98.00 thin bid
    book.print();

    // --- 2. Sell crosses the 100.00 bid; remainder rests on ask ---
    std::cout << "--- Sell 20 at 100.00 crosses both 100.00 bids ---\n";
    submit(Side::Sell, 10000, 20);
    book.print();

    // --- 3. Buy sweeps three ask levels; remainder rests ---
    std::cout << "--- Buy 16 at 101.00 sweeps three ask levels ---\n";
    submit(Side::Buy, 10100, 16);
    book.print();

    // --- 4. Market sell drains top bids ---
    std::cout << "--- Market sell 12 takes best bids ---\n";
    submit(Side::Sell, 0, 12, Type::Market);
    book.print();

    // --- 5. Market buy clears remaining asks ---
    std::cout << "--- Market buy 10 takes remaining asks ---\n";
    submit(Side::Buy, 0, 10, Type::Market);
    book.print();

    // --- Session summary ---
    cli::render_trades(all_trades);

    cli::Stats st;
    st.orders_submitted = next_id - 1;
    st.trades_executed  = static_cast<int>(all_trades.size());
    int64_t price_sum   = 0;
    for (const auto& t : all_trades) {
        st.volume_traded += t.qty;
        price_sum        += t.price * t.qty;
    }
    if (st.volume_traded > 0)
        st.avg_price_ticks = price_sum / st.volume_traded;
    cli::render_stats(st);
}
