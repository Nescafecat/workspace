#include <iostream>
#include <algorithm>
using namespace std;
char str[17] = {"0123456789ABCDEF"};

int main()
{
    int n, k;// 0<=n<=2^31-1, 2<=k<=16
    int i = 0, j;
    char rec[80];
    cin >> n >> k;

    while(n>0)
    {
        //rec[i] = str[n % k];
        rec.push_back(str[n % k]);
        n = n / k;
        //i++;
    }
    cout <<'"';
    /*for (j = i -1; j >= 0; j--)
    {
        cout << rec[j];
    }*/
    reverse(rec.begin(),rec.end());
    fot ()
    cout <<'"';
    cout << endl;
    return 0;
}

