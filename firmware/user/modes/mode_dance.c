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
 * Typedefs
 *==========================================================================*/

typedef void (*ledDance)(uint32_t, uint32_t, bool);

typedef struct
{
    ledDance func;
    uint32_t arg;
} ledDanceArg;

#define RGB_2_ARG(r,g,b) ((((r)&0xFF) << 16) | (((g)&0xFF) << 8) | (((b)&0xFF)))
#define ARG_R(arg) (((arg) >> 16)&0xFF)
#define ARG_G(arg) (((arg) >>  8)&0xFF)
#define ARG_B(arg) (((arg) >>  0)&0xFF)

/*============================================================================
 * Prototypes
 *==========================================================================*/

void ICACHE_FLASH_ATTR setDanceLeds(led_t* ledData, uint8_t ledDataLen);
uint32_t ICACHE_FLASH_ATTR danceRand(uint32_t upperBound);

void ICACHE_FLASH_ATTR danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR dancePureRandom(uint32_t tElapsedUs, uint32_t arg, bool reset);
void ICACHE_FLASH_ATTR danceRandomDance(uint32_t tElapsedUs, uint32_t arg, bool reset);

/*============================================================================
 * Variables
 *==========================================================================*/

static const uint8_t danceBrightnesses[] =
{
    0x01,
    0x08,
    0x40,
};

static const ledDanceArg ledDances[] =
{
    {.func = danceComet, .arg = RGB_2_ARG(0xFF, 0, 0)},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0xFF, 0)},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0xFF)},
    {.func = danceComet, .arg = RGB_2_ARG(0, 0, 0)}, // This is rainbow
    {.func = danceRise, .arg = RGB_2_ARG(0xFF, 0, 0)},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0xFF, 0)},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0xFF)},
    {.func = danceRise, .arg = RGB_2_ARG(0, 0, 0)}, // This is random
    {.func = dancePulse, .arg = RGB_2_ARG(0xFF, 0, 0)},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0xFF, 0)},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0xFF)},
    {.func = dancePulse, .arg = RGB_2_ARG(0, 0, 0)}, // This is random
    {.func = danceSharpRainbow, .arg = 0},
    {.func = danceSmoothRainbow, .arg = 0},
    {.func = danceRainbowSolid, .arg = 0},
    {.func = danceFire, .arg = RGB_2_ARG(0xFF, 51, 0)},
    {.func = danceFire, .arg = RGB_2_ARG(0, 0xFF, 51)},
    {.func = danceFire, .arg = RGB_2_ARG(51, 0, 0xFF)},
    {.func = danceBinaryCounter, .arg = 0},
    {.func = dancePoliceSiren, .arg = 0},
    {.func = dancePureRandom, .arg = 0},
    {.func = danceRandomDance, .arg = 0},
};

uint8_t danceBrightnessIdx = 0;

/*============================================================================
 * Functions
 *==========================================================================*/

/** @return The number of different tances
 */
uint8_t ICACHE_FLASH_ATTR getNumDances(void)
{
    return (sizeof(ledDances) / sizeof(ledDances[0]));
}

/** This is called to clear all dance variables
 */
void ICACHE_FLASH_ATTR danceClearVars(void)
{
    // Reset all dances
    for(uint8_t i = 0; i < getNumDances(); i++)
    {
        ledDances[i].func(0, ledDances[i].arg, true);
    }
}

/** Set the brightness index
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

/** Intermediate function which adjusts brightness and sets the LEDs
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

/** Get a random number from a range.
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
uint32_t ICACHE_FLASH_ATTR danceRand(uint32_t bound)
{
    if(bound == 0)
    {
        return 0;
    }
    return os_random() % bound;
}

/** Call this to animate LEDs. Dances use the system time for animations, so this
 * should be called reasonably fast for smooth operation
 *
 * @param danceIdx The index of the dance to display.
 */
void ICACHE_FLASH_ATTR danceLeds(uint8_t danceIdx)
{
    static uint32_t tLast = 0;
    if(0 == tLast)
    {
        tLast = system_get_time();
    }
    else
    {
        uint32_t tNow = system_get_time();
        uint32_t tElapsedUs = tNow - tLast;
        tLast = tNow;
        ledDances[danceIdx].func(tElapsedUs, ledDances[danceIdx].arg, false);
    }
}

/** This animation is set to be called every 100 ms
 * Rotate a single white LED around the hexagon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount = 0;
    static uint8_t rainbow = 0;
    static int32_t msCount = 0;
    static uint32_t tAccumulated = 0;
    static led_t leds[NUM_LIN_LEDS] = {{0}};

    if(reset)
    {
        ledCount = 0;
        rainbow = 0;
        msCount = 80;
        tAccumulated = 2000;
        ets_memset(leds, sizeof(leds), 0);
        return;
    }

    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 2000)
    {
        tAccumulated -= 2000;
        for(uint8_t i = 0; i < NUM_LIN_LEDS; i++)
        {
            if(leds[i].r > 0)
            {
                leds[i].r--;
            }
            if(leds[i].g > 0)
            {
                leds[i].g--;
            }
            if(leds[i].b > 0)
            {
                leds[i].b--;
            }
        }
        msCount++;

        if(msCount % 10 == 0)
        {
            rainbow++;
        }

        if(msCount >= 80)
        {
            if(0 == arg)
            {
                int32_t color = EHSVtoHEX(rainbow, 0xFF, 0xFF);
                leds[ledCount].r = (color >>  0) & 0xFF;
                leds[ledCount].g = (color >>  8) & 0xFF;
                leds[ledCount].b = (color >> 16) & 0xFF;
            }
            else
            {
                leds[ledCount].r = ARG_R(arg);
                leds[ledCount].g = ARG_G(arg);
                leds[ledCount].b = ARG_B(arg);
            }
            ledCount = (ledCount + 1) % NUM_LIN_LEDS;
            msCount = 0;
        }
        ledsUpdated = true;
    }

    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** This animation is set to be called every 100ms
 * Blink all LEDs red for on for 500ms, then off for 500ms
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint8_t ledVal = 0;
    static uint8_t randColor = 0;
    static bool goingUp = true;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        ledVal = 0;
        randColor = 0;
        goingUp = true;
        tAccumulated = 5000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 5000)
    {
        tAccumulated -= 5000;

        if(goingUp)
        {
            ledVal++;
            if(255 == ledVal)
            {
                goingUp = false;
            }
        }
        else
        {
            ledVal--;
            if(0 == ledVal)
            {
                goingUp = true;
                randColor = danceRand(256);
            }
        }

        for (int i = 0; i < NUM_LIN_LEDS; i++)
        {
            if(0 == arg)
            {
                int32_t color = EHSVtoHEX(randColor, 0xFF, 0xFF);
                leds[i].r = (ledVal * ((color >>  0) & 0xFF) >> 8);
                leds[i].g = (ledVal * ((color >>  8) & 0xFF) >> 8);
                leds[i].b = (ledVal * ((color >> 16) & 0xFF) >> 8);
            }
            else
            {
                leds[i].r = (ledVal * ARG_R(arg)) >> 8;
                leds[i].g = (ledVal * ARG_G(arg)) >> 8;
                leds[i].b = (ledVal * ARG_B(arg)) >> 8;
            }
        }
        ledsUpdated = true;
    }

    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** This animation is set to be called every 100 ms
 * Rotate a single white LED around the hexagon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int16_t levels[NUM_LIN_LEDS / 2] = {0, -256, -512};
    static bool rising[NUM_LIN_LEDS / 2] = {true, true, true};
    static uint8_t angle = 0;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        for(uint8_t i; i < NUM_LIN_LEDS / 2; i++)
        {
            levels[i] = i * -256;
            rising[i] = true;
        }
        angle = 0;
        tAccumulated = 800;
        return;
    }

    bool ledsUpdated = false;
    led_t leds[NUM_LIN_LEDS] = {0};

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 800)
    {
        tAccumulated -= 800;

        if(true == rising[0] && 0 == levels[0])
        {
            angle = danceRand(256);
        }

        for(uint8_t i = 0; i < NUM_LIN_LEDS / 2; i++)
        {
            if(rising[i])
            {
                levels[i]++;
                if(levels[i] == 255)
                {
                    rising[i] = false;
                }
            }
            else
            {
                levels[i]--;
                if(levels[i] == -512)
                {
                    rising[i] = true;
                }
            }
        }

        if(0 == arg)
        {
            int32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);
            for(uint8_t i = 0; i < NUM_LIN_LEDS / 2; i++)
            {
                if(levels[i] > 0)
                {
                    leds[i].r = (levels[i] * ((color >>  0) & 0xFF) >> 8);
                    leds[i].g = (levels[i] * ((color >>  8) & 0xFF) >> 8);
                    leds[i].b = (levels[i] * ((color >> 16) & 0xFF) >> 8);

                    leds[NUM_LIN_LEDS - 1 - i].r = (levels[i] * ((color >>  0) & 0xFF) >> 8);
                    leds[NUM_LIN_LEDS - 1 - i].g = (levels[i] * ((color >>  8) & 0xFF) >> 8);
                    leds[NUM_LIN_LEDS - 1 - i].b = (levels[i] * ((color >> 16) & 0xFF) >> 8);
                }
            }
        }
        else
        {
            for(uint8_t i = 0; i < NUM_LIN_LEDS / 2; i++)
            {
                if(levels[i] > 0)
                {
                    leds[i].r = (levels[i] * ARG_R(arg)) >> 8;
                    leds[i].g = (levels[i] * ARG_G(arg)) >> 8;
                    leds[i].b = (levels[i] * ARG_B(arg)) >> 8;

                    leds[NUM_LIN_LEDS - 1 - i].r = (levels[i] * ARG_R(arg)) >> 8;
                    leds[NUM_LIN_LEDS - 1 - i].g = (levels[i] * ARG_G(arg)) >> 8;
                    leds[NUM_LIN_LEDS - 1 - i].b = (levels[i] * ARG_B(arg)) >> 8;
                }
            }
        }
        ledsUpdated = true;
    }

    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** Smoothly rotate a color wheel around the hexagon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;
    static uint8_t ledCount = 0;

    if(reset)
    {
        ledCount = 0;
        tAccumulated = 20000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 20000)
    {
        tAccumulated -= 20000;
        ledsUpdated = true;

        ledCount--;

        uint8_t i;
        for(i = 0; i < NUM_LIN_LEDS; i++)
        {
            int16_t angle = ((((i * 256) / NUM_LIN_LEDS)) + ledCount) % 256;
            uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

            leds[i].r = (color >>  0) & 0xFF;
            leds[i].g = (color >>  8) & 0xFF;
            leds[i].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** Sharply rotate a color wheel around the hexagon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount = 0;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        ledCount = 0;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

        ledCount = ledCount + 1;
        if(ledCount > NUM_LIN_LEDS - 1)
        {
            ledCount = 0;
        }

        uint8_t i;
        for(i = 0; i < NUM_LIN_LEDS; i++)
        {
            int16_t angle = (((i * 256) / NUM_LIN_LEDS)) % 256;
            uint32_t color = EHSVtoHEX(angle, 0xFF, 0xFF);

            leds[(i + ledCount) % NUM_LIN_LEDS].r = (color >>  0) & 0xFF;
            leds[(i + ledCount) % NUM_LIN_LEDS].g = (color >>  8) & 0xFF;
            leds[(i + ledCount) % NUM_LIN_LEDS].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** Counts up to 64 in binary. At 64, the color is held for ~3s
 * The 'on' color is smoothly iterated over the color wheel. The 'off'
 * color is also iterated over the color wheel, 180 degrees offset from 'on'
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount = 0;
    static int32_t ledCount2 = 0;
    static bool led_bool = false;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        ledCount = 0;
        ledCount2 = 0;
        led_bool = false;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

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
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/**
 * Fire pattern. All LEDs are random amount of red, and fifth that of green.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        tAccumulated = 100000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 100000)
    {
        tAccumulated -= 100000;
        ledsUpdated = true;

        uint8_t base = danceRand(120) + 135;
        uint8_t mid = danceRand(80) + 80;
        uint8_t tip = danceRand(50) + 40;

        leds[0].r = (base * ARG_R(arg)) / 256;
        leds[0].g = (base * ARG_G(arg)) / 256;
        leds[0].b = (base * ARG_B(arg)) / 256;
        leds[5].r = leds[0].r;
        leds[5].g = leds[0].g;
        leds[5].b = leds[0].b;

        leds[1].r = (mid * ARG_R(arg)) / 256;
        leds[1].g = (mid * ARG_G(arg)) / 256;
        leds[1].b = (mid * ARG_B(arg)) / 256;
        leds[4].r = leds[1].r;
        leds[4].g = leds[1].g;
        leds[4].b = leds[1].b;

        leds[2].r = (tip * ARG_R(arg)) / 256;
        leds[2].g = (tip * ARG_G(arg)) / 256;
        leds[2].b = (tip * ARG_B(arg)) / 256;
        leds[3].r = leds[2].r;
        leds[3].g = leds[2].g;
        leds[3].b = leds[2].b;
    }
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** police sirens, flash half red then half blue
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        ledCount = 0;
        tAccumulated = 120000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 120000)
    {
        tAccumulated -= 120000;
        ledsUpdated = true;

        // Skip to the next LED around the hexagon
        ledCount = ledCount + 1;
        if(ledCount > NUM_LIN_LEDS)
        {
            ledCount = 0;

        }

        uint8_t i;
        if(ledCount < (NUM_LIN_LEDS >> 1))
        {
            uint32_t red = EHSVtoHEX(245, 0xFF, 0xFF); // Red, hint of blue
            for(i = 0; i < (NUM_LIN_LEDS >> 1); i++)
            {
                leds[i].r = (red >>  0) & 0xFF;
                leds[i].g = (red >>  8) & 0xFF;
                leds[i].b = (red >> 16) & 0xFF;
            }
        }
        else
        {
            uint32_t blue = EHSVtoHEX(180, 0xFF, 0xFF); // Blue, hint of red
            for(i = (NUM_LIN_LEDS >> 1); i < NUM_LIN_LEDS; i++)
            {
                leds[i].r = (blue >>  0) & 0xFF;
                leds[i].g = (blue >>  8) & 0xFF;
                leds[i].b = (blue >> 16) & 0xFF;
            }
        }
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** Turn a random LED on to a random color, one at a time
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR dancePureRandom(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;
    static uint8_t randLed = 0;
    static uint32_t randColor = 0;
    static uint8_t ledVal = 0;
    static bool ledRising = true;
    static uint32_t randInterval = 5000;

    if(reset)
    {
        randInterval = 5000;
        tAccumulated = randInterval;
        randLed = 0;
        randColor = 0;
        ledVal = 0;
        ledRising = true;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= randInterval)
    {
        tAccumulated -= randInterval;

        if(0 == ledVal)
        {
            randColor = danceRand(256);
            randLed = danceRand(NUM_LIN_LEDS);
            randInterval = 500 + danceRand(4096);
            ledVal++;
        }
        else if(ledRising)
        {
            ledVal++;
            if(255 == ledVal)
            {
                ledRising = false;
            }
        }
        else
        {
            ledVal--;
            if(0 == ledVal)
            {
                ledRising = true;
            }
        }

        ledsUpdated = true;
        uint32_t color = EHSVtoHEX(randColor, 0xFF, ledVal);
        leds[randLed].r = (color >>  0) & 0xFF;
        leds[randLed].g = (color >>  8) & 0xFF;
        leds[randLed].b = (color >> 16) & 0xFF;
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/**
 * Turn on all LEDs and smooth iterate their singular color around the color wheel
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount = 0;
    static int32_t color_save = 0;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        ledCount = 0;
        color_save = 0;
        tAccumulated = 70000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[NUM_LIN_LEDS] = {{0}};
    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 70000)
    {
        tAccumulated -= 70000;
        ledsUpdated = true;

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
    }
    // Output the LED data, actually turning them on
    if(ledsUpdated)
    {
        setDanceLeds(leds, sizeof(leds));
    }
}

/** Called ever 1ms
 * Pick a random dance mode and call it at its period for 4.5s. Then pick
 * another random dance and repeat
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void ICACHE_FLASH_ATTR danceRandomDance(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t random_choice = -1;
    static uint32_t tAccumulated = 0;

    if(reset)
    {
        random_choice = -1;
        tAccumulated = 4500000;
        return;
    }

    if(-1 == random_choice)
    {
        random_choice = danceRand(getNumDances() - 1); // exclude the random mode
    }

    tAccumulated += tElapsedUs;
    while(tAccumulated >= 4500000)
    {
        tAccumulated -= 4500000;
        random_choice = danceRand(getNumDances() - 1); // exclude the random mode
        ledDances[random_choice].func(0, ledDances[random_choice].arg, true);
    }

    ledDances[random_choice].func(tElapsedUs, ledDances[random_choice].arg, false);
}
