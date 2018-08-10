#ifndef ROTATINGCALIPERS_H
#define ROTATINGCALIPERS_H

#include <math.h>
#include <float.h>

typedef struct point_s {
	float x;
	float y;
} point2d;

static void swap(point2d *arr, int a, int b)
{
	point2d temp = arr[a];
	arr[a] = arr[b];
	arr[b] = temp;
}

static float getDist(point2d p1, point2d p2)
{
	return sqrt((p2.x - p1.x)*(p2.x - p1.x) + (p2.y - p1.y)*(p2.y - p1.y));
}

static float geCross(point2d p0, point2d p1, point2d p2)
{
	return (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
}

static float getDot(point2d p0, point2d p1, point2d p2)
{
	return (p1.x - p0.x)*(p2.x - p0.x) + (p1.y - p0.y)*(p2.y - p0.y);
}

static float angleCmp(point2d p0, point2d p1, point2d p2)
{
	float cross = geCross(p0, p1, p2);
	if (cross == 0) {
		return getDist(p0, p2) - getDist(p0, p1);
	}
	return cross;
}

static void vectorSort(point2d *arr, int left, int right)
{
	int i, mid, last;

	if (left >= right) {
		return;
	}

	mid = (left + right) / 2;
	swap(arr, left, mid);
	last = left;
	for (i = left + 1; i <= right; i++) {
		if (angleCmp(arr[0], arr[i], arr[left]) > 0) {
			swap(arr, i, ++last);
		}
	}
	swap(arr, left, last);
	vectorSort(arr, left, last - 1);
	vectorSort(arr, last + 1, right);
}

/// "graham scan" algorithm
static void getConvex(point2d *arr, int len, int *n)
{
	int i, base, top;

	if (len < 4) {
		*n = len;
		return;
	}
	// base point
	base = 0;
	for (i = 0; i < len; i++) {
		if (arr[i].y == arr[base].y && arr[i].x < arr[base].x) {
			base = i;
		}
		else if (arr[i].y < arr[base].y) {
			base = i;
		}
	}
	swap(arr, base, 0);

	// sort
	vectorSort(arr, 1, len - 1);

	// calculate convex hull
	top = 1;
	for (i = 2; i < len; i++) {
		while (top > 0 && geCross(arr[top - 1], arr[top], arr[i]) <= 0) {
			top--;
		}
		arr[++top] = arr[i];
	}
	*n = top;
}

static void rotatingCalipers(point2d *arr, int len, point2d *rectangle)
{
	int top, down, right = 1, up = 0, left = 0, downlast = 0, rightlast = 0, uplast = 0, leftlast = 0;
	float area = FLT_MAX, dist, X, Y, k;
	point2d temp;

	getConvex(arr, len, &top);
	arr[++top] = arr[0];

	for (down = 0; down < top; down++) {
		// find right
		while (getDot(arr[down], arr[down + 1], arr[right]) <= getDot(arr[down], arr[down + 1], arr[right + 1])) {
			right = (right + 1) % top;
		}

		// find up
		if (down == 0) {
			up = right;
		}
		while (geCross(arr[down], arr[down + 1], arr[up]) <= geCross(arr[down], arr[down + 1], arr[up + 1])) {
			up = (up + 1) % top;
		}

		// find down
		if (down == 0) {
			left = up;
		}
		while (getDot(arr[down], arr[down + 1], arr[left]) >= getDot(arr[down], arr[down + 1], arr[left + 1])) {
			left = (left + 1) % top;
		}

		dist = getDist(arr[down], arr[down + 1]);
		X = geCross(arr[down], arr[down + 1], arr[up]) / dist;
		temp.x = arr[right].x + arr[down].x - arr[left].x;
		temp.y = arr[right].y + arr[down].y - arr[left].y;
		Y = getDot(arr[down], arr[down + 1], temp);

		if (area > X*Y) {
			area = X * Y;
			downlast = down;
			rightlast = right;
			uplast = up;
			leftlast = left;
		}
	}

	// calculte outer rectangle
	if (arr[downlast + 1].y == arr[downlast].y) {
		rectangle[0].x = arr[leftlast].x;
		rectangle[0].y = arr[downlast].y;

		rectangle[1].x = arr[rightlast].x;
		rectangle[1].y = arr[downlast].y;

		rectangle[2].x = arr[rightlast].x;
		rectangle[2].y = arr[uplast].y;

		rectangle[3].x = arr[leftlast].x;
		rectangle[3].y = arr[uplast].y;

	}
	else if (arr[downlast + 1].x == arr[downlast].x) {
		rectangle[0].x = arr[downlast].x;
		rectangle[0].y = arr[leftlast].y;

		rectangle[1].x = arr[downlast].x;
		rectangle[1].y = arr[rightlast].y;

		rectangle[2].x = arr[uplast].x;
		rectangle[2].y = arr[rightlast].y;

		rectangle[3].x = arr[uplast].x;
		rectangle[3].y = arr[leftlast].y;

	}
	else {
		k = (arr[downlast + 1].y - arr[downlast].y) / (arr[downlast + 1].x - arr[downlast].x);

		rectangle[0].x = (k*arr[leftlast].y + arr[leftlast].x - k * arr[downlast].y + k * k*arr[downlast].x) / (k*k + 1.0);
		rectangle[0].y = k * rectangle[0].x + arr[downlast].y - k * arr[downlast].x;

		rectangle[1].x = (k*arr[rightlast].y + arr[rightlast].x - k * arr[downlast].y + k * k*arr[downlast].x) / (k*k + 1.0);
		rectangle[1].y = k * rectangle[1].x + arr[downlast].y - k * arr[downlast].x;

		rectangle[2].x = (k*arr[rightlast].y + arr[rightlast].x - k * arr[uplast].y + k * k*arr[uplast].x) / (k*k + 1.0);
		rectangle[2].y = k * rectangle[2].x + arr[uplast].y - k * arr[uplast].x;

		rectangle[3].x = (k*arr[leftlast].y + arr[leftlast].x - k * arr[uplast].y + k * k*arr[uplast].x) / (k*k + 1.0);
		rectangle[3].y = k * rectangle[3].x + arr[uplast].y - k * arr[uplast].x;
	}
}

#endif // ! ROTATINGCALIPERS_H