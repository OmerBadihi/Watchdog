#include <stdio.h>
#include <stdlib.h>/*exit*/
#include "wdlib.h"
#include <unistd.h>

void WD();

int main(int argc, char *argv[])
{
	if (-1 == KeepMeAlive(argc, argv))
	{
		exit(EXIT_FAILURE);
	}
	WD();

	printf("Sinai I love you!\n");
	return 0;
}

void WD()
{
	size_t count = 50;
	while (1)
	{
		printf("WD is a live\n");
		sleep(1);
		--count;
	}
}
