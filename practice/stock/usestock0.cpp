#include<iostream>
#include"stock00.h"
const int STKS = 4;
int main()
{
    Stock stocks[STKS] = {
        Stock("NanoSmart", 12, 20.0),
        Stock("Boffo", 200, 2.0),
        Stock("Mono",130, 3.25),
        Stock("Fleep", 60, 6.5)
    };
    std::cout << "Stock holdings:\n";
    int st;
    for (st = 0; st < STKS; st++)
        stocks[st].show();
    const Stock * top = &stocks[0];
    for (st = 1; st < STKS; st++)
        top = &top->topval(stocks[st]);
    std::cout << "\nMost valuable holding:\n";
    top->show();
    return 0;
}
