// HFTStuff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "OrderBook.h"

int main()
{
    OrderBook orderBook;
    orderBook.AddOrder({1, false, 101.0f, 100});
    orderBook.AddOrder({2, false, 102.0f, 200});
    orderBook.AddOrder({3, true, 101.0f, 50});
}

