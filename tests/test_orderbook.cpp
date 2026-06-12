#include <catch2/catch_test_macros.hpp>
#include "orderbook.h"

static Order buy(int id, int64_t price, int qty) {
    return {id, Side::Buy, price, qty, Type::Limit};
}
static Order sell(int id, int64_t price, int qty) {
    return {id, Side::Sell, price, qty, Type::Limit};
}
static Order market_buy(int id, int qty) {
    return {id, Side::Buy, 0, qty, Type::Market};
}
static Order market_sell(int id, int qty) {
    return {id, Side::Sell, 0, qty, Type::Market};
}

// ---------------------------------------------------------------------------
// Matching behaviour
// ---------------------------------------------------------------------------

TEST_CASE("Non-crossing orders rest without a trade", "[matching]") {
    OrderBook book;
    REQUIRE(book.add(buy(1, 100, 10)).empty());
    REQUIRE(book.add(sell(2, 101, 5)).empty());
}

TEST_CASE("Crossing limit orders produce exactly one trade", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 10));
    auto trades = book.add(sell(2, 100, 10));

    REQUIRE(trades.size() == 1);
    CHECK(trades[0].buyer_id  == 1);
    CHECK(trades[0].seller_id == 2);
    CHECK(trades[0].price     == 100);
    CHECK(trades[0].qty       == 10);
}

TEST_CASE("Trade executes at the resting order's price, not the aggressor's", "[matching]") {
    OrderBook book;
    book.add(sell(1, 100, 5));              // ask rests at 100
    auto trades = book.add(buy(2, 110, 5)); // buy willing to pay 110

    REQUIRE(trades.size() == 1);
    CHECK(trades[0].price == 100);  // price improvement: aggressor gets the better price
}

TEST_CASE("FIFO: earlier order at same price fills first", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 5));
    book.add(buy(2, 100, 5));  // same price, arrived second

    auto trades = book.add(sell(3, 100, 5));
    REQUIRE(trades.size() == 1);
    CHECK(trades[0].buyer_id == 1);
}

TEST_CASE("Partial fill: incoming smaller than resting", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 10));
    auto trades = book.add(sell(2, 100, 3));

    REQUIRE(trades.size() == 1);
    CHECK(trades[0].qty == 3);
    CHECK(book.status(1)->filled_qty  == 3);
    CHECK(book.status(1)->remaining() == 7);
}

TEST_CASE("Partial fill: incoming larger than resting, remainder rests", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 5));
    auto trades = book.add(sell(2, 100, 10));

    REQUIRE(trades.size() == 1);
    CHECK(trades[0].qty == 5);
    CHECK(book.status(1)->status == Status::Filled);
    CHECK(book.status(2)->status == Status::PartiallyFilled);
    CHECK(book.status(2)->remaining() == 5);
}

TEST_CASE("Sweep across multiple bid levels", "[matching]") {
    OrderBook book;
    book.add(buy(1, 101, 5));
    book.add(buy(2, 100, 5));
    auto trades = book.add(sell(3, 99, 10));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].price == 101);  // best bid first
    CHECK(trades[1].price == 100);
    CHECK(book.status(3)->status == Status::Filled);
}

TEST_CASE("Sweep across multiple ask levels", "[matching]") {
    OrderBook book;
    book.add(sell(1, 100, 5));
    book.add(sell(2, 101, 5));
    auto trades = book.add(buy(3, 102, 10));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].price == 100);  // best ask first
    CHECK(trades[1].price == 101);
    CHECK(book.status(3)->status == Status::Filled);
}

TEST_CASE("Market buy sweeps all available asks", "[matching]") {
    OrderBook book;
    book.add(sell(1, 100, 3));
    book.add(sell(2, 101, 4));
    auto trades = book.add(market_buy(3, 7));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].price == 100);
    CHECK(trades[1].price == 101);
    CHECK(book.status(3)->status == Status::Filled);
}

TEST_CASE("Market sell sweeps all available bids", "[matching]") {
    OrderBook book;
    book.add(buy(1, 101, 4));
    book.add(buy(2, 100, 3));
    auto trades = book.add(market_sell(3, 7));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].price == 101);
    CHECK(trades[1].price == 100);
    CHECK(book.status(3)->status == Status::Filled);
}

TEST_CASE("Market order against empty book is fully cancelled", "[matching]") {
    OrderBook book;
    auto trades = book.add(market_buy(1, 10));

    REQUIRE(trades.empty());
    CHECK(book.status(1)->status    == Status::Cancelled);
    CHECK(book.status(1)->filled_qty == 0);
    CHECK(book.status(1)->remaining() == 10);
}

TEST_CASE("Market order with insufficient liquidity: partial fill then cancelled", "[matching]") {
    OrderBook book;
    book.add(sell(1, 100, 4));
    auto trades = book.add(market_buy(2, 10));

    REQUIRE(trades.size() == 1);
    CHECK(trades[0].qty == 4);
    CHECK(book.status(2)->status     == Status::Cancelled);
    CHECK(book.status(2)->filled_qty  == 4);
    CHECK(book.status(2)->remaining() == 6);
}

TEST_CASE("Aggressor side is recorded correctly", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 5));
    auto sell_trades = book.add(sell(2, 100, 5));
    REQUIRE(!sell_trades.empty());
    CHECK(sell_trades[0].aggressor == Side::Sell);

    book.add(sell(3, 100, 5));
    auto buy_trades = book.add(buy(4, 100, 5));
    REQUIRE(!buy_trades.empty());
    CHECK(buy_trades[0].aggressor == Side::Buy);
}

// ---------------------------------------------------------------------------
// Order status lifecycle
// ---------------------------------------------------------------------------

TEST_CASE("New resting order has Open status with zero fills", "[status]") {
    OrderBook book;
    book.add(buy(1, 100, 10));

    auto s = book.status(1);
    REQUIRE(s != nullptr);
    CHECK(s->status      == Status::Open);
    CHECK(s->filled_qty  == 0);
    CHECK(s->remaining() == 10);
}

TEST_CASE("Fully filled order has Filled status", "[status]") {
    OrderBook book;
    book.add(buy(1, 100, 10));
    book.add(sell(2, 100, 10));

    CHECK(book.status(1)->status     == Status::Filled);
    CHECK(book.status(1)->filled_qty == 10);
    CHECK(book.status(1)->remaining() == 0);
}

TEST_CASE("Resting order transitions Open → PartiallyFilled → Filled", "[status]") {
    OrderBook book;
    book.add(buy(1, 100, 10));
    CHECK(book.status(1)->status == Status::Open);

    book.add(sell(2, 100, 4));
    CHECK(book.status(1)->status     == Status::PartiallyFilled);
    CHECK(book.status(1)->filled_qty == 4);

    book.add(sell(3, 100, 6));
    CHECK(book.status(1)->status     == Status::Filled);
    CHECK(book.status(1)->filled_qty == 10);
}

TEST_CASE("Unknown order id returns nullptr", "[status]") {
    OrderBook book;
    CHECK(book.status(999) == nullptr);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_CASE("Sell sweeps two resting buys at same price, second partially filled", "[matching]") {
    OrderBook book;
    book.add(buy(1, 100, 5));
    book.add(buy(2, 100, 5));
    auto trades = book.add(sell(3, 100, 8));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].buyer_id == 1);
    CHECK(trades[0].qty      == 5);
    CHECK(trades[1].buyer_id == 2);
    CHECK(trades[1].qty      == 3);
    CHECK(book.status(1)->status     == Status::Filled);
    CHECK(book.status(2)->status     == Status::PartiallyFilled);
    CHECK(book.status(2)->remaining() == 2);
    CHECK(book.status(3)->status     == Status::Filled);
}

TEST_CASE("Incoming limit sweeps two ask levels, remainder rests as PartiallyFilled", "[matching]") {
    OrderBook book;
    book.add(sell(1, 100, 3));
    book.add(sell(2, 101, 5));
    auto trades = book.add(buy(3, 102, 12));

    REQUIRE(trades.size() == 2);
    CHECK(trades[0].price == 100);
    CHECK(trades[1].price == 101);
    CHECK(book.status(3)->filled_qty  == 8);
    CHECK(book.status(3)->remaining() == 4);
    CHECK(book.status(3)->status      == Status::PartiallyFilled);
}
