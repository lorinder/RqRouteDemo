#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include <time.h>

/**	@file	timeutil.h
 *
 *	Utilities related to timekeeping
 */

void sleep_frac(double time_sec);

double get_delta_seconds(const struct timespec* a,
			const struct timespec* b);

void add_delta_to(struct timespec* ts, double delta);

void get_timespec_from_seconds(struct timespec* ts, double secs);

#endif /* TIMEUTIL_H */
