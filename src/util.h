#pragma once

#define NELEM(array) \
	(sizeof(array) / sizeof(*(array)))

#define FOREACH(array, iter) \
	for (__typeof__(*(array)) *iter = (array); \
		iter < (array) + NELEM(array); \
		iter++)
