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

        auto nextIndex = FindNextOccupied(m_SellBitMap, currentIndex);
        if (!nextIndex || *nextIndex > maxIndex)
        {
            return;
        }

        PriceLevel& sellLevel = m_SellLevels[*nextIndex];
        MatchOrder(incoming, sellLevel);
        if (sellLevel.m_Orders.empty())
        {
            ClearOccupied(m_SellBitMap, *nextIndex);
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

        auto nextIndex = FindPrevOccupied(m_SellBitMap, currentIndex);
        if (!nextIndex || *nextIndex < minIndex)
        {
            return;
        }

        PriceLevel& buyLevel = m_BuyLevels[*nextIndex];
        MatchOrder(incoming, buyLevel);
        if (buyLevel.m_Orders.empty())
        {
            ClearOccupied(m_SellBitMap, *nextIndex);
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

void OrderBook::AddToBook(PriceLevel& priceLevel, const Order& order, BitMapArray& bitMapArray)
{
    if (order.m_Quantity > 0)
    {
        if (priceLevel.m_Orders.empty())
        {
            SetOccupied(bitMapArray, PriceToIndex(order.m_Price)); //only need to set if it was not set before
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
        AddToBook(m_BuyLevels[PriceToIndex(order.m_Price)], order, m_BuyBitMap);
    }
    else
    {
        MatchSell(order);
        AddToBook(m_SellLevels[PriceToIndex(order.m_Price)], order, m_SellBitMap);
    }
}

void OrderBook::SetOccupied(BitMapArray& bitMapArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    bitMapArray[bitMapIndex] |= u64{ 1 } << bitIndex;
}

void OrderBook::ClearOccupied(BitMapArray& bitMapArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    bitMapArray[bitMapIndex] &= ~(u64{ 1 } << bitIndex);
}

bool OrderBook::IsOccupied(const BitMapArray& bitMapArray, u64 index)
{
    const u64 bitMapIndex = index / 64; //index of the corresponding bitmap holding the func input index
    const u64 bitIndex = index % 64; //index within the corresponding bitmap

    return bitMapArray[bitMapIndex] & u64{ 1 } << bitIndex;
}

std::optional<u64> OrderBook::FindNextOccupied(const BitMapArray& bitMapArray, u64 currentIndex)
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

    bitMapIndex++;
    while (bitMapIndex < BITMAP_NUM)
    {
        bitMap = bitMapArray[bitMapIndex];

        if (bitMap != 0)
        {
            return bitMapIndex * 64 + std::countr_zero(bitMap);
        }

        bitMapIndex++;
    }

    return std::optional<u64>();
}

std::optional<u64> OrderBook::FindPrevOccupied(const BitMapArray& bitMapArray, u64 currentIndex)
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

    while (bitMapIndex > 0)
    {
        bitMapIndex--;

        bitMap = bitMapArray[bitMapIndex];

        if (bitMap != 0)
        {
           return bitMapIndex * 64 + (63 - std::countl_zero(bitMap));
        }
    }
    return std::optional<u64>();
}
