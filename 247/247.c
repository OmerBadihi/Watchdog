#include <stdio.h>
#include <stdlib.h>/*exit*/
#include "wdlib.h"
#include <unistd.h>

void ATM();

int main(int argc, char *argv[])
{
	if (-1 == KeepMeAlive(argc, argv))
	{
		exit(EXIT_FAILURE);
	}
	ATM();
	LetMeDie(argc, argv);

	printf("Sinai I love you!\n");
	return 0;
}

void ATM()
{
	size_t count = 50;
	while (count)
	{
		printf("ATM is a live\n");
		sleep(1);
		--count;
	}
}
