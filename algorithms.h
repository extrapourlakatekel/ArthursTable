#ifndef ALGORITHMS_H
#define ALGORITHMS_H
#include <assert.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <QPoint>
#include <QVector>
#include "tools.h"


void floodfill(uint16_t * overlay, bool * wall, int width, int height, QPoint seed, int radius);
void illuminate(uint16_t * overlay, bool * wall, int width, int height, QPoint seed, int radius);
void leeFromImage(uint16_t * overlay, bool * wall, int width, int height, int decrease);

#endif