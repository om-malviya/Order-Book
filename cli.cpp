#include "cli.h"
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
    else                        line << "limit " << o.price;
    line << " x " << o.qty;
    std::cout << line.str() << "\n";
}

void cli::trade(const Trade& t) {
    const char* agg_color = (t.aggressor == Side::Buy) ? GREEN : RED;
    const char* agg_word  = (t.aggressor == Side::Buy) ? "BUY" : "SELL";

    std::ostringstream line;
    line << "    " << paint("TRADE", BOLD) << " #" << t.id
         << "  " << t.price << " x " << t.qty
         << "  buyer " << t.buyer_id << "  seller " << t.seller_id
         << "  " << paint(std::string("[") + agg_word + " aggressor]", agg_color);
    std::cout << line.str() << "\n";
}

void cli::order_status(const OrderStatus& s) {
    std::ostringstream line;
    line << "    " << paint("└ ", GREY)
         << paint(status_word(s.status), status_color(s.status)) << "  ";
    if (s.status == Status::Cancelled) {
        line << "filled " << s.filled_qty << "  cancelled " << s.remaining();
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
                  << rjust(it->price, 8) << "  " << rjust(it->volume, 7)
                  << "   " << paint(depth_bar(it->volume, max_vol), RED) << "\n";

    if (!asks.empty() && !bids.empty()) {
        int64_t best_ask = asks.front().price;
        int64_t best_bid = bids.front().price;
        std::ostringstream div;
        div << "  ----------------------------  spread " << (best_ask - best_bid)
            << "  mid " << std::fixed << std::setprecision(1)
            << (best_ask + best_bid) / 2.0;
        std::cout << paint(div.str(), GREY) << "\n";
    } else if (asks.empty() && bids.empty()) {
        std::cout << paint("  (empty book)", DIM) << "\n";
    } else {
        std::cout << paint("  ----------------------------  (one-sided)", GREY) << "\n";
    }

    for (const auto& l : bids)
        std::cout << "  " << paint("BID", GREEN)
                  << rjust(l.price, 8) << "  " << rjust(l.volume, 7)
                  << "   " << paint(depth_bar(l.volume, max_vol), GREEN) << "\n";

    std::cout << "\n";
}
