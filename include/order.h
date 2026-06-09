#pragma once
#include <cstdint>

enum class Side { Buy, Sell };
enum class Type { Limit, Market };

struct Order {
    int      id;
    Side     side;
    int64_t  price;
    int      qty;
    Type     type = Type::Limit;

    bool crosses(int64_t opposite_price) const {
        if (type == Type::Market) return true;
        return (side == Side::Buy) ? price >= opposite_price
                                   : price <= opposite_price;
    }

    bool rests() const { return type == Type::Limit; }
};
