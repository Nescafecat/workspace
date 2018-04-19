#include<stdio.h>
#include<string.h>
#define DENSITY 62.4
int main()
{
	float weight, volume;
	int size, letters;
	char name[40];
	
	printf("your name:\n");
	scanf("%s", name);
	printf("%s, your weight:\n", name);
	scanf("%f", &weight);
	size = sizeof name;
	letters = strlen(name);
	volume = weight / DENSITY;
	printf("%s, your volume is %2.2f cubic feet.\n", name, volume);
	printf("%d letters,\n", letters);
	printf("%d bytes.\n", size);

	return 0;
}

