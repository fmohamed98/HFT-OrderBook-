#include "OrderBook.h"
#include <print>

void OrderBook::MatchBuy(Order& incoming)
{
    for (Price price = MIN_PRICE; price <= incoming.m_Price; price++)
    {
        if (incoming.m_Quantity == 0)
        {
            return;
        }

        PriceLevel& sellLevel = m_SellLevels[PriceToIndex(price)];
        if (sellLevel.m_Orders.empty())
        {
            continue;
        }

        MatchOrder(incoming, sellLevel);
    }
}

void OrderBook::MatchSell(Order& incoming)
{
    for (Price price = MAX_PRICE; price >= incoming.m_Price; price--)
    {
        if (incoming.m_Quantity == 0)
        {
            return;
        }

        PriceLevel& buyLevel = m_BuyLevels[PriceToIndex(price)];
        if (buyLevel.m_Orders.empty())
        {
            continue;
        }

        MatchOrder(incoming, buyLevel);
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

void OrderBook::AddOrder(Order order)
{
    if (!IsValidPrice(order.m_Price))
    {
        return;
    }

    if (order.m_Side == Side::Buy)
    {
        MatchBuy(order);
        if (order.m_Quantity > 0)
        {
            m_BuyLevels[PriceToIndex(order.m_Price)].m_Orders.push_back(order);
            m_BuyLevels[PriceToIndex(order.m_Price)].m_TotalQuantity += order.m_Quantity;
        }
    }
    else
    {
        MatchSell(order);
        if (order.m_Quantity > 0)
        {
            m_SellLevels[PriceToIndex(order.m_Price)].m_Orders.push_back(order);
            m_SellLevels[PriceToIndex(order.m_Price)].m_TotalQuantity += order.m_Quantity;
        } 
    }
}
