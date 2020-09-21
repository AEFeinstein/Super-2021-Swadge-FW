/*
 * mode_dance.c
 *
 *  Created on: Nov 10, 2018
 *      Author: adam
 */

/*============================================================================
 * Includes
 *==========================================================================*/

#include <osapi.h>
#include <stdint.h>
#include <user_interface.h>

#include "ccconfig.h"
#include "user_main.h"
#include "user_main.h"
#include "embeddedout.h"
#include "mode_dance.h"
#include "hsv_utils.h"

/*============================================================================
 * Prototypes
 *==========================================================================*/

void ICACHE_FLASH_ATTR danceEnterMode(void);
void ICACHE_FLASH_ATTR danceExitMode(void);
void ICACHE_FLASH_ATTR danceClearVars(void);
void ICACHE_FLASH_ATTR danceButtonCallback(uint8_t state, int button, int down);
void ICACHE_FLASH_ATTR setDanceLeds(led_t* ledData, uint8_t ledDataLen);
uint32_t ICACHE_FLASH_ATTR dance_rand(uint32_t upperBound);

void ICACHE_FLASH_ATTR unlockAnimation(void* arg);

void ICACHE_FLASH_ATTR freeze_color(void* arg);
void ICACHE_FLASH_ATTR random_dance_mode(void* arg);

void ICACHE_FLASH_ATTR danceTimerMode1(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode2(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode3(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode4(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode5(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode6(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode7(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode8(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode9(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode10(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode11(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode12(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode13(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode14(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode15(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode16(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode17(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode18(void* arg);
void ICACHE_FLASH_ATTR danceTimerMode19(void* arg);

/*============================================================================
 * Static Const Variables
 *==========================================================================*/

static const uint8_t danceBrightnesses[] =
{
    0x01,
    0x08,
    0x40,
};

/*============================================================================
 * Variables
 *==========================================================================*/

uint8_t danceBrightnessIdx = 0;

/**
 * This is an array of timerWithPeriod structs. Each is a separate animation
 * which is cycled around by button presses
 */
timerWithPeriod danceTimers[] =
{
    {
        .timerFn = danceTimerMode1,
        .period = 100
    },
    {
        .timerFn = danceTimerMode2,
        .period = 100
    },
    {
        .timerFn = danceTimerMode3,
        .period = 100
    },
    {
        .timerFn = danceTimerMode4,
        .period = 5
    },
    {
        .timerFn = danceTimerMode5,
        .period = 100
    },
    {
        .timerFn = danceTimerMode6,
        .period = 80
    },
    {
        .timerFn = danceTimerMode8,
        .period = 300
    },
    {
        .timerFn = danceTimerMode9,
        .period = 100
    },
    {
        .timerFn = danceTimerMode10,
        .period = 100
    },
    {
        .timerFn = danceTimerMode11,
        .period = 100
    },
    {
        .timerFn = danceTimerMode12,
        .period = 100
    },
    {
        .timerFn = danceTimerMode13,
        .period = 2
    },
    {
        .timerFn = danceTimerMode14,
        .period = 10
    },
    {
        .timerFn = danceTimerMode15,
        .period = 100
    },
    {
        .timerFn = danceTimerMode16,
        .period = 120
    },
    {
        .timerFn = danceTimerMode17,
        .period = 100
    },
    {
        .timerFn = danceTimerMode18,
        .period = 7
    },
    {
        .timerFn = danceTimerMode19,
        .period = 10
    },
    {
        .timerFn = danceTimerMode7,
        .period = 70
    },
    {
        .timerFn = freeze_color,
        .period = 40
    },
    // Note this MUST be last so that random mode never calls random mode
    {
        .timerFn = random_dance_mode,
        .period = 1
    }
};

/// This is a state variable used in animations
int ledCount = 0;
int ledCount2 = 0;
int timerCount = 0;
int strobeCount = 0;

int random_dance_timer = 0;
int random_choice = 8;

uint32_t color_save = 256;

uint8_t color_hue_save[NUM_LIN_LEDS] = {0};
uint8_t color_saturation_save[NUM_LIN_LEDS] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t current_color_hue[NUM_LIN_LEDS] = {0};
uint8_t current_color_saturation[NUM_LIN_LEDS] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool led_bool = true;

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * This is called to clear all dance variables
 */
void ICACHE_FLASH_ATTR danceClearVars(void)
{
    // This is a state variable used in animations
    ledCount = 0;
    ledCount2 = 0;
    timerCount = 0;
    strobeCount = 0;

    random_dance_timer = 0;
    random_choice = 8;

    color_save = 256;

    uint8_t i = 0;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        color_hue_save[i] = 0;
        color_saturation_save[i] = 0xFF;
        current_color_hue[i] = 0;
        current_color_saturation[i] = 0xFF;
    }

    led_bool = true;
}

/**
 * Set the brightness index
 *
 * @param brightness index into danceBrightnesses[]
 */
void ICACHE_FLASH_ATTR setDanceBrightness(uint8_t brightness)
{
    if(brightness > 2)
    {
        brightness = 2;
    }
    danceBrightnessIdx = brightness;
}

/**
 * Intermediate function which adjusts brightness and sets the LEDs
 *
 * @param ledData    The LEDs to be scaled, then set
 * @param ledDataLen The length of the LEDs to set
 */
void ICACHE_FLASH_ATTR setDanceLeds(led_t* ledData, uint8_t ledDataLen)
{
    uint8_t i;
    for(i = 0; i < ledDataLen / sizeof(led_t); i++)
    {
        ledData[i].r = ledData[i].r / danceBrightnesses[danceBrightnessIdx];
        ledData[i].g = ledData[i].g / danceBrightnesses[danceBrightnessIdx];
        ledData[i].b = ledData[i].b / danceBrightnesses[danceBrightnessIdx];
    }
    setLeds(ledData, ledDataLen);
}

/**
 * Get a random number from a range.
 *
 * This isn't true-random, unless bound is a power of 2. But it's close enough?
 * The problem is that os_random() returns a number between [0, 2^64), and the
 * size of the range may not be even divisible by bound
 *
 * For what it's worth, this is what Arduino's random() does. It lies!
 *
 * @param bound An upper bound of the random range to return
 * @return A number in the range [0,bound), which does not include bound
 */
uint32_t ICACHE_FLASH_ATTR dance_rand(uint32_t bound)
{
    if(bound == 0)
    {
        return 0;
    }
    return os_random() % bound;
}

/**
 * This animation is set to be called every 100 ms
 * Rotate a single white LED around the hexagon
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode1(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > NUM_LIN_LEDS)
    {
        ledCount = 0;
    }

    // Turn the current LED on, full bright white
    leds[ledCount].r = 255;
    leds[ledCount].g = 255;
    leds[ledCount].b = 255;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * This animation is set to be called every 100ms
 * Blink all LEDs red for on for 500ms, then off for 500ms
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode2(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount < 5)
    {
        // full bright red
        for (int i = 0; i < NUM_LIN_LEDS; i++)
        {
            leds[i].r = 200;
        }
    }
    else
    {
        // off
        for (int i = 0; i < NUM_LIN_LEDS; i++)
        {
            leds[i].r = 0;
        }
    }

    if(ledCount > 10)
    {
        ledCount = 0;
    }

    setDanceLeds(leds, sizeof(leds));
}

/**
 * This animation is set to be called every 100ms
 * Rotates two blue LEDs, one clockwise and one counterclockwise
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode3(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount - 1;
    if(ledCount < 0 || ledCount > NUM_LIN_LEDS - 1)
    {
        ledCount = NUM_LIN_LEDS - 1;
    }

    // Turn the current LED on, full bright blue
    leds[ledCount].r = 0;
    leds[ledCount].g = 0;
    leds[ledCount].b = 255;

    leds[(NUM_LIN_LEDS - ledCount ) % NUM_LIN_LEDS].r = 0;
    leds[(NUM_LIN_LEDS - ledCount ) % NUM_LIN_LEDS].g = 0;
    leds[(NUM_LIN_LEDS - ledCount ) % NUM_LIN_LEDS].b = 255;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 5ms
 * Smoothly rotate a color wheel around the hexagon
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode4(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > 256)
    {
        ledCount = 0;
    }

    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        int16_t angle = (((i * 256) / NUM_LIN_LEDS)) + ledCount % 256;
        uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

        leds[i].r = (color >>  0) & 0xFF;
        leds[i].g = (color >>  8) & 0xFF;
        leds[i].b = (color >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 100ms
 * Sharply rotate a color wheel around the hexagon
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode5(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > NUM_LIN_LEDS - 1)
    {
        ledCount = 0;
    }

    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        int16_t angle = (((i * 256)  / NUM_LIN_LEDS)) % 256;
        uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

        leds[(i + ledCount) % NUM_LIN_LEDS].r = (color >>  0) & 0xFF;
        leds[(i + ledCount) % NUM_LIN_LEDS].g = (color >>  8) & 0xFF;
        leds[(i + ledCount) % NUM_LIN_LEDS].b = (color >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 80ms
 * Rotate a single LED around the hexagon while smoothy iterating its color
 * around the color wheel
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode6(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount - 1;
    if(ledCount < 0 || ledCount > 255)
    {
        ledCount = 255;
    }
    int16_t angle = ledCount % 256;
    uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

    leds[(ledCount) % NUM_LIN_LEDS].r = (color >>  0) & 0xFF;
    leds[(ledCount) % NUM_LIN_LEDS].g = (color >>  8) & 0xFF;
    leds[(ledCount) % NUM_LIN_LEDS].b = (color >> 16) & 0xFF;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 300ms
 * Counts up to 64 in binary. At 64, the color is held for ~3s
 * The 'on' color is smoothly iterated over the color wheel. The 'off'
 * color is also iterated over the color wheel, 180 degrees offset from 'on'
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode8(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 1;
    ledCount2 = ledCount2 + 1;
    if(ledCount2 > 75)
    {
        led_bool = !led_bool;
        ledCount2 = 0;
    }
    if(ledCount > 255)
    {
        ledCount = 0;
    }
    int16_t angle = ledCount % 256;
    uint32_t colorOn = EHSVtoHEX(angle, 0xFF, 0xFF);
    uint32_t colorOff = EHSVtoHEX((angle + 128) % 256, 0xFF, 0xFF);

    uint8_t i;
    uint8_t j;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        if(ledCount2 >= 64)
        {
            leds[i].r = (colorOn >>  0) & 0xFF;
            leds[i].g = (colorOn >>  8) & 0xFF;
            leds[i].b = (colorOn >> 16) & 0xFF;
        }
        else
        {
            if(led_bool)
            {
                j = 6 - i;
            }
            else
            {
                j = i;
            }

            if((ledCount2 >> i) & 1)
            {
                leds[(j) % NUM_LIN_LEDS].r = (colorOn >>  0) & 0xFF;
                leds[(j) % NUM_LIN_LEDS].g = (colorOn >>  8) & 0xFF;
                leds[(j) % NUM_LIN_LEDS].b = (colorOn >> 16) & 0xFF;
            }
            else
            {
                leds[(j) % NUM_LIN_LEDS].r = (colorOff >>  0) & 0xFF;
                leds[(j) % NUM_LIN_LEDS].g = (colorOff >>  8) & 0xFF;
                leds[(j) % NUM_LIN_LEDS].b = (colorOff >> 16) & 0xFF;
            }
        }
    }
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 100ms
 *
 * Fire pattern. All LEDs are random amount of red, and fifth that of green.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode9(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    leds[3].r = dance_rand(120) + 135;
    leds[3].g = leds[3].r / 5;

    leds[4].r = dance_rand(80) + 80;
    leds[4].g = leds[4].r / 5;
    leds[2].r = dance_rand(80) + 80;
    leds[2].g = leds[2].r / 5;

    leds[5].r = dance_rand(50) + 40;
    leds[5].g = leds[5].r / 5;
    leds[1].r = dance_rand(50) + 40;
    leds[1].g = leds[1].r / 5;

    leds[0].r = dance_rand(10) + 10;
    leds[0].g = leds[0].r / 5;

    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 100ms
 *
 * Fire pattern. All LEDs are random amount of green, and fifth that of blue.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode10(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    leds[3].g = dance_rand(120) + 135;
    leds[3].b = leds[3].g / 5;

    leds[4].g = dance_rand(80) + 80;
    leds[4].b = leds[4].g / 5;
    leds[2].g = dance_rand(80) + 80;
    leds[2].b = leds[2].g / 5;

    leds[5].g = dance_rand(50) + 40;
    leds[5].b = leds[5].g / 5;
    leds[1].g = dance_rand(50) + 40;
    leds[1].b = leds[1].g / 5;

    leds[0].g = dance_rand(10) + 10;
    leds[0].b = leds[0].g / 5;

    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 100ms
 *
 * Fire pattern. All LEDs are random amount of blue, and fifth that of green.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode11(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    leds[3].b = dance_rand(120) + 135;
    leds[3].g = leds[3].b / 5;

    leds[4].b = dance_rand(80) + 80;
    leds[4].g = leds[4].b / 5;
    leds[2].b = dance_rand(80) + 80;
    leds[2].g = leds[2].b / 5;

    leds[5].b = dance_rand(50) + 40;
    leds[5].g = leds[5].b / 5;
    leds[1].b = dance_rand(50) + 40;
    leds[1].g = leds[1].b / 5;

    leds[0].b = dance_rand(10) + 10;
    leds[0].g = leds[0].b / 5;

    setDanceLeds(leds, sizeof(leds));
}

/**
 * This animation is set to be called every 100 ms
 * Rotate a single LED around the hexagon, giving it a new random color for each
 * rotation
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode12(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > NUM_LIN_LEDS - 1)
    {
        ledCount = 0;
        color_save = EHSVtoHEX(dance_rand(256), 0xFF, 0xFF);
    }

    // Turn the current LED on, full bright white
    leds[ledCount].r = (color_save >>  0) & 0xFF;
    leds[ledCount].g = (color_save >>  8) & 0xFF;
    leds[ledCount].b = (color_save >> 16) & 0xFF;

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 2 ms
 * Pulse all LEDs smoothly on and off. For each pulse, pick a random color
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode13(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;

    if(ledCount > 510)
    {
        ledCount = 0;
        ledCount2 = dance_rand(256);
    }
    int intensity = ledCount;
    if(ledCount > 255)
    {
        intensity = 510 - ledCount;
    }
    color_save = EHSVtoHEX(ledCount2, 0xFF, intensity);
    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {

        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 10ms
 * Dac's multi-color strobe, strobe R then G then B, 120ms off, 10ms on
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode14(void* arg __attribute__((unused)))
{
    bool resetStrobeCount = false;

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Keep a count of time in 10ms increments
    strobeCount++;

    // Adjust the LEDs
    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        switch(strobeCount)
        {
            case 13:
            {
                // From 130 to 140ms, be red
                leds[i].r = 255;
                leds[i].g = 0;
                leds[i].b = 0;
                break;
            }
            case 27:
            {
                // From 270 to 280ms, be green
                leds[i].r = 0;
                leds[i].g = 255;
                leds[i].b = 0;
                break;
            }
            case 41:
            {
                // From 410 to 420ms, be blue
                leds[i].r = 0;
                leds[i].g = 0;
                leds[i].b = 255;

                resetStrobeCount = true;
                break;
            }
            default:
            {
                // Otherwise be off
                leds[i].r = 0;
                leds[i].g = 0;
                leds[i].b = 0;
                break;
            }
        }
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));

    if(resetStrobeCount)
    {
        strobeCount = 0;
    }
}

/**
 * Called every 100ms
 * Show a static pattern for 30s, then show another static pattern for 30s
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode15(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    timerCount++;
    if (timerCount > 600)
    {
        timerCount = 0;
    }

    if (timerCount < 300)
    {
        // Turn the current LED on GREEN
        leds[0].r = 13;
        leds[0].g = 255;
        leds[0].b = 32;

        leds[1].r = 40;
        leds[1].g = 80;
        leds[1].b = 50;

        leds[2].r = 13;
        leds[2].g = 255;
        leds[2].b = 32;

        leds[3].r = 152;
        leds[3].g = 113;
        leds[3].b = 20;

        leds[4].r = 13;
        leds[4].g = 255;
        leds[4].b = 32;

        leds[5].r = 40;
        leds[5].g = 80;
        leds[5].b = 50;
    }
    else
    {
        // Turn the current LED on RED
        leds[0].r = 255;
        leds[0].g = 32;
        leds[0].b = 32;

        leds[1].r = 80;
        leds[1].g = 50;
        leds[1].b = 50;

        leds[2].r = 255;
        leds[2].g = 32;
        leds[2].b = 32;

        leds[3].r = 152;
        leds[3].g = 113;
        leds[3].b = 20;

        leds[4].r = 255;
        leds[4].g = 32;
        leds[4].b = 32;

        leds[5].r = 80;
        leds[5].g = 50;
        leds[5].b = 50;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));

}

/**
 * Called every 120ms
 * police sirens, flash half red then half blue
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode16(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};

    // Skip to the next LED around the hexagon
    ledCount = ledCount + 1;
    if(ledCount > NUM_LIN_LEDS)
    {
        ledCount = 0;

    }
    //green is 0
    //100 is fuchsia
    //70 is orange-ish
    uint8_t i;
    // Turn the current LED on, full bright white
    if(ledCount < (NUM_LIN_LEDS >> 1))
    {
        uint32_t red = EHSVtoHEX(85, 0xFF, 0xFF);
        for(i = 0; i < (NUM_LIN_LEDS >> 1); i++)
        {
            leds[i].r = (red >>  0) & 0xFF;
            leds[i].g = (red >>  8) & 0xFF;
            leds[i].b = (red >> 16) & 0xFF;
        }
    }
    else
    {
        uint32_t blue = EHSVtoHEX(170, 0xFF, 0xFF);
        for(i = (NUM_LIN_LEDS >> 1); i < NUM_LIN_LEDS; i++)
        {
            leds[i].r = (blue >>  0) & 0xFF;
            leds[i].g = (blue >>  8) & 0xFF;
            leds[i].b = (blue >> 16) & 0xFF;
        }
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 100ms
 * Turn a random LED on to a random color, one at a time
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode17(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    uint32_t rand_color = EHSVtoHEX(dance_rand(255), 0xFF, 0xFF);
    int rand_light = dance_rand(NUM_LIN_LEDS);
    leds[rand_light].r = (rand_color >>  0) & 0xFF;
    leds[rand_light].g = (rand_color >>  8) & 0xFF;
    leds[rand_light].b = (rand_color >> 16) & 0xFF;
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 7ms
 * Christmas lights. Picks random target hues (red or green) and saturations for
 * random LEDs at random intervals, then smoothly iterates towards those targets.
 * All LEDs are shown with a randomness added to their brightness for a little
 * sparkle
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode18(void* arg __attribute__((unused)))
{
    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    ledCount += 1;
    if(ledCount > ledCount2)
    {
        ledCount = 0;
        ledCount2 = dance_rand(1000) + 50; // 350ms to 7350ms
        int color_picker = dance_rand(NUM_LIN_LEDS - 1);
        int node_select = dance_rand(NUM_LIN_LEDS);

        if(color_picker < 2)
        {
            color_hue_save[node_select] = 0;
            color_saturation_save[node_select] = dance_rand(15) + 240;
        }
        else if (color_picker < 4)
        {
            color_hue_save[node_select] = 86;
            color_saturation_save[node_select] = dance_rand(15) + 240;
        }
        else
        {
            color_saturation_save[node_select] = dance_rand(25);
        }
    }

    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        if(current_color_hue[i] > color_hue_save[i])
        {
            current_color_hue[i] -= 1;
        }
        else if (current_color_hue[i] < color_hue_save[i])
        {
            current_color_hue[i] += 1;
        }

        if(current_color_saturation[i] > color_saturation_save[i])
        {
            current_color_saturation[i] -= 1;
        }
        else if (current_color_saturation[i] < color_saturation_save[i])
        {
            current_color_saturation[i] += 1;
        }
    }

    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        leds[i].r = (EHSVtoHEX(current_color_hue[i],  current_color_saturation[i], dance_rand(15) + 240) >>  0) & 0xFF;
        leds[i].g = (EHSVtoHEX(current_color_hue[i],  current_color_saturation[i], dance_rand(15) + 240) >>  8) & 0xFF;
        leds[i].b = (EHSVtoHEX(current_color_hue[i],  current_color_saturation[i], dance_rand(15) + 240) >> 16) & 0xFF;
    }
    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 10ms
 *
 * Chainsaw blade. Use EHSVtoHEX() to smoothly iterate, but only draw red
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode19(void* arg __attribute__((unused)))
{
    // setDanceLeds(leds, sizeof(leds));
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 2;
    if(ledCount > 256)
    {
        ledCount = 0;
    }

    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        int16_t angle = 256 - (((i * 256) / NUM_LIN_LEDS)) + ledCount % 256;
        uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);
        leds[i].r = (color >>  0) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 70ms
 * Turn on all LEDs and smooth iterate their singular color around the color wheel
 * Note, called the 7th but comes after danceTimerMode18(). Must come before
 * freeze_color()
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR danceTimerMode7(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    ledCount = ledCount + 1;
    if(ledCount > 255)
    {
        ledCount = 0;
    }
    int16_t angle = ledCount % 256;
    color_save = EHSVtoHEX(angle, 0xFF, 0xFF);

    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {

        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }

    // Output the LED data, actually turning them on
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called every 40ms
 * Comes after danceTimerMode7(). Holds the color for all LEDs constant as
 * whatever the last shown color was from danceTimerMode7()
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR freeze_color(void* arg __attribute__((unused)))
{

    led_t leds[NUM_LIN_LEDS] = {{0}};
    uint8_t i;
    for(i = 0; i < NUM_LIN_LEDS; i++)
    {
        leds[i].r = (color_save >>  0) & 0xFF;
        leds[i].g = (color_save >>  8) & 0xFF;
        leds[i].b = (color_save >> 16) & 0xFF;
    }
    setDanceLeds(leds, sizeof(leds));
}

/**
 * Called ever 1ms
 * Pick a random dance mode and call it at its period for 4.5s. Then pick
 * another random dance and repeat
 *
 * @param arg unused
 */
void ICACHE_FLASH_ATTR random_dance_mode(void* arg __attribute__((unused)))
{
    if(random_dance_timer == 0 || random_dance_timer > 4500)
    {
        danceClearVars();

        /* If this is just starting, or it's time to restart, pick a random
         * dance, excluding the random dance for recursive reasons
         */
        random_dance_timer = 0;
        uint8_t numDances = sizeof(danceTimers) / sizeof(danceTimers[0]);
        random_choice = dance_rand(numDances - 1); // exclude the random mode

        // If the random dance is freeze, pick a random color to freeze
        if(danceTimers[random_choice].timerFn == &freeze_color)
        {
            color_save = EHSVtoHEX(dance_rand(256), 0xFF, 0xFF);
        }
    }

    // This timer runs at 1ms, so only call the chosen dance's function
    // at that dance's period
    if(0 == random_dance_timer % danceTimers[random_choice].period)
    {
        danceTimers[random_choice].timerFn(arg);
    }

    // Increment the timer
    random_dance_timer += 1;
}

/**
 * @brief Get the number of dances
 *
 * @return uint8_t getNumDances
 */
uint8_t ICACHE_FLASH_ATTR getNumDances(void)
{
    return sizeof(danceTimers) / sizeof(danceTimers[0]);
}