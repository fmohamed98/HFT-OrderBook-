#pragma once

#include "Defines.h"
#include <vector>
#include <map>

struct Order
{
     u32 m_ID;
     bool m_IsBuy;
     double m_Price;
     u32 m_Quantity;
};

class OrderBook
{
private:
    std::map<double, std::vector<Order>> m_BuyOrders;
    std::map<double, std::vector<Order>> m_SellOrders;

    void MatchBuy(Order& order);
    void MatchSell(Order& order);

public:
    void AddOrder(Order order);
};

