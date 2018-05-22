#include<iostream>
#include"mytime.h"

int main()
{
    using std::cout;
    using std::endl;
    Time planning;
    Time coding(2, 40);
    Time fixing(5, 55);
    Time total;

    cout << "planning time = ";
    cout << planning;
    cout << endl;

    cout << "coding time = ";
    cout << coding;
    cout << endl;

    cout << "fixing time = ";
    cout << fixing;
    cout << endl;

    total = coding + fixing;
    cout << "coding + fixing = ";
    cout << total;
    cout << endl;

    Time morefixing(3, 28);
    cout << "more fixing time = ";
    cout << morefixing;
    cout << endl;
    total = morefixing.operator+(total);
    cout << total;
    cout << endl;

    return 0;
}


