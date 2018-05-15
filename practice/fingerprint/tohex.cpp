#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
using namespace std;
int main ()
{
    int i = 63;
    char buffer[4];
    unsigned char cmd[2];
    sprintf(buffer, "%x", i);
    puts(buffer);
    for (int b = 0; b < 4; b++)
        cout << buffer[b] << endl;
    cmd[0] = 0x00;
    cmd[1] = buffer[0]*16 + buffer[1];
    cout << hex << (unsigned int)(unsigned char)cmd[0] << endl;
    cout << hex << (unsigned int)(unsigned char)cmd[1] << endl;
    return 0;
}
