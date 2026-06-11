#include "cli.h"
#include "ticks.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <string>
#include <unistd.h>
#include <cstdlib>

namespace {

bool g_color = false;

constexpr const char* RESET  = "\033[0m";
constexpr const char* BOLD   = "\033[1m";
constexpr const char* DIM    = "\033[2m";
constexpr const char* RED    = "\033[31m";
constexpr const char* GREEN  = "\033[32m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* CYAN   = "\033[36m";
constexpr const char* GREY   = "\033[90m";

std::string paint(const std::string& s, const char* code) {
    if (!g_color) return s;
    return std::string(code) + s + RESET;
}

std::string rjust(int64_t v, int width) {
    std::ostringstream o;
    o << std::setw(width) << v;
    return o.str();
}

std::string rjust_str(const std::string& s, int width) {
    std::ostringstream o;
    o << std::setw(width) << s;
    return o.str();
}

std::string fmt_price(int64_t ticks) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(2) << (static_cast<double>(ticks) / TICK_SCALE);
    return o.str();
}

const char* status_word(Status s) {
    switch (s) {
        case Status::Open:            return "Open";
        case Status::PartiallyFilled: return "PartiallyFilled";
        case Status::Filled:          return "Filled";
        case Status::Cancelled:       return "Cancelled";
    }
    return "";
}

const char* status_color(Status s) {
    switch (s) {
        case Status::Open:            return CYAN;
        case Status::PartiallyFilled: return YELLOW;
        case Status::Filled:          return GREEN;
        case Status::Cancelled:       return GREY;
    }
    return RESET;
}

std::string depth_bar(int volume, int max_volume) {
    constexpr int MAX_BAR = 20;
    if (max_volume <= 0) return "";
    int len = static_cast<int>(std::lround(double(volume) / max_volume * MAX_BAR));
    if (len < 1) len = 1;
    return std::string(len, '#');
}

}  // namespace

void cli::init() {
    g_color = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == nullptr;
}

void cli::order_submitted(const Order& o) {
    const char* side_color = (o.side == Side::Buy) ? GREEN : RED;
    std::string side_tag   = (o.side == Side::Buy) ? "BUY " : "SELL";

    std::ostringstream line;
    line << paint(">", BOLD) << " " << paint(side_tag, side_color)
         << "  #" << o.id << "  ";
    if (o.type == Type::Market) line << "market";
    else                        line << "limit " << fmt_price(o.price);
    line << " x " << o.qty;
    std::cout << line.str() << "\n";
}

void cli::trade(const Trade& t) {
    const char* agg_color = (t.aggressor == Side::Buy) ? GREEN : RED;
    const char* agg_word  = (t.aggressor == Side::Buy) ? "BUY" : "SELL";

    std::ostringstream line;
    line << "    " << paint("TRADE", BOLD) << " #" << t.id
         << "  " << fmt_price(t.price) << " x " << t.qty
         << "  buyer " << t.buyer_id << "  seller " << t.seller_id
         << "  " << paint(std::string("[") + agg_word + " aggressor]", agg_color);
    std::cout << line.str() << "\n";
}

void cli::order_status(const OrderStatus& s) {
    std::ostringstream line;
    line << "    " << paint("└ ", GREY)
         << paint(status_word(s.status), status_color(s.status)) << "  ";
    if (s.status == Status::Cancelled) {
        line << "filled " << s.filled_qty << "/" << s.original_qty 
        << "  cancelled " << s.remaining() << "/" << s.original_qty;
    } else {
        line << s.filled_qty << "/" << s.original_qty;
    }
    std::cout << line.str() << "\n";
}

void cli::render_book(const std::vector<Level>& asks, const std::vector<Level>& bids) {
    int max_vol = 0;
    for (const auto& l : asks) max_vol = std::max(max_vol, l.volume);
    for (const auto& l : bids) max_vol = std::max(max_vol, l.volume);

    std::cout << "\n" << paint("  ORDER BOOK", BOLD) << "\n";
    std::cout << paint("        PRICE     SIZE   DEPTH", DIM) << "\n";

    // asks highest-first so best ask sits just above the spread
    for (auto it = asks.rbegin(); it != asks.rend(); ++it)
        std::cout << "  " << paint("ASK", RED)
                  << rjust_str(fmt_price(it->price), 9) << "  " << rjust(it->volume, 7)
                  << "   " << paint(depth_bar(it->volume, max_vol), RED) << "\n";

    if (!asks.empty() && !bids.empty()) {
        int64_t best_ask = asks.front().price;
        int64_t best_bid = bids.front().price;
        double  spread   = static_cast<double>(best_ask - best_bid) / TICK_SCALE;
        double  mid      = static_cast<double>(best_ask + best_bid) / 2.0 / TICK_SCALE;
        std::ostringstream div;
        div << "  ----------------------------  spread " << std::fixed << std::setprecision(2)
            << spread << "  mid " << mid;
        std::cout << paint(div.str(), GREY) << "\n";
    } else if (asks.empty() && bids.empty()) {
        std::cout << paint("  (empty book)", DIM) << "\n";
    } else {
        std::cout << paint("  ----------------------------  (one-sided)", GREY) << "\n";
    }

    for (const auto& l : bids)
        std::cout << "  " << paint("BID", GREEN)
                  << rjust_str(fmt_price(l.price), 9) << "  " << rjust(l.volume, 7)
                  << "   " << paint(depth_bar(l.volume, max_vol), GREEN) << "\n";

    std::cout << "\n";
}

void cli::banner() {
    std::cout << "\n" << paint("  ORDER BOOK ENGINE", BOLD) << "\n"
              << paint("  a small price-time matching engine", DIM) << "\n\n";
}

void cli::start_menu() {
    std::cout << "  Start session:\n"
              << "   1) Demo\n"
              << "   2) Interactive\n";
}

void cli::menu() {
    std::cout << "\n" << paint("  === ORDER BOOK CONSOLE ===", BOLD) << "\n"
              << "   1) Place order\n"
              << "   2) View order book\n"
              << "   3) Order status\n"
              << "   4) Trade history\n"
              << "   5) Help\n"
              << "   0) Quit\n";
}

void cli::help() {
    std::cout << "\n" << paint("  HELP", BOLD) << "\n"
              << "   " << paint("Place order", GREEN)  << "  pick a type, then a side, then price (limit only) and size.\n"
              << "   " << paint("Limit", GREEN)        << "        rests on the book until it crosses; you set the price.\n"
              << "   " << paint("Market", GREEN)       << "       takes liquidity immediately; unfilled size is cancelled.\n"
              << "   " << paint("View book", GREEN)    << "    shows resting depth: asks above the spread, bids below.\n"
              << "   " << paint("Order status", GREEN) << " tracks each order: " << paint("Open", CYAN) << " / "
              << paint("PartiallyFilled", YELLOW) << " / " << paint("Filled", GREEN) << " / " << paint("Cancelled", GREY) << ".\n"
              << "   " << paint("Trade history", GREEN)<< " lists every fill produced this session.\n"
              << "   " << paint("Inputs are numbers", DIM) << ": type " << paint("1", GREEN) << " (Limit) / "
              << paint("2", GREEN) << " (Market), side " << paint("1", GREEN) << " (Buy) / " << paint("2", RED) << " (Sell).\n";
}

void cli::render_trades(const std::vector<Trade>& history) {
    std::cout << "\n" << paint("  TRADE HISTORY", BOLD) << "\n";
    if (history.empty()) {
        std::cout << paint("  (no trades yet)", DIM) << "\n";
        return;
    }
    std::cout << paint("       ID       PRICE    SIZE   BUYER  SELLER", DIM) << "\n";
    for (const auto& t : history) {
        const char* agg_color = (t.aggressor == Side::Buy) ? GREEN : RED;
        std::ostringstream id_col;
        id_col << "#" << t.id;
        std::cout << "  " << paint(rjust_str(id_col.str(), 5), agg_color)
                  << rjust_str(fmt_price(t.price), 9) << rjust(t.qty, 8)
                  << rjust(t.buyer_id, 8) << rjust(t.seller_id, 8) << "\n";
    }
}

void cli::render_statuses(const std::vector<StatusRow>& rows) {
    std::cout << "\n" << paint("  ORDERS", BOLD) << "\n";
    if (rows.empty()) {
        std::cout << paint("  (no orders yet)", DIM) << "\n";
        return;
    }
    std::cout << paint("    ID  SIDE     PRICE   FILLED   STATUS", DIM) << "\n";
    for (const auto& r : rows) {
        const char* side_color = (r.side == Side::Buy) ? GREEN : RED;
        std::string side_tag    = (r.side == Side::Buy) ? "BUY " : "SELL";

        std::ostringstream price_col;
        if (r.is_market) price_col << std::setw(9) << "market";
        else             price_col << std::setw(9) << fmt_price(r.price);

        std::ostringstream fill_col;
        fill_col << r.status.filled_qty << "/" << r.status.original_qty;

        std::cout << "  " << rjust(r.id, 3) << "  " << paint(side_tag, side_color)
                  << "  " << price_col.str()
                  << "  " << std::left << std::setw(7) << fill_col.str() << std::right
                  << "  " << paint(status_word(r.status.status), status_color(r.status.status)) << "\n";
    }
}
