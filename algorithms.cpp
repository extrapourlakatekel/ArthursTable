#include "algorithms.h"

typedef struct {int16_t x; int16_t y;} Point16;
typedef struct {int16_t x1, y1, x2, y2;} Gap16;

#define FLOODBUFFERSIZE 5000000
#define LEEBUFFERSIZE 5000000

void floodfill(uint16_t * overlay, bool * wall, int width, int height, QPoint seed, int radius)
{
	// FIXME: Need much faster algorithm!
	qDebug("floodfill: flooding");
	assert(seed.x() >= 0);
 	assert(seed.x() < width);
	assert(seed.y() >= 0);
	assert(seed.y() < height);
	if (width == 0 || height == 0) return; 
	Point16 * stack;
	Point16 s;
	s.x = seed.x();
	s.y = seed.y();
	stack = new Point16[FLOODBUFFERSIZE];
	int stackpos = 0;
	int maxstackpos = 0;
	stack[stackpos].x = seed.x();
	stack[stackpos].y = seed.y();
	stackpos++;
	while (stackpos > 0) {
		Point16 p = stack[stackpos-1];
		int idx =  (int32_t)p.y * width + p.x;
		if (wall[idx] || overlay[idx] > 0 || sqr(p.x-s.x)+sqr(p.y-s.y) > sqr(radius) ) stackpos--;
		else {
			assert(idx<width*height);
			overlay[idx] = 256;
			if (p.x>0)        {stack[stackpos].x = p.x-1; stack[stackpos].y = p.y; stackpos++;}
			if (p.x<width-1)  {stack[stackpos].x = p.x+1; stack[stackpos].y = p.y; stackpos++;}
			if (p.y>0)        {stack[stackpos].x = p.x; stack[stackpos].y = p.y-1; stackpos++;}
			if (p.y<height-1) {stack[stackpos].x = p.x; stack[stackpos].y = p.y+1; stackpos++;}
			assert(stackpos <= FLOODBUFFERSIZE);
			if (stackpos > maxstackpos) maxstackpos = stackpos;
		}
		if (stackpos > FLOODBUFFERSIZE-4) {
			// FIXME: Do buffer consolidation here
			qWarning("floodfill: flood buffer exceeded"); 
			break;
		} 
	}
	delete[] stack;
	qDebug("floodfill: Needed %d stack entries", maxstackpos);
}

void illuminate(uint16_t * overlay, bool * wall, int width, int height, QPoint seed, int radius)
{
	qDebug("illuminate: illuminating");
	if (radius == 0) radius = 2000; // Maximum radius to save CPU power
	assert(seed.x() >= 0);
	assert(seed.x() < width);
	assert(seed.y() >= 0);
	assert(seed.y() < height);
	assert(malloc_usable_size((void*)overlay) >= width*height);
	assert(malloc_usable_size((void*)wall) >= width*height);
	if (width == 0 || height == 0) return; 
	QVector <Gap16> oldgap, newgap;
	Gap16 gap;
	int dist;
	int mode;
	// Check whether we should start in a wall
	if (wall[seed.y() * width + seed.x()]) return;
	// The dot in the center that otherwise would stay black
	overlay[seed.y() * width + seed.x()] = 256;
	// Left first
	if (seed.x() > 0) {
		gap.x1 = gap.x2 = seed.x() - 1;
		gap.y1 = max(seed.y() - 1, 0);
		gap.y2 = min(seed.y() + 1, height-1);
		oldgap << gap;
		dist = 1;
		while (oldgap.size() > 0 && seed.x() - dist > 0 && dist <= radius) {
			// Check from left end of gap to right end of gap
			newgap.clear();
			for (int i=0; i<oldgap.size(); i++) {
				// Process each gap seperately
				int dx1 = oldgap[i].x1 - seed.x();
				int dy1 = oldgap[i].y1 - seed.y();
				int dx2 = oldgap[i].x2 - seed.x();
				int dy2 = oldgap[i].y2 - seed.y();
				//qDebug("illuminate: dx1: %d, dy1: %d, dx2: %d, dy2:%d", dx1, dy1, dx2, dy2);
				float s1 = (float)-dist / (float)dx1 * (float) dy1 + seed.y();
				float s2 = (float)-dist / (float)dx2 * (float) dy2 + seed.y();
				int starty = max(ceil(s1), 0);
				int endy   = min(floor(s2), height - 1);
				//qDebug("illuminate: starty: %d, endy: %d", starty, endy);
				mode = 0;
				int x = seed.x() - dist;
				gap = oldgap[i];
				for (int y = starty; y <= endy; y++) {
					int idx = y * width + x;
					assert(idx>=0);
					assert(idx<width*height);
					if (wall[idx]) {
						if (mode == 0) {gap.x1 = x; gap.y1 = y;}
						if (mode == 1) {
							gap.x2 = x; 
							gap.y2 = y; 
							newgap << gap; 
							mode = 0;
							gap.x1 = x;
							gap.y1 = y;
							gap.x2 = oldgap[i].x2;
							gap.y2 = oldgap[i].y2;
							}
					} else {
						// Dim the light slowly on the last 50% of the radius
						float cc = limit(2.0-2.0*sqrt(sqr(x-seed.x()) + sqr(y-seed.y()))/radius, 0.0, 1.0);
						overlay[idx] = 256.0 * cc;
						mode = 1; // The next black pixel is the _end_ of a gap
					}
				}
				if (mode == 1) newgap << gap;
			}
			//qDebug("illuminate: found %d gaps:", newgap.size());
			//for (int i=0; i<newgap.size(); i++) qDebug("  %d/%d %d/%d", newgap[i].x1, newgap[i].y1, newgap[i].x2, newgap[i].y2);
			oldgap = newgap;
			dist++;
		}
	}
	// Then right
	oldgap.clear();
	if (seed.x() < width-1) {
		gap.x1 = gap.x2 = seed.x() + 1;
		gap.y1 = max(seed.y() - 1, 0);
		gap.y2 = min(seed.y() + 1, height-1);
		oldgap << gap;
		dist = 1;
		while (oldgap.size() > 0 && seed.x() + dist < width-1 && dist <= radius) {
			// Check from left end of gap to right end of gap
			newgap.clear();
			for (int i=0; i<oldgap.size(); i++) {
				//for (int i=0; i<1; i++) {
				// Process each gap seperately
				int dx1 = oldgap[i].x1 - seed.x();
				int dy1 = oldgap[i].y1 - seed.y();
				int dx2 = oldgap[i].x2 - seed.x();
				int dy2 = oldgap[i].y2 - seed.y();
				//qDebug("illuminate: dx1: %d, dy1: %d, dx2: %d, dy2:%d", dx1, dy1, dx2, dy2);
				float s1 = (float)dist / (float)dx1 * (float) dy1 + seed.y();
				float s2 = (float)dist / (float)dx2 * (float) dy2 + seed.y();
				int starty = max(ceil(s1), 0);
				int endy   = min(floor(s2), height - 1);
				//qDebug("illuminate: starty: %d, endy: %d", starty, endy);
				mode = 0;
				int x = seed.x() + dist;
				gap = oldgap[i];
				for (int y = starty; y <= endy; y++) {
					int idx = y * width + x;
					assert(idx>=0);
					assert(idx<width*height);
					if (wall[idx]) {
						if (mode == 0) {gap.x1 = x; gap.y1 = y;}
						if (mode == 1) {
							gap.x2 = x; 
							gap.y2 = y; 
							newgap << gap; 
							mode = 0;
							gap.x1 = x;
							gap.y1 = y;
							gap.x2 = oldgap[i].x2;
							gap.y2 = oldgap[i].y2;
						}
					} else {
						float cc = limit(2.0-2.0*sqrt(sqr(x-seed.x()) + sqr(y-seed.y()))/radius, 0.0, 1.0); 
						overlay[idx] = 256.0 * cc;
						mode = 1; // The next black pixel is the _end_ of a gap
					}
				}
				if (mode == 1) newgap << gap;
			}
			//qDebug("illuminate: found %d gaps:", newgap.size());
			//for (int i=0; i<newgap.size(); i++) qDebug("  %d/%d %d/%d", newgap[i].x1, newgap[i].y1, newgap[i].x2, newgap[i].y2);
			oldgap = newgap;
			dist++;
		}
	} 
	// Then up
	oldgap.clear();
	if (seed.y() > 0) {
		gap.x1 = max(seed.x() - 1, 0);
		gap.x2 = min(seed.x() + 1, width-1);
		gap.y1 = gap.y2 = seed.y() - 1;
		oldgap << gap;
		dist = 1;
		while (oldgap.size() > 0 && seed.y() - dist > 0 && dist <= radius) {
			// Check from left end of gap to right end of gap
			newgap.clear();
			for (int i=0; i<oldgap.size(); i++) {
				// Process each gap seperately
				int dx1 = oldgap[i].x1 - seed.x();
				int dy1 = oldgap[i].y1 - seed.y();
				int dx2 = oldgap[i].x2 - seed.x();
				int dy2 = oldgap[i].y2 - seed.y();
				//qDebug("illuminate: dx1: %d, dy1: %d, dx2: %d, dy2:%d", dx1, dy1, dx2, dy2);
				float s1 = (float)-dist / (float)dy1 * (float) dx1 + seed.x();
				float s2 = (float)-dist / (float)dy2 * (float) dx2 + seed.x();
				float c1 = ceil(s1) - s1;  // For anti-aliasing effect
				float c2 = s2 - floor(s2); // For anti-aliasing effect
				c1 = 1.0; c2 = 1.0;
				
				int startx = max(ceil(s1), 0);
				int endx   = min(floor(s2), width - 1);
				//qDebug("illuminate: starty: %d, endy: %d", starty, endy);
				mode = 0;
				int y = seed.y() - dist;
				gap.x1 = oldgap[i].x1;
				gap.y1 = oldgap[i].y1;
				gap.x2 = oldgap[i].x2;
				gap.y2 = oldgap[i].y2;
				for (int x = startx; x <= endx; x++) {
					int idx = y * width + x;
					assert(idx>=0);
					assert(idx<width*height);
					if (wall[idx]) {
						if (mode == 0) {gap.x1 = x; gap.y1 = y;}
						if (mode == 1) {
							gap.x2 = x; 
							gap.y2 = y; 
							newgap << gap; 
							mode = 0;
							gap.x1 = x;
							gap.y1 = y;
							gap.x2 = oldgap[i].x2;
							gap.y2 = oldgap[i].y2;
							}
					} else {
						// Dim the light slowly on the last 20% of the radius
						float cc = limit(2.0-2.0*sqrt(sqr(x-seed.x()) + sqr(y-seed.y()))/radius, 0.0, 1.0);
						if (x == startx) overlay[idx] = 256.0 * c1 * cc;
						else if (x == endx) overlay[idx] = 256.0 * c2 * cc;
						else overlay[idx] = 256.0 * cc;
						mode = 1; // The next black pixel is the _end_ of a gap
					}
				}
				if (mode == 1) newgap << gap;
			}
			//qDebug("illuminate: found %d gaps:", newgap.size());
			//for (int i=0; i<newgap.size(); i++) qDebug("  %d/%d %d/%d", newgap[i].x1, newgap[i].y1, newgap[i].x2, newgap[i].y2);
			oldgap = newgap;
			dist++;
		}
	} 
	// Last but not leat: bottom!
	oldgap.clear();
	if (seed.y() < height - 1) {
		gap.x1 = max(seed.x() - 1, 0);
		gap.x2 = min(seed.x() + 1, width-1);
		gap.y1 = gap.y2 = seed.y() + 1;
		oldgap << gap;
		dist = 1;
		while (oldgap.size() > 0 && seed.y() + dist < height - 1 && dist <= radius) {
			// Check from left end of gap to right end of gap
			newgap.clear();
			for (int i=0; i<oldgap.size(); i++) {
				// Process each gap seperately
				int dx1 = oldgap[i].x1 - seed.x();
				int dy1 = oldgap[i].y1 - seed.y();
				int dx2 = oldgap[i].x2 - seed.x();
				int dy2 = oldgap[i].y2 - seed.y();
				//qDebug("illuminate: dx1: %d, dy1: %d, dx2: %d, dy2:%d", dx1, dy1, dx2, dy2);
				float s1 = (float)dist / (float)dy1 * (float) dx1 + seed.x();
				float s2 = (float)dist / (float)dy2 * (float) dx2 + seed.x();
				//float c1 = ceil(s1) - s1;  // For anti-aliasing effect
				//float c2 = s2 - floor(s2); // For anti-aliasing effect
				int startx = max(ceil(s1), 0);
				int endx   = min(floor(s2), width - 1);
				//qDebug("illuminate: starty: %d, endy: %d", starty, endy);
				mode = 0;
				int y = seed.y() + dist;
				gap.x1 = oldgap[i].x1;
				gap.y1 = oldgap[i].y1;
				gap.x2 = oldgap[i].x2;
				gap.y2 = oldgap[i].y2;
				for (int x = startx; x <= endx; x++) {
					int idx = y * width + x;
					assert(idx>=0);
					assert(idx<width*height);
					if (wall[idx]) {
						if (mode == 0) {gap.x1 = x; gap.y1 = y;}
						if (mode == 1) {
							gap.x2 = x; 
							gap.y2 = y; 
							newgap << gap; 
							mode = 0;
							gap.x1 = x;
							gap.y1 = y;
							gap.x2 = oldgap[i].x2;
							gap.y2 = oldgap[i].y2;
							}
					} else {
						// Dim the light slowly on the last 20% of the radius
						float cc = limit(2.0-2.0*sqrt(sqr(x-seed.x()) + sqr(y-seed.y()))/radius, 0.0, 1.0);
						overlay[idx] = 256.0 * cc;
						mode = 1; // The next black pixel is the _end_ of a gap
					}
				}
				if (mode == 1) newgap << gap;
			}
			//qDebug("illuminate: found %d gaps:", newgap.size());
			//for (int i=0; i<newgap.size(); i++) qDebug("  %d/%d %d/%d", newgap[i].x1, newgap[i].y1, newgap[i].x2, newgap[i].y2);
			oldgap = newgap;
			dist++;
		}
	}
}

void leeFromImage(uint16_t * overlay, bool * wall, int width, int height, int decrease)
{
	qDebug("leeFromImage: leeing");
	assert(malloc_usable_size((void*)overlay) >= width*height);
	assert(malloc_usable_size((void*)wall) >= width*height);
	Point16 * stack;
	stack = new Point16[LEEBUFFERSIZE];
	int stackpos = 0;
	int maxstackpos = 0;
	int diag = decrease * sqrt(2.0);
	if (width == 0 || height == 0) return; // No valid image
	// Do not process to the exact image border - sloppy but faster
	for (int y=1; y < height-1; y++) {
		int idx = y*width;
		for (int x=1; x<width-1; x++) {
			idx++;
			assert(idx<width*height-width-1);
			assert(idx>=width+1);
			// Don't bother checking black pixels or walls
			if (overlay[idx] > decrease && !wall[idx]) {
				int16_t o = overlay[idx] - decrease;
				int16_t d = max(overlay[idx] - diag, 0);
				//int16_t o = overlay[idx]/2;
				//int16_t d = overlay[idx]/3;
				if (stackpos > LEEBUFFERSIZE - 10) {
					qWarning("leeFromImage: buffer exceeded");
					return;
				}
				if (!wall[idx - 1 - width] && overlay[idx - 1 - width] < d) {overlay[idx - 1 - width] = d; stack[stackpos].x = x - 1; stack[stackpos++].y = y - 1;}
				if (!wall[idx - 1        ] && overlay[idx - 1        ] < o) {overlay[idx - 1        ] = o; stack[stackpos].x = x - 1; stack[stackpos++].y = y    ;}
				if (!wall[idx - 1 + width] && overlay[idx - 1 + width] < d) {overlay[idx - 1 + width] = d; stack[stackpos].x = x - 1; stack[stackpos++].y = y + 1;}
				if (!wall[idx     - width] && overlay[idx     - width] < o) {overlay[idx     - width] = o; stack[stackpos].x = x    ; stack[stackpos++].y = y - 1;}
				if (!wall[idx     + width] && overlay[idx     + width] < o) {overlay[idx     + width] = o; stack[stackpos].x = x    ; stack[stackpos++].y = y + 1;}
				if (!wall[idx + 1 - width] && overlay[idx + 1 - width] < d) {overlay[idx + 1 - width] = d; stack[stackpos].x = x + 1; stack[stackpos++].y = y - 1;}
				if (!wall[idx + 1        ] && overlay[idx + 1        ] < o) {overlay[idx + 1        ] = o; stack[stackpos].x = x + 1; stack[stackpos++].y = y    ;}
				if (!wall[idx + 1 + width] && overlay[idx + 1 + width] < d) {overlay[idx + 1 + width] = d; stack[stackpos].x = x + 1; stack[stackpos++].y = y + 1;}
			}
		}
	}

	while (stackpos > 0) {
		stackpos--;
		int x = stack[stackpos].x;
		int y = stack[stackpos].y;
		if (x>0 && x<width-1 && y>0 && y<height-1) {
			int idx = y * width + x;
			assert(idx<width*height-width-1);
			assert(idx>=width+1);
			// Don't bother checking black pixels or walls
			if (overlay[idx] > decrease && !wall[idx]) {
				int16_t o = overlay[idx] - decrease;
				int16_t d = max(overlay[idx] - diag, 0);
				if (stackpos > LEEBUFFERSIZE - 10) {
					qWarning("leeFromImage: buffer exceeded");
					return;
				}
				if (!wall[idx - 1 - width] && overlay[idx - 1 - width] < d) {overlay[idx - 1 - width] = d; stack[stackpos].x = x - 1; stack[stackpos++].y = y - 1;}
				if (!wall[idx - 1        ] && overlay[idx - 1        ] < o) {overlay[idx - 1        ] = o; stack[stackpos].x = x - 1; stack[stackpos++].y = y    ;}
				if (!wall[idx - 1 + width] && overlay[idx - 1 + width] < d) {overlay[idx - 1 + width] = d; stack[stackpos].x = x - 1; stack[stackpos++].y = y + 1;}
				if (!wall[idx     - width] && overlay[idx     - width] < o) {overlay[idx     - width] = o; stack[stackpos].x = x    ; stack[stackpos++].y = y - 1;}
				if (!wall[idx     + width] && overlay[idx     + width] < o) {overlay[idx     + width] = o; stack[stackpos].x = x    ; stack[stackpos++].y = y + 1;}
				if (!wall[idx + 1 + width] && overlay[idx + 1 + width] < d) {overlay[idx + 1 - width] = d; stack[stackpos].x = x + 1; stack[stackpos++].y = y - 1;}
				if (!wall[idx + 1        ] && overlay[idx + 1        ] < o) {overlay[idx + 1        ] = o; stack[stackpos].x = x + 1; stack[stackpos++].y = y    ;}
				if (!wall[idx + 1 + width] && overlay[idx + 1 + width] < d) {overlay[idx + 1 + width] = d; stack[stackpos].x = x + 1; stack[stackpos++].y = y + 1;}
			}
		}
	}
	delete[] stack;
	qDebug("leeFromImage: leeing done");
}