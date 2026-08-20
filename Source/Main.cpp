// HFTStuff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "OrderBook.h"

int main()
{
    OrderBook orderBook;
    orderBook.AddOrder({1, Side::Sell, ToPrice(101.23) , 100});
    orderBook.AddOrder({2, Side::Sell, ToPrice(102.25), 200});
    orderBook.AddOrder({3, Side::Buy, ToPrice(101.24), 50});
}

