#pragma once

#include "Defines.h"
#include <vector>
#include <array>

struct Order
{
     u64 m_ID;
     Side m_Side;
     Price m_Price;
     u32 m_Quantity;
};

struct PriceLevel
{
    std::vector<Order> m_Orders;
    u32 m_TotalQuantity = 0;
};

class OrderBook
{
private:
    static constexpr Price MIN_PRICE = 10000;
    static constexpr Price MAX_PRICE = 12000;
    static constexpr Price NUM_PRICE_LEVELS = MAX_PRICE - MIN_PRICE + 1;

    inline Price PriceToIndex(Price price) const { return price - MIN_PRICE;}
    inline bool IsValidPrice(Price price) const { return price >= MIN_PRICE && price <= MAX_PRICE; }

    void MatchBuy(Order& order);
    void MatchSell(Order& order);
    void MatchOrder(Order& order, PriceLevel& level);
    
    std::array<PriceLevel, NUM_PRICE_LEVELS> m_BuyLevels;
    std::array<PriceLevel, NUM_PRICE_LEVELS> m_SellLevels;

public:
    void AddOrder(Order order);
};

