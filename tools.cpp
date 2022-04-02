#include "tools.h"


int max(int a, int b)
{
	if (a>b) return a;
	return b;
}

int min(int a, int b)
{
	if (a<b) return a;
	return b;
}

int abs(int a)
{
	if (a<0) return -a;
	return a;
}

