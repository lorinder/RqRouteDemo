#include <assert.h>
#include <math.h>

#include "timeutil.h"

void sleep_frac(double time_sec)
{
	struct timespec wait_dur;
	get_timespec_from_seconds(&wait_dur, time_sec);
	nanosleep(&wait_dur, NULL);
}

double get_delta_seconds(const struct timespec* a,
			const struct timespec* b)
{
	double result = a->tv_sec - b->tv_sec;
	result += (a->tv_nsec - b->tv_nsec) / 1.0e9;
	return result;
}

void add_delta_to(struct timespec* ts, double delta)
{
	int delta_int = floor(delta);
	ts->tv_sec += delta_int;
	double delta_frac = delta - delta_int;
	ts->tv_nsec += delta_frac * 1.0e9;
	if (ts->tv_nsec >= 1000000000) {
		ts->tv_nsec -= 1000000000;
		ts->tv_sec += 1;
		assert(ts->tv_nsec < 1000000000);
	}
}

void get_timespec_from_seconds(struct timespec* ts, double secs)
{
	ts->tv_sec = secs;
	ts->tv_nsec = (secs - ts->tv_sec) * 1e9;
}
