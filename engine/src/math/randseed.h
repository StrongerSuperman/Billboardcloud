#ifndef RANDSEED_H
#define RANDSEED_H

#include <random>

static unsigned int gen_rand_int(int min, int max)
{
	return rand() % (max - min + 1) + min;
}

static float gen_rand_real(float min, float max)
{
	return min + (max - min) * rand() / (RAND_MAX + 1);
}

#endif // !RANDSEED_H
