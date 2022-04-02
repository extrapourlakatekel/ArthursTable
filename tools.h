#ifndef TOOLS_H
#define TOOLS_H
#include <assert.h>
#include <math.h>


int max(int a, int b);
int min(int a, int b);
int abs(int a);

template <class T>
inline bool limitinplace(T * value, T lower, T upper) 
{
	bool limited = false;
	if ((*value)<lower) {(*value) = lower; limited = true;}
	if ((*value)>upper) {(*value) = upper; limited = true;}
	return limited;
}

template <class T>
inline T limit(T value, T lower, T upper) 
{
	if (value<lower) value = lower;
	if (value>upper) value = upper;
	return value;
}

inline float sqr(float a) 
{
	return a*a;
}

inline int sqr(int a)
{
	return a * a;
}

template <class T>
inline void swp(T &a, T &b) {
	T d = a; a = b; b = d;
}

inline int numberBetween(int lower, int upper)
{
	return (int)((float)random()/(float)RAND_MAX*float(upper-lower))-lower;
}

#endif