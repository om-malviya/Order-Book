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

}  // namespace cli
