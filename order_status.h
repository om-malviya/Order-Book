#pragma once

enum class Status { Open, PartiallyFilled, Filled, Cancelled };

struct OrderStatus {
    int    order_id;
    Status status;
    int    original_qty;
    int    filled_qty;

    int remaining() const { return original_qty - filled_qty; }
};
