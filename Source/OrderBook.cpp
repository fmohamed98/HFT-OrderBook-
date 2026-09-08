#include "OrderBook.h"
#include <print>

OrderBook::OrderBook()
{
    m_OrderPool.reserve(100'000);
    m_OrderLookup.reserve(100'000);
}

void OrderBook::MatchBuy(u64 orderId, Price price, u32& quantity)
{
    u64 searchIndex = PriceToIndex(MIN_PRICE);

    while (quantity > 0)
    {
        const auto bestAskIndex = FindNextOccupied(m_SellBitMap, m_SellSummary, searchIndex);
        if (!bestAskIndex)
        {
            return;
        }

        const Price bestAsk = IndexToPrice(*bestAskIndex);
        if (bestAsk > price)
        {
            return;
        }

        PriceLevel& sellLevel = m_SellLevels[*bestAskIndex];
        while (quantity > 0 && sellLevel.m_Head != INVALID_ORDER)
        {
            const u32 orderIndex = sellLevel.m_Head;
            Order& restingOrder = m_OrderPool[orderIndex];
            const u32 tradedQuantity = std::min(restingOrder.m_Quantity, quantity);

            const OrderInfo info{ *bestAskIndex, orderIndex, Side::Sell };

            std::println(
                "TRADE: BUY {} matched SELL {} @ {} x {}",
                orderId,
                restingOrder.m_ID,
                bestAsk,
                tradedQuantity);

            quantity -= tradedQuantity;

            FillRestingOrder(info, tradedQuantity);

            if (restingOrder.m_Quantity == 0)
            {
                RemoveOrder(info);
            }
        }

        if (quantity == 0)
        {
            return;
        }

        if (*bestAskIndex == NUM_PRICE_LEVELS - 1)
        {
            return;
        }

        searchIndex = *bestAskIndex + 1;
    }
}

void OrderBook::MatchSell(u64 orderId, Price price, u32& quantity)
{
    u64 searchIndex = PriceToIndex(MAX_PRICE);
    
    while (quantity > 0)
    {
        const auto bestBidIndex = FindPrevOccupied(m_BuyBitMap, m_BuySummary, searchIndex);
        if (!bestBidIndex)
        {
            return;
        }

        const Price bestBid = IndexToPrice(*bestBidIndex);
        if (bestBid < price)
        {
            return;
        }

        PriceLevel& buyLevel = m_BuyLevels[*bestBidIndex];
        while (quantity > 0 && buyLevel.m_Head != INVALID_ORDER)
        {
            const u32 orderIndex = buyLevel.m_Head;
            Order& restingOrder = m_OrderPool[orderIndex];
            const u32 tradedQuantity = std::min(restingOrder.m_Quantity, quantity);

            const OrderInfo info{ *bestBidIndex, orderIndex, Side::Buy };

            std::println(
                "TRADE: SELL {} matched BUY {} @ {} x {}",
                orderId,
                restingOrder.m_ID,
                bestBid,
                tradedQuantity);

            quantity -= tradedQuantity;

            FillRestingOrder(info, tradedQuantity);

            if (restingOrder.m_Quantity == 0)
            {
                RemoveOrder(info);
            }
        }

        if (quantity == 0)
        {
            return;
        }

        if (*bestBidIndex == 0)
        {
            return;
        }

        searchIndex = *bestBidIndex - 1;
    }

}

void OrderBook::MatchOrder(u64 orderId, Side side, Price price, u32& quantity)
{
    if (side == Side::Buy)
    {
        MatchBuy(orderId, price, quantity);
    }
    else
    {
        MatchSell(orderId, price, quantity);
    }
}

void OrderBook::FillRestingOrder(const OrderInfo& orderInfo, u32 tradedQuantity)
{
    const u64 priceIndex = orderInfo.m_PriceIndex;
    const u32 orderIndex = orderInfo.m_OrderIndex;

    PriceLevel& priceLevel = orderInfo.m_Side == Side::Buy ? m_BuyLevels[priceIndex] : m_SellLevels[priceIndex];
    Order& order = m_OrderPool[orderIndex];

    order.m_Quantity -= tradedQuantity;
    priceLevel.m_TotalQuantity -= tradedQuantity;
}

u32 OrderBook::AllocateOrder()
{
    if (m_FreeOrderHead != INVALID_ORDER)
    {
        const u32 orderIndex = m_FreeOrderHead;
        m_FreeOrderHead = m_OrderPool[orderIndex].m_Next;

        return orderIndex;
    }

    const u32 orderIndex = static_cast<u32>(m_OrderPool.size());
    m_OrderPool.emplace_back();

    return orderIndex;
}

void OrderBook::FreeOrder(u32 orderIndex)
{
    Order& order = m_OrderPool[orderIndex];
    order.m_Next = m_FreeOrderHead;
    m_FreeOrderHead = orderIndex;
}

void OrderBook::AddOrderToLevel(u64 orderId, Side side, u64 priceIndex, u32 quantity)
{
    const u32 orderIndex = AllocateOrder();
    Order& order = m_OrderPool[orderIndex];
    order = Order{ orderId, quantity, INVALID_ORDER, INVALID_ORDER };
    PriceLevel& priceLevel = side == Side::Buy ? m_BuyLevels[priceIndex] : m_SellLevels[priceIndex];
    
    const bool wasEmpty = priceLevel.m_Head == INVALID_ORDER;
    if (wasEmpty) //empty
    {
        priceLevel.m_Head = orderIndex;
        priceLevel.m_Tail = orderIndex;
    }
    else
    {
        Order& tailOrder = m_OrderPool[priceLevel.m_Tail];

        tailOrder.m_Next = orderIndex;
        order.m_Prev = priceLevel.m_Tail;
        priceLevel.m_Tail = orderIndex;
    }
    priceLevel.m_TotalQuantity += order.m_Quantity;

    m_OrderLookup.try_emplace(orderId, priceIndex, orderIndex, side);

    if (!wasEmpty)
    {
        return;
    }

    if (side == Side::Buy)
    {
        SetOccupied(m_BuyBitMap, m_BuySummary, priceIndex);
    }
    else
    {
        SetOccupied(m_SellBitMap, m_SellSummary, priceIndex);
    }
}

void OrderBook::AddOrder(u64 orderId, Side side, Price price, u32 quantity)
{
    if (!IsValidPrice(price) || quantity == 0)
    {
        return;
    }

    MatchOrder(orderId, side, price, quantity);

    if (quantity > 0)
    {
        AddOrderToLevel(orderId, side, PriceToIndex(price), quantity);
    }
}

void OrderBook::CancelOrder(u64 orderID)
{
    const auto it = m_OrderLookup.find(orderID);

    if (it == m_OrderLookup.end())
    {
        return;
    }

    RemoveOrder(it->second);
}

void OrderBook::RemoveOrder(const OrderInfo& orderInfo)
{
    const u32 orderIndex = orderInfo.m_OrderIndex;
    const u64 priceIndex = orderInfo.m_PriceIndex;

    Order& order = m_OrderPool[orderIndex];
    PriceLevel& priceLevel = orderInfo.m_Side == Side::Buy ? m_BuyLevels[priceIndex] : m_SellLevels[priceIndex];

    if (order.m_Prev != INVALID_ORDER) //not head, there is a previous order
    {
        Order& prevOrder = m_OrderPool[order.m_Prev];
        prevOrder.m_Next = order.m_Next;
    }
    else //order was head
    {
        priceLevel.m_Head = order.m_Next;
    }

    if (order.m_Next != INVALID_ORDER) //not tail, there is a next order
    {
        Order& nextOrder = m_OrderPool[order.m_Next];
        nextOrder.m_Prev = order.m_Prev;
    }
    else //order was tail
    {
        priceLevel.m_Tail = order.m_Prev;
    }

    priceLevel.m_TotalQuantity -= order.m_Quantity;

    m_OrderLookup.erase(order.m_ID);

    FreeOrder(orderIndex);

    if (priceLevel.m_Head != INVALID_ORDER) //not empty
    {
        return;
    }

    if (orderInfo.m_Side == Side::Buy)
    {
        ClearOccupied(m_BuyBitMap, m_BuySummary, priceIndex);
    }
    else
    {
        ClearOccupied(m_SellBitMap, m_SellSummary, priceIndex);
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
