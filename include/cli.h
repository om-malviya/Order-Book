#pragma once
#include <vector>
#include <cstdint>
#include "order.h"
#include "order_status.h"
#include "trade.h"

namespace cli {

// Enable colors only when stdout is a real terminal (and NO_COLOR is unset).
void init();

void order_submitted(const Order& o);
void trade(const Trade& t);
void order_status(const OrderStatus& s);

struct Level {
    int64_t price;
    int     volume;
};

// asks_ascending: lowest price first (best ask at front).
// bids_descending: highest price first (best bid at front).
void render_book(const std::vector<Level>& asks_ascending,
                 const std::vector<Level>& bids_descending);

// --- Interactive console views ---

void banner();
void start_menu();
void menu();
void help();

void render_trades(const std::vector<Trade>& history);

struct StatusRow {
    int         id;
    Side        side;
    int64_t     price;
    bool        is_market;
    OrderStatus status;
};
void render_statuses(const std::vector<StatusRow>& rows);

struct Stats {
    int     orders_submitted = 0;
    int     trades_executed  = 0;
    int     volume_traded    = 0;
    int64_t avg_price_ticks  = 0; // 0 means no trades
};
void render_stats(const Stats& stats);

}  // namespace cli
