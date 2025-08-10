#pragma once

#define PACKED \
	__attribute__((packed))

#define NELEM(array) \
	(sizeof(array) / sizeof(*(array)))

#define FOREACH(array, iter) \
	for (__typeof__(*(array)) *iter = (array); \
		iter < (array) + NELEM(array); \
		iter++)
