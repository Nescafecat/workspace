#include <iostream>
using namespace std;

int main()
{
    const int array_size = 128;
    int ia[array_size];
    int ival, icnt = 0;
    int sum, ix;

    while (cin >> ival && icnt < array_size)
        ia[icnt++] = ival;
    for (sum = 0, ix = 0; ix < icnt; ++ix)
        sum += ia[ix];
    
    int average = sum / icnt;

    cout << "sum of " << icnt
         << " elements: " << sum
         << ". avergae: " << average << endl;
}
