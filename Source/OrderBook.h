#pragma once

#include "Defines.h"
#include <vector>
#include <array>
#include <optional>

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

    static constexpr u64 BITMAP_NUM = (NUM_PRICE_LEVELS + 63) / 64;
    static constexpr u64 SUMMARY_NUM = (BITMAP_NUM + 63) / 64;

    using BitMapArray = std::array<u64, BITMAP_NUM>;

    inline Price PriceToIndex(Price price) const { return price - MIN_PRICE;}
    inline bool IsValidPrice(Price price) const { return price >= MIN_PRICE && price <= MAX_PRICE; }

    void MatchBuy(Order& order);
    void MatchSell(Order& order);
    void MatchOrder(Order& order, PriceLevel& level);
    void AddToBook(PriceLevel& priceLevel, const Order& order, BitMapArray& bitMapArray);
    
    std::array<PriceLevel, NUM_PRICE_LEVELS> m_BuyLevels;
    std::array<PriceLevel, NUM_PRICE_LEVELS> m_SellLevels;

    BitMapArray m_BuyBitMap;
    BitMapArray m_SellBitMap;

public:
    void AddOrder(Order order);

    void SetOccupied(BitMapArray& bitMapArray, u64 index);
    void ClearOccupied(BitMapArray& bitMapArray, u64 index);
    bool IsOccupied(const BitMapArray& bitMapArray, u64 index);

    std::optional<u64> FindNextOccupied(const BitMapArray& bitMapArray, u64 currentOccupiedIndex);
    std::optional<u64> FindPrevOccupied(const BitMapArray& bitMapArray, u64 currentOccupiedIndex);
};

