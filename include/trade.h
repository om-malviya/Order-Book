#pragma once
#include <cstdint>
#include "order.h"

struct Trade {
    int     id;
    int     buyer_id;
    int     seller_id;
    int64_t price;
    int     qty;
    Side    aggressor;  // For knowing Trade Buyer initiated or Seller initiated (Liquidity Taker)
};
