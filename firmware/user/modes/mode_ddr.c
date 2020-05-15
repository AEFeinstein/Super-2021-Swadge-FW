/*
 * mode_ddr.c
 *
 *  Created on: May 13, 2019
 *      Author: rick
 */
#include "user_config.h"

/*============================================================================
 * Includes
 *==========================================================================*/

#include <osapi.h>
#include <stdlib.h>

#include "user_main.h"
#include "mode_ddr.h"
#include "hsv_utils.h"
#include "oled.h"
#include "sprite.h"
#include "font.h"
#include "bresenham.h"
#include "buttons.h"
#include "hpatimer.h"

#include "assets.h"
#include "synced_timer.h"

/*============================================================================
 * Defines
 *==========================================================================*/

#define BTN_CTR_X 96
#define BTN_CTR_Y 40
#define BTN_RAD    8
#define BTN_OFF   12

#define ARROW_ROW_MAX_COUNT 16
#define ARROW_PERFECT_HPOS 1650
#define ARROW_PERFECT_RADIUS 30
#define ARROW_HIT_RADIUS 60

#define ARROWS_TIMER 15
#define MAX_SIXTEENTH_TIMER (60000 / 4 / ARROWS_TIMER)

#define LEDS_TIMER 15
#define MAX_PULSE_TIMER (60000 / LEDS_TIMER)

#define FEEDBACK_PERFECT 3
#define FEEDBACK_HIT 2
#define FEEDBACK_MISS 1
#define FEEDBACK_NONE 0

#define MAX_FEEDBACK_TIMER 250

/*============================================================================
 * Prototypes
 *==========================================================================*/

void ICACHE_FLASH_ATTR ddrEnterMode(void);
void ICACHE_FLASH_ATTR ddrExitMode(void);
void ICACHE_FLASH_ATTR ddrButtonCallback(uint8_t state __attribute__((unused)),
        int button, int down);
void ICACHE_FLASH_ATTR ddrAccelerometerHandler(accel_t* accel);

static void ICACHE_FLASH_ATTR ddrUpdateDisplay(void* arg __attribute__((unused)));
static void ICACHE_FLASH_ATTR ddrAnimateNotes(void* arg __attribute__((unused)));
static void ICACHE_FLASH_ATTR ddrLedFunc(void* arg __attribute__((unused)));
static void ICACHE_FLASH_ATTR ddrHandleArrows(void* arg __attribute__((unused)));
static void ICACHE_FLASH_ATTR ddrAnimateSuccessMeter(void* arg __attribute((unused)));
//static void ICACHE_FLASH_ATTR ddrAnimateSprite(void* arg __attribute__((unused)));
//static void ICACHE_FLASH_ATTR ddrUpdateButtons(void* arg __attribute__((unused)));

void ddrUpdateButtons();
void ddrHandleHit();
void ddrHandlePerfect();
void ddrHandleMiss();

/*============================================================================
 * Const data
 *==========================================================================*/


const char ddrSprites[][16] =
{
    "skull01.png",
    "skull02.png",
    "skull01.png",
    "skull03.png",
};

/*============================================================================
 * Variables
 *==========================================================================*/

swadgeMode ddrMode =
{
    .modeName = "ddr",
    .fnEnterMode = ddrEnterMode,
    .fnExitMode = ddrExitMode,
    .fnButtonCallback = ddrButtonCallback,
    .wifiMode = NO_WIFI,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
    .fnAccelerometerCallback = ddrAccelerometerHandler
};

typedef struct 
{
    uint16_t hPos;
} ddrArrow;

typedef struct 
{
    ddrArrow arrows[ARROW_ROW_MAX_COUNT];
    uint8 start;
    uint8 count;
    int pressDirection;
} ddrArrowRow;


struct
{
    // Callback variables
    accel_t Accel;
    uint8_t ButtonState;
    uint8_t ButtonDownState;

    // Timer variables
    syncedTimer_t TimerHandleLeds;
    syncedTimer_t TimerHandleArrows;
    syncedTimer_t timerAnimateNotes;
    syncedTimer_t timerUpdateDisplay;
    syncedTimer_t TimerAnimateSuccessMeter;

    uint8_t NoteIdx;

    ddrArrowRow arrowRows[4];
    uint16_t tempo;
    uint16_t maxPressForgiveness;
    uint8_t sixteenths;
    uint16_t sixteenthNoteCounter;

    uint16_t PulseTimeLeft;

    uint8_t feedbackTimer;
    uint8_t currentFeedback;

    uint8_t successMeter;
    uint8_t successMeterShineStart;
} ddr;

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for ddr
 */
void ICACHE_FLASH_ATTR ddrEnterMode(void)
{
    enableDebounce(false);

    // Clear everything
    ets_memset(&ddr, 0, sizeof(ddr));

    // Test the buzzer
    // uint32_t songLen;
    // startBuzzerSong((song_t*)getAsset("carmen.rtl", &songLen), false);

    syncedTimerDisarm(&ddr.timerAnimateNotes);
    syncedTimerSetFn(&ddr.timerAnimateNotes, ddrAnimateNotes, NULL);
    syncedTimerArm(&ddr.timerAnimateNotes, 60, true);

    syncedTimerDisarm(&ddr.TimerHandleArrows);
    syncedTimerSetFn(&ddr.TimerHandleArrows, ddrHandleArrows, NULL);
    syncedTimerArm(&ddr.TimerHandleArrows, 15, true);

    syncedTimerDisarm(&ddr.timerUpdateDisplay);
    syncedTimerSetFn(&ddr.timerUpdateDisplay, ddrUpdateDisplay, NULL);
    syncedTimerArm(&ddr.timerUpdateDisplay, 15, true);

    // Test the LEDs
    syncedTimerDisarm(&ddr.TimerHandleLeds);
    syncedTimerSetFn(&ddr.TimerHandleLeds, ddrLedFunc, NULL);
    syncedTimerArm(&ddr.TimerHandleLeds, LEDS_TIMER, true);

    syncedTimerDisarm(&ddr.TimerAnimateSuccessMeter);
    syncedTimerSetFn(&ddr.TimerAnimateSuccessMeter, ddrAnimateSuccessMeter, NULL);
    syncedTimerArm(&ddr.TimerAnimateSuccessMeter, 40, true);

    // Draw a gif
    //drawGifFromAsset("ragequit.gif", 0, 0, false, false, 0, &ddr.gHandle);

    // reset arrows
    for (int i = 0; i < 4; i++)
    {
        ddr.arrowRows[i].count = 0;
        ddr.arrowRows[i].start = 0;
    }

    ddr.arrowRows[0].pressDirection = DOWN;
    ddr.arrowRows[1].pressDirection = RIGHT;
    ddr.arrowRows[2].pressDirection = LEFT;
    ddr.arrowRows[3].pressDirection = UP;

    ddr.tempo = 110;
    ddr.sixteenths = 6;
    ddr.sixteenthNoteCounter = MAX_SIXTEENTH_TIMER;

    ddr.ButtonDownState = 0;

    ddr.PulseTimeLeft = MAX_PULSE_TIMER;

    ddr.currentFeedback = 0;
    ddr.feedbackTimer = 0;

    ddr.successMeter = 80;
    ddr.successMeterShineStart = 0;
}

/**
 * Called when ddr is exited
 */
void ICACHE_FLASH_ATTR ddrExitMode(void)
{
    stopBuzzerSong();
    syncedTimerDisarm(&ddr.timerAnimateNotes);
    syncedTimerDisarm(&ddr.TimerHandleLeds);
    syncedTimerDisarm(&ddr.TimerHandleArrows);
    syncedTimerDisarm(&ddr.timerUpdateDisplay);
    syncedTimerDisarm(&ddr.TimerHandleLeds);
}

/**
 * @brief called on a timer, this blinks an LED pattern
 *
 * @param arg
 */
static void ICACHE_FLASH_ATTR ddrLedFunc(void* arg __attribute__((unused)))
{
    led_t leds[NUM_LIN_LEDS] = {{0}};

    if (ddr.currentFeedback)
    {
        if (LEDS_TIMER > ddr.feedbackTimer)
        {
            ddr.feedbackTimer = MAX_FEEDBACK_TIMER;
            ddr.currentFeedback = FEEDBACK_NONE;
        } 
        else
        {
            ddr.feedbackTimer -= LEDS_TIMER;
            switch(ddr.currentFeedback)
            {
                case FEEDBACK_PERFECT:
                    leds[2].g=250;
                    leds[3].g=250;
                    break;
                    
                case FEEDBACK_HIT:
                    leds[2].g=100;
                    leds[2].r=50;
                    leds[3].g=100;
                    leds[3].r=50;
                    break;

                default:// FEEDBACK_MISS:
                    leds[2].r=50;
                    leds[3].r=50;
            }
        }
        
    }

    uint16_t pulseTimeReduction = ddr.tempo;
    if (pulseTimeReduction > ddr.PulseTimeLeft)
    {
        ddr.PulseTimeLeft = MAX_PULSE_TIMER - pulseTimeReduction + ddr.PulseTimeLeft;
    }
    else 
    {
        ddr.PulseTimeLeft -= pulseTimeReduction;
        if (ddr.PulseTimeLeft < 1000)
        {
            leds[0].b=128;
            leds[1].b=128;
        
            leds[NUM_LIN_LEDS-1].b=128;
            leds[NUM_LIN_LEDS-2].b=128;
        }
    } 

    setLeds(leds, sizeof(leds));
}

void fisherYates(int arr[], int n)
{
    for (int i = n - 1; i> 0; i--)
    {
        int j = rand() % (i+1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/**
 * @brief Called on a timer, this moves the arrows and checks for hits/misses
 *
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR ddrHandleArrows(void* arg __attribute__((unused)))
{
    ddrArrowRow* curRow;
    ddrArrow* curArrow;
    int curStart;
    int curCount;
    int curEnd;

    bool canSpawnArrow = false;
    int percentChanceSpawn = 0;

    if (ddr.tempo > ddr.sixteenthNoteCounter) 
    {
        ddr.sixteenthNoteCounter = MAX_SIXTEENTH_TIMER - ddr.tempo + ddr.sixteenthNoteCounter;
        ddr.sixteenths = (ddr.sixteenths + 1 ) % 16;

        canSpawnArrow = true;
        
        if (0 == ddr.sixteenths)
        {
            percentChanceSpawn = 30; // 30 percent chance on first beat
        }
         else if ( 8 == ddr.sixteenths)
        {
            percentChanceSpawn = 25; // 20 percent chance on 3rd beat
        }
        else if ( 0 == ddr.sixteenths % 4)
        {
            percentChanceSpawn = 20; // 10 percent chance on 2nd/4th beat
        }
        else if ( 0 == ddr.sixteenths % 2)
        {
            percentChanceSpawn = 5; // 5 percent chance on half beats
        }
        else 
        {
            percentChanceSpawn = 0; // 0 percent chance on other 16th beats
        }
        
    }
    else 
    {
        ddr.sixteenthNoteCounter -= ddr.tempo;
    }

    // generate arrows for rows in random order to avoid tending to "run out" 
    // before row 2 & 3
    int rowIdxs[] = {0,1,2,3};
    fisherYates(rowIdxs,4);

    int arrowsSpawnedThisBeat = 0;
    for (int i = 0; i < 4; i++)
    {
        int rowIdx = rowIdxs[i];
        curRow = &(ddr.arrowRows[rowIdx]);
        curStart = curRow->start;
        curCount = curRow->count;
        curEnd = (curStart + curCount) % ARROW_ROW_MAX_COUNT;

        for(int arrowIdx = curStart; arrowIdx != curEnd; arrowIdx = (arrowIdx + 1) % ARROW_ROW_MAX_COUNT)
        {
            curArrow = &(curRow->arrows[arrowIdx]);
            curArrow->hPos += ddr.tempo * 0.07;

            uint16_t arrowDist = abs(curArrow->hPos - ARROW_PERFECT_HPOS);

            if (arrowDist <= ARROW_PERFECT_RADIUS)
            {
                if(ddr.ButtonDownState & curRow->pressDirection)
                { //assumes that no more than one arrow per row can be in hit zone at a time
                    curRow->count--;
                    curRow->start = (curRow->start + 1) % ARROW_ROW_MAX_COUNT;
                    
                    // reset down state
                    ddr.ButtonDownState = ddr.ButtonDownState & ~curRow->pressDirection;
                    ddrHandlePerfect();
                }
            }
            else if (arrowDist <= ARROW_HIT_RADIUS)
            {
                if(ddr.ButtonDownState & curRow->pressDirection)
                { //assumes that no more than one arrow per row can be in hit zone at a time
                    curRow->count--;
                    curRow->start = (curRow->start + 1) % ARROW_ROW_MAX_COUNT;
                    
                    // reset down state
                    ddr.ButtonDownState = ddr.ButtonDownState & ~curRow->pressDirection;
                    ddrHandleHit();
                }
            }
            else if (curArrow->hPos > ARROW_PERFECT_HPOS)
            {
                curRow->count--;
                curRow->start = (curRow->start + 1) % ARROW_ROW_MAX_COUNT;
                ddrHandleMiss();
            }
            
        }

        if (canSpawnArrow && arrowsSpawnedThisBeat < 2)
        {
            if (rand() % 100 < percentChanceSpawn)
            {
                arrowsSpawnedThisBeat++;
                curRow->count++;
                curRow->arrows[(curRow->start+curRow->count) % ARROW_ROW_MAX_COUNT].hPos = 0;
            }
        }
    }
}

/**
 * @brief Called on a timer, this animates the note asset
 *
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR ddrAnimateNotes(void* arg __attribute__((unused)))
{
    ddr.NoteIdx = (ddr.NoteIdx + 1) % 4;
    // testUpdateDisplay();
}

void ddrUpdateDownButtonState(uint8_t mask)
{
    uint8_t state = ddr.ButtonState & mask;
    uint8_t startDownState = ddr.ButtonDownState & mask;

    if (state || startDownState)
    {
        ddr.ButtonDownState = (ddr.ButtonDownState & ~mask) + (state & ~startDownState);
    }
}

void ddrUpdateButtons()
{
    ddrUpdateDownButtonState(LEFT);
    ddrUpdateDownButtonState(DOWN);
    ddrUpdateDownButtonState(UP);
    ddrUpdateDownButtonState(RIGHT);
}

/**
 * TODO
 */
static void ICACHE_FLASH_ATTR ddrUpdateDisplay(void* arg __attribute__((unused)))
{
    // Clear the display
    clearDisplay();

    // Draw a title
    //plotText(0, 0, "DDR MODE", RADIOSTARS, WHITE);

    //ddrUpdateButtons();
    ddrArrowRow* curRow;
    ddrArrow* curArrow;
    int curStart;
    int curCount;
    int curEnd;

    for (int rowIdx = 0; rowIdx < 4; rowIdx++)
    {
        curRow = &(ddr.arrowRows[rowIdx]);
        curStart = curRow->start;
        curCount = curRow->count;
        curEnd = (curStart + curCount) % ARROW_ROW_MAX_COUNT;

        for(int arrowIdx = curStart; arrowIdx != curEnd; arrowIdx = (arrowIdx + 1) % ARROW_ROW_MAX_COUNT)
        {
            curArrow = &(curRow->arrows[arrowIdx]);
            
            drawBitmapFromAsset(ddrSprites[ddr.NoteIdx],
                            (curArrow->hPos-400)/12,
                            48 - rowIdx * 16,
                            false,
                            false,
                            0);
        }
    }
    
    plotCircle(110, 55-16*3, BTN_RAD-3, WHITE);
    plotCircle(110, 55-16*2, BTN_RAD-3, WHITE);
    plotCircle(110, 55-16*1, BTN_RAD-3, WHITE);
    plotCircle(110, 55-16*0, BTN_RAD-3, WHITE);

    /*
    if(ddr.ButtonDownState & UP)
    {
        // A
        plotCircle(110, 55-16*3, BTN_RAD, WHITE);
    }

    if(ddr.ButtonDownState & LEFT)
    {
        // S
        plotCircle(110, 55-16*2, BTN_RAD, WHITE);
    }
    
    if(ddr.ButtonDownState & RIGHT)
    {
        // D
        plotCircle(110, 55-16*1, BTN_RAD, WHITE);
    }

    if(ddr.ButtonDownState & DOWN)
    {
        // F
        plotCircle(110, 55-16*0, BTN_RAD, WHITE);
    }
    */

    if ( ddr.successMeter == 100 )
    {
        plotLine(OLED_WIDTH - 1, 0, OLED_WIDTH - 1, OLED_HEIGHT - 1, WHITE);
        for (int y = OLED_HEIGHT - 1 - ddr.successMeterShineStart; y > 0 ; y-=3 )
        {
            drawPixel(OLED_WIDTH-1, y, BLACK);
        }
    }
    else 
    {
        plotLine(OLED_WIDTH - 1, (OLED_HEIGHT - 1) - (float)ddr.successMeter/100.0f * (OLED_HEIGHT - 1), OLED_WIDTH - 1, OLED_HEIGHT - 1, WHITE);
    }


    ddr.ButtonDownState = 0;
}


void ddrHandleHit()
{
    ddr.feedbackTimer = MAX_FEEDBACK_TIMER;
    ddr.currentFeedback = FEEDBACK_HIT;

    if (ddr.successMeter >= 99)
    {
        ddr.successMeter = 100;
    } else
    {
        ddr.successMeter += 1;
    }
}

void ddrHandlePerfect()
{
    ddr.feedbackTimer = MAX_FEEDBACK_TIMER;
    ddr.currentFeedback = FEEDBACK_PERFECT;

    if (ddr.successMeter >= 97)
    {
        ddr.successMeter = 100;
    } else
    {
        ddr.successMeter += 3;
    }
}

void ddrHandleMiss(){
    ddr.feedbackTimer = MAX_FEEDBACK_TIMER;
    ddr.currentFeedback = FEEDBACK_MISS;

    if (ddr.successMeter <= 5)
    {
        ddr.successMeter = 0;
    } else
    {
        ddr.successMeter -= 5;
    }
    
}

/**
 * TODO
 *
 * @param state  A bitmask of all button states, unused
 * @param button The button which triggered this event
 * @param down   true if the button was pressed, false if it was released
 */
void ICACHE_FLASH_ATTR ddrButtonCallback( uint8_t state,
        int button, int down)
{
    ddr.ButtonState = state;
    ddr.ButtonDownState = (ddr.ButtonDownState & ~(1<<button)) + (down << button);
}

/**
 * Store the acceleration data to be displayed later
 *
 * @param x
 * @param x
 * @param z
 */
void ICACHE_FLASH_ATTR ddrAccelerometerHandler(accel_t* accel)
{
    ddr.Accel.x = accel->x;
    ddr.Accel.y = accel->y;
    ddr.Accel.z = accel->z;
    // ddrUpdateDisplay();
}

static void ICACHE_FLASH_ATTR ddrAnimateSuccessMeter(void* arg __attribute((unused)))
{
    ddr.successMeterShineStart = (ddr.successMeterShineStart + 1 ) % 4;
}