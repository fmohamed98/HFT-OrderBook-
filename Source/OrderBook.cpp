#include "OrderBook.h"
#include <print>

void OrderBook::MatchBuy(Order& incoming)
{
    if (incoming.m_Quantity == 0)
    {
        return;
    }

    auto priceLevel = m_SellOrders.begin();

    while (priceLevel != m_SellOrders.end())
    {
        double sellPrice = priceLevel->first;

        // No more sellers can match.
        if (sellPrice > incoming.m_Price)
        {
            break;
        }
            
        std::vector<Order>& sellOrders = priceLevel->second;

        for (auto it = sellOrders.begin(); it != sellOrders.end(); )
        {
            Order& sellOrder = *it;

            u32 tradedQuantity = std::min(sellOrder.m_Quantity, incoming.m_Quantity);
            sellOrder.m_Quantity -= tradedQuantity;
            incoming.m_Quantity -= tradedQuantity;

            if (sellOrder.m_Quantity == 0)
            {
                it = sellOrders.erase(it);
            }
            else
            {
                it++;
            }

            std::print("TRADE {} @ {}\n", tradedQuantity, sellOrder.m_Price);
        }

        if (sellOrders.empty())
        {
            priceLevel = m_SellOrders.erase(priceLevel);
        }
        else
        {
            priceLevel++;
        }   
    }
}

void OrderBook::MatchSell(Order& incoming)
{
    if (incoming.m_Quantity == 0)
    {
        return;
    }

    auto priceLevel = m_BuyOrders.rbegin();

    while (priceLevel != m_BuyOrders.rend())
    {
        double buyPrice = priceLevel->first;

        // No more buyers can match.
        if (buyPrice < incoming.m_Price)
        {
            break;
        }

        std::vector<Order>& buyOrders = priceLevel->second;

        for (auto it = buyOrders.begin(); it != buyOrders.end(); )
        {
            Order& buyOrder = *it;

            u32 tradedQuantity = std::min(buyOrder.m_Quantity, incoming.m_Quantity);
            buyOrder.m_Quantity -= tradedQuantity;
            incoming.m_Quantity -= tradedQuantity;

            if (buyOrder.m_Quantity == 0)
            {
                it = buyOrders.erase(it);
            }
            else
            {
                it++;
            }

            std::print("TRADE {} @ {}\n", tradedQuantity, buyOrder.m_Price);
        }

        if (buyOrders.empty())
        {
            break;
        }
        else
        {
            priceLevel++;
        }
    }
}

void OrderBook::AddOrder(Order order)
{
    if (order.m_IsBuy)
    {
        MatchBuy(order);
        if (order.m_Quantity > 0)
        {
            m_BuyOrders[order.m_Price].push_back(order);
        }
    }
    else
    {
        MatchSell(order);
        if (order.m_Quantity > 0)
        {
            m_BuyOrders[order.m_Price].push_back(order);
        } 
    }
}
