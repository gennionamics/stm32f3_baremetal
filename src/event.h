#pragma once

#include <stdbool.h>

enum Event {
	EVENT_FRAME,

	EVENT_count
};

extern void event_raise (const enum Event);
extern bool event_test_and_clear (const enum Event event);
