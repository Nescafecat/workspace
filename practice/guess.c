#include<stdio.h>
int main(void)
{
	int guess = 1;
	char response;
	printf("your number is %d?\n", guess);
	while ((response = getchar()) != 'y')
	{
		if (response == 'n')
			printf("Well, then, is it %d?\n", ++guess);
		else
			printf("sorry, I don't understand.\n");
		while (getchar() != '\n')
			continue;
	}
	
	return 0;
}
