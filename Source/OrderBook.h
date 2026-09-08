#pragma once

#include "Defines.h"
#include <vector>
#include <array>
#include <unordered_map>
#include <optional>

struct Order
{
     u64 m_ID;
     u32 m_Quantity;
     u32 m_Prev = INVALID_ORDER;
     u32 m_Next = INVALID_ORDER;
};

struct OrderInfo
{   
    u64 m_PriceIndex;
    u32 m_OrderIndex;
    Side m_Side;
};

struct PriceLevel
{
    u32 m_Head = INVALID_ORDER;
    u32 m_Tail = INVALID_ORDER;
    u32 m_TotalQuantity = 0;
};

class OrderBook
{
public:
    OrderBook();

private:
    static constexpr Price MIN_PRICE = 10000;
    static constexpr Price MAX_PRICE = 12000;
    static constexpr Price NUM_PRICE_LEVELS = MAX_PRICE - MIN_PRICE + 1;

    static constexpr u64 BITMAP_NUM = (NUM_PRICE_LEVELS + 63) / 64;
    static constexpr u64 SUMMARY_NUM = (BITMAP_NUM + 63) / 64;

    using BitMapArray = std::array<u64, BITMAP_NUM>;
    using SummaryArray = std::array<u64, SUMMARY_NUM>;

    inline u64 PriceToIndex(Price price) const { return static_cast<u64>(price - MIN_PRICE);}
    inline Price IndexToPrice(u64 priceIndex) const { return static_cast<Price>(priceIndex + static_cast<u64>(MIN_PRICE)); }
    inline bool IsValidPrice(Price price) const { return price >= MIN_PRICE && price <= MAX_PRICE; }

    void MatchBuy(u64 orderId, Price price, u32& quantity);
    void MatchSell(u64 orderId, Price price, u32& quantity);
    void MatchOrder(u64 orderId, Side side, Price price, u32& quantity);

    void FillRestingOrder(const OrderInfo& orderInfo, u32 quantity);
    u32 AllocateOrder();
    void FreeOrder(u32 orderIndex);
    
    std::vector<Order> m_OrderPool;
    std::unordered_map<u64, OrderInfo> m_OrderLookup;

    // Head of the singly-linked list of free order slots.
    u32 m_FreeOrderHead = INVALID_ORDER;

    std::array<PriceLevel, NUM_PRICE_LEVELS> m_BuyLevels;
    std::array<PriceLevel, NUM_PRICE_LEVELS> m_SellLevels;

    //A binary(0/1) representation of occupied PriceLevels. Each entry is a 64 bit binary where each bit corresponds to a PriceLevel
    BitMapArray m_BuyBitMap{};
    BitMapArray m_SellBitMap{};

    //A binary(0/1) representation of bitmaps that are not 0. Each entry is a 64 bit binary where each bit corresponds to a Bitmap
    SummaryArray m_BuySummary{};
    SummaryArray m_SellSummary{};

public:
    void AddOrder(u64 orderId, Side side, Price price, u32 quantity);
    void AddOrderToLevel(u64 orderId, Side side, u64 priceIndex, u32 quantity);
    void CancelOrder(u64 orderID);
    void RemoveOrder(const OrderInfo& orderInfo);

    void SetOccupied(BitMapArray& bitMapArray, SummaryArray& summaryArray, u64 index);
    void ClearOccupied(BitMapArray& bitMapArray, SummaryArray& summaryArray, u64 index);
    bool IsOccupied(const BitMapArray& bitMapArray, u64 index);

    std::optional<u64> FindNextOccupied(const BitMapArray& bitMapArray, const SummaryArray& summaryArray, u64 currentOccupiedIndex);
    std::optional<u64> FindPrevOccupied(const BitMapArray& bitMapArray, const SummaryArray& summaryArray, u64 currentOccupiedIndex);
};

