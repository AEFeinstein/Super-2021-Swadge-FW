#ifndef _ASSETS_H_
#define _ASSETS_H_

#include "synced_timer.h"

uint32_t* getAsset(const char* name, uint32_t* retLen);

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint32_t* data;
} pngHandle;

bool ICACHE_FLASH_ATTR allocPngAsset(const char* name, pngHandle* handle);
void ICACHE_FLASH_ATTR freePngAsset(pngHandle* handle);
void ICACHE_FLASH_ATTR drawPng(pngHandle* handle, int16_t xp,
                               int16_t yp, bool flipLR, bool flipUD, int16_t rotateDeg);

typedef struct
{
    uint32_t* assetPtr;
    uint32_t idx;

    uint8_t* compressed;
    uint8_t* decompressed;
    uint8_t* frame;
    uint32_t allocedSize;

    uint16_t width;
    uint16_t height;
    uint16_t xp;
    uint16_t yp;
    bool flipLR;
    bool flipUD;
    int16_t rotateDeg;

    uint16_t nFrames;
    uint16_t cFrame;
    uint16_t duration;
    syncedTimer_t timer;
} gifHandle;

void drawGifFromAsset(const char* name, int16_t xp, int16_t yp,
                      bool flipLR, bool flipUD, int16_t rotateDeg,
                      gifHandle* handle);

void freeGifMemory(gifHandle* handle);

#endif