#include<stdio.h>
#define NUMBER 9
//long fact(int n);

long fact(int n)
{
	long ans;
	if (n > 0)
		ans = n * fact(n - 1);
	else
		ans = 1;	
	return ans;
}

int main(void)
{
	long num;
	num = fact(NUMBER);
	
	printf("%ld \n", num);

	return 0;
}
/*
long fact(int n)
{
	long ans;
	if (n > 0)
		ans = n * fact(n - 1);
	else
		ans = 1;	
	return ans;
}
*/
