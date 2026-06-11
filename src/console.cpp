#include "console.h"
#include "orderbook.h"
#include "cli.h"
#include "ticks.h"
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cmath>

namespace {

struct Session {
    OrderBook          book;
    int                next_id = 1;
    std::vector<Trade> history;
    std::vector<Order> submitted;
};

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool read_line(const char* prompt, std::string& out) {
    std::cout << prompt;
    std::cout.flush();
    return static_cast<bool>(std::getline(std::cin, out));
}

std::optional<long long> parse_int(const std::string& s) {
    std::string t = trim(s);
    if (t.empty()) return std::nullopt;
    try {
        size_t pos = 0;
        long long v = std::stoll(t, &pos);
        if (pos != t.size()) return std::nullopt;
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

// Reads a strictly-positive integer. nullopt means "abandon this action";
// eof is set true when the stream closed and the whole session should end.
std::optional<long long> read_positive(const char* prompt, const char* label, bool& eof) {
    std::string line;
    if (!read_line(prompt, line)) { eof = true; return std::nullopt; }
    auto v = parse_int(line);
    if (!v || *v <= 0) {
        std::cout << "  " << label << " must be a positive whole number. Cancelled.\n";
        return std::nullopt;
    }
    return v;
}

std::optional<int64_t> read_price(bool& eof) {
    std::string line;
    if (!read_line("  Price> ", line)) { eof = true; return std::nullopt; }
    std::string t = trim(line);
    try {
        size_t pos = 0;
        double d = std::stod(t, &pos);
        if (pos != t.size() || d <= 0) {
            std::cout << "  Price must be a positive number (e.g. 100.50). Cancelled.\n";
            return std::nullopt;
        }
        return static_cast<int64_t>(std::llround(d * TICK_SCALE));
    } catch (...) {
        std::cout << "  Price must be a positive number (e.g. 100.50). Cancelled.\n";
        return std::nullopt;
    }
}

std::optional<Type> read_type(bool& eof) {
    std::cout << "  Order type:\n"
              << "    1) Limit\n"
              << "    2) Market\n";
    std::string line;
    if (!read_line("  Type> ", line)) { eof = true; return std::nullopt; }
    std::string t = trim(line);
    if (t == "1") return Type::Limit;
    if (t == "2") return Type::Market;
    std::cout << "  Type must be 1 (Limit) or 2 (Market). Cancelled.\n";
    return std::nullopt;
}

std::optional<Side> read_side(bool& eof) {
    std::cout << "  Side:\n"
              << "    1) Buy\n"
              << "    2) Sell\n";
    std::string line;
    if (!read_line("  Side> ", line)) { eof = true; return std::nullopt; }
    std::string t = trim(line);
    if (t == "1") return Side::Buy;
    if (t == "2") return Side::Sell;
    std::cout << "  Side must be 1 (Buy) or 2 (Sell). Cancelled.\n";
    return std::nullopt;
}

void do_submit(Session& s, Order o) {
    cli::order_submitted(o);
    s.submitted.push_back(o);
    for (const auto& t : s.book.add(o)) {
        cli::trade(t);
        s.history.push_back(t);
    }
    if (const auto* st = s.book.status(o.id))
        cli::order_status(*st);
}

bool place_order(Session& s) {
    bool eof = false;

    auto type = read_type(eof);
    if (eof) return false;
    if (!type) return true;

    auto side = read_side(eof);
    if (eof) return false;
    if (!side) return true;

    int64_t price = 0;
    if (*type == Type::Limit) {
        auto p = read_price(eof);
        if (eof) return false;
        if (!p) return true;
        price = *p;
    }

    auto qty = read_positive("  Quantity> ", "Quantity", eof);
    if (eof) return false;
    if (!qty) return true;

    do_submit(s, Order{s.next_id++, *side, price, static_cast<int>(*qty), *type});
    return true;
}

bool show_status(Session& s) {
    std::string line;
    if (!read_line("  Order id (blank = all)> ", line)) return false;
    std::string t = trim(line);

    std::vector<cli::StatusRow> rows;
    if (t.empty()) {
        for (const auto& o : s.submitted)
            if (const auto* st = s.book.status(o.id))
                rows.push_back({o.id, o.side, o.price, o.type == Type::Market, *st});
    } else {
        auto id = parse_int(t);
        const Order* found = nullptr;
        if (id)
            for (const auto& o : s.submitted)
                if (o.id == *id) { found = &o; break; }

        const OrderStatus* st = (id ? s.book.status(static_cast<int>(*id)) : nullptr);
        if (!found || !st) {
            std::cout << "  No such order this session.\n";
            return true;
        }
        rows.push_back({found->id, found->side, found->price, found->type == Type::Market, *st});
    }
    cli::render_statuses(rows);
    return true;
}

}  // namespace

void console::run() {
    Session s;
    bool running = true;

    while (running) {
        cli::menu();
        std::string choice;
        if (!read_line("  Choose> ", choice)) break;
        std::string c = trim(choice);

        if      (c == "1") running = place_order(s);
        else if (c == "2") s.book.print();
        else if (c == "3") running = show_status(s);
        else if (c == "4") cli::render_trades(s.history);
        else if (c == "5") cli::help();
        else if (c == "0" || c == "q" || c == "quit") running = false;
        else if (!c.empty()) std::cout << "  Unknown option '" << c << "'. Type 6 for help.\n";
    }

    std::cout << "\n  Goodbye.\n";
}
