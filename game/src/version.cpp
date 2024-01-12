#include <stdio.h>

void WriteVersion()
{
#ifndef __WIN32__
	FILE* fp = fopen("VERSION.txt", "w");

	if (fp)
	{
		fprintf(fp, "__GAME_VERSION__: %s\n", __GAME_VERSION__);
		fprintf(fp, "%s@%s:%s\n", "aGV4dQ==", __HOSTNAME__, __PWD__);
		fclose(fp);
	}
#endif
}
//martysama0134's 7f12f88f86c76f82974cba65d7406ac8
