#include "OrderBook.h"
#include <print>

void OrderBook::MatchBuy(Order& incoming)
{
    const u64 startIndex = PriceToIndex(MIN_PRICE);
    const u64 maxIndex = PriceToIndex(incoming.m_Price);

    u64 currentIndex = startIndex;
    while (currentIndex <= maxIndex)
    {
        if (incoming.m_Quantity == 0)
        {
            return;
        }

        auto nextIndex = FindNextOccupied(m_SellBitMap, m_SellSummary, currentIndex);
        if (!nextIndex || *nextIndex > maxIndex)
        {
            return;
        }

        PriceLevel& sellLevel = m_SellLevels[*nextIndex];
        MatchOrder(incoming, sellLevel);
        if (sellLevel.m_Orders.empty())
        {
            ClearOccupied(m_SellBitMap, m_SellSummary, *nextIndex);
        }

        currentIndex = *nextIndex + 1;
    }
}

void OrderBook::MatchSell(Order& incoming)
{
    const u64 startIndex = PriceToIndex(MAX_PRICE);
    const u64 minIndex = PriceToIndex(incoming.m_Price);

    u64 currentIndex = startIndex;
    while (currentIndex >= minIndex)
    {
        if (incoming.m_Quantity == 0)
        {
            return;
        }

        auto nextIndex = FindPrevOccupied(m_SellBitMap, m_BuySummary, currentIndex);
        if (!nextIndex || *nextIndex < minIndex)
        {
            return;
        }

        PriceLevel& buyLevel = m_BuyLevels[*nextIndex];
        MatchOrder(incoming, buyLevel);
        if (buyLevel.m_Orders.empty())
        {
            ClearOccupied(m_SellBitMap,m_BuySummary, *nextIndex);
        }

        if (*nextIndex == 0)
        {
            return;
        }

        currentIndex = *nextIndex - 1;
    }

}

void OrderBook::MatchOrder(Order& incoming, PriceLevel& priceLevel)
{
    for (auto it = priceLevel.m_Orders.begin(); it != priceLevel.m_Orders.end(); )
    {
        if (incoming.m_Quantity == 0)
        {
            return;
        }

        Order& order = *it;

        u32 tradedQuantity = std::min(order.m_Quantity, incoming.m_Quantity);
        order.m_Quantity -= tradedQuantity;
        incoming.m_Quantity -= tradedQuantity;
        priceLevel.m_TotalQuantity -= tradedQuantity;

        if (order.m_Quantity == 0)
        {
            it = priceLevel.m_Orders.erase(it);
        }
        else
        {
            it++;
        }

        std::println("TRADE {} @ {} | Incoming: {}", tradedQuantity, order.m_Price, incoming.m_Side == Side::Buy ? "BUY" : "SELL");
    }
}

void OrderBook::AddToBook(PriceLevel& priceLevel, const Order& order, BitMapArray& bitMapArray, SummaryArray& summaryArray)
{
    if (order.m_Quantity > 0)
    {
        if (priceLevel.m_Orders.empty())
        {
            SetOccupied(bitMapArray, summaryArray, PriceToIndex(order.m_Price)); //only need to set if it was not set before
        }
        priceLevel.m_Orders.push_back(order);
        priceLevel.m_TotalQuantity += order.m_Quantity;
    }
}

void OrderBook::AddOrder(Order order)
{
    if (!IsValidPrice(order.m_Price))
    {
        return;
    }

    if (order.m_Side == Side::Buy)
    {
        MatchBuy(order);
        AddToBook(m_BuyLevels[PriceToIndex(order.m_Price)], order, m_BuyBitMap, m_BuySummary);
    }
    else
    {
        MatchSell(order);
        AddToBook(m_SellLevels[PriceToIndex(order.m_Price)], order, m_SellBitMap, m_SellSummary);
    }
}

void OrderBook::SetOccupied(BitMapArray& bitMapArray, SummaryArray& summaryArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    bitMapArray[bitMapIndex] |= u64{ 1 } << bitIndex;

    const u64 summaryIndex  = bitMapIndex / 64;
    const u64 summaryBitIndex = bitMapIndex % 64;

    summaryArray[summaryIndex] |= u64{ 1 } << summaryBitIndex;
}

void OrderBook::ClearOccupied(BitMapArray& bitMapArray, SummaryArray& summaryArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    bitMapArray[bitMapIndex] &= ~(u64{ 1 } << bitIndex);

    if (bitMapArray[bitMapIndex] != 0)
    {
        return;
    }

    const u64 summaryIndex = bitMapIndex / 64;
    const u64 summaryBitIndex = bitMapIndex % 64;

    summaryArray[summaryIndex] &= ~(u64{ 1 } << summaryBitIndex);
}

bool OrderBook::IsOccupied(const BitMapArray& bitMapArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    return bitMapArray[bitMapIndex] & u64{ 1 } << bitIndex;
}

std::optional<u64> OrderBook::FindNextOccupied(const BitMapArray& bitMapArray, const SummaryArray& summaryArray, u64 currentIndex)
{
    u64 bitMapIndex = currentIndex / 64; //index of the corresponding bitmap holding the func input index
    u64 bitIndex = currentIndex % 64; //index within the corresponding bitmap

    u64 bitMap = bitMapArray[bitMapIndex];
    u64 mask = ~u64{ 0 } << bitIndex;

    bitMap &= mask;

    if (bitMap != 0)
    {
        return bitMapIndex * 64 + std::countr_zero(bitMap);
    }

    u64 summaryIndex = bitMapIndex / 64;
    u64 summaryBitIndex = bitMapIndex % 64;

    u64 summary = summaryArray[summaryIndex];
    u64 summaryMask = ~u64{ 0 } << (summaryBitIndex + 1); //Shifting beyond the current bitMap

    summary &= summaryBitIndex < 63 ? summaryMask : 0; //shifting beyond 64 bits is undefined behaviour

    if (summary != 0)
    {
        const u64 occupiedSummaryBit = std::countr_zero(summary);

        const u64 nextBitMapIndex = summaryIndex * 64 + occupiedSummaryBit;
        const u64 nextBitMapBit = std::countr_zero(bitMapArray[nextBitMapIndex]);

        return nextBitMapIndex * 64 + nextBitMapBit;
    }

    summaryIndex++;
    while (summaryIndex < SUMMARY_NUM)
    {
        summary = summaryArray[summaryIndex];

        if (summary != 0)
        {
            const u64 occupiedSummaryBit = std::countr_zero(summary);

            const u64 nextBitMapIndex = summaryIndex * 64 + occupiedSummaryBit;
            const u64 nextBitMapBit = std::countr_zero(bitMapArray[nextBitMapIndex]);

            return nextBitMapIndex * 64 + nextBitMapBit;
        }

        summaryIndex++;
    }

    return std::nullopt;
}

std::optional<u64> OrderBook::FindPrevOccupied(const BitMapArray& bitMapArray, const SummaryArray& summaryArray, u64 currentIndex)
{
    u64 bitMapIndex = currentIndex / 64; //index of the corresponding bitmap holding the func input index
    u64 bitIndex = currentIndex % 64; //index within the corresponding bitmap

    u64 bitMap = bitMapArray[bitMapIndex];
    u64 mask = ~u64{ 0 } >> (63 - bitIndex);

    bitMap &= mask;

    if (bitMap != 0)
    {
        return bitMapIndex * 64 + (63 - std::countl_zero(bitMap));
    }

    u64 summaryIndex = bitMapIndex / 64;
    u64 summaryBitIndex = bitMapIndex % 64;

    u64 summary = summaryArray[summaryIndex];
    u64 summaryMask = ~u64{ 0 } >> (64 - summaryBitIndex); //63 + 1 as we don't need the current index

    summary &= summaryBitIndex > 0 ? summaryMask : 0;

    if (summary != 0)
    {
        const u64 occupiedSummaryBit = 63 - std::countl_zero(summary);

        const u64 prevBitMapIndex = summaryIndex * 64 + occupiedSummaryBit;
        const u64 prevBitMapBit = 63 - std::countl_zero(bitMapArray[prevBitMapIndex]);

        return prevBitMapIndex * 64 + prevBitMapBit;
    }

    while (summaryIndex > 0)
    {
        summaryIndex--;

        summary = summaryArray[summaryIndex];

        if (summary != 0)
        {
            const u64 occupiedSummaryBit = 63 - std::countl_zero(summary);

            const u64 prevBitMapIndex = summaryIndex * 64 + occupiedSummaryBit;
            const u64 prevBitMapBit = 63 - std::countl_zero(bitMapArray[prevBitMapIndex]);

            return prevBitMapIndex * 64 + prevBitMapBit;
        }
    }

    return std::nullopt;
}
