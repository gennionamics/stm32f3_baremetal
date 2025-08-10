#include <stdint.h>

// Initialize the LEDs, timers and DMA transfers.
extern void led_init (void);

// Set the brightness for a single LED (0..7) in the range 0..15.
extern void led_set (const uint8_t led, const uint8_t level);

// Set the brightness for all LEDs.
extern void led_set_all (const uint8_t *levels);

// Set the LED circle to indicate a given binary angle (0-255 deg).
extern void led_set_angle (const uint8_t brad);
