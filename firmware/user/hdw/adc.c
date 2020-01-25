// I did not write this file, but I don't know where it came from.
// It was also heavily modified by me, Charles Lohr.  I personally
// renounce any copyright over this file, so all of my modifications
// are hereby placed into the public domain.

/*============================================================================
 * Includes
 *==========================================================================*/

#include <ets_sys.h>
#include <osapi.h>

#include "adc.h"
#include "missingEspFnPrototypes.h"

/*============================================================================
 * Defines
 *==========================================================================*/

#define i2c_saradc                          0x6C
#define i2c_saradc_hostid                   2

#define i2c_saradc_en_test        0
#define i2c_saradc_en_test_msb    5
#define i2c_saradc_en_test_lsb    5

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata) \
    rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)

#define i2c_writeReg_Mask_def(block, reg_add, indata) \
    i2c_writeReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Start an ADC measurement, but don't take it yet
 */
void hs_adc_start(void)
{
    // select test mux
    i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1);

    // PWDET_CAL_EN=0, PKDET_CAL_EN=0
    SET_PERI_REG_MASK(0x60000D5C, 0x200000);

    while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0)
    {
        ; // wait r_state == 0
    }
    CLEAR_PERI_REG_MASK(0x60000D50, 0x02);    // force_en=0
    SET_PERI_REG_MASK(0x60000D50, 0x02);    // force_en=1
}

/**
 * Once the ADC has been taking a measurement for long enough, read it
 *
 * @return A 16 bit ADC measurement
 */
uint16 hs_adc_read(void)
{
    uint8 i;
    uint16 sardata[8];
    uint16_t sar_dout = 0;

    while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0)
    {
        ; // wait r_state == 0
    }

    read_sar_dout(sardata);

    for (i = 0; i < 8; i++)
    {
        sar_dout += sardata[i];
    }

#ifdef OLDWAY_NEEDS_RESTART
    // tout = (sar_dout + 8) >> 4;   // tout is 10 bits fraction
    // ??? Why does this exist ??? It didn't start commented out, but now that I did comment it, it still seems happy.
    // i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1); // select test mux
    // while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); // wait r_state == 0

    // CLEAR_PERI_REG_MASK(0x60000D5C, 0x200000);
    // SET_PERI_REG_MASK(0x60000D60, 0x1);    // force_en=1
    // CLEAR_PERI_REG_MASK(0x60000D60, 0x1);    // force_en=1
#else
    // Start reading a new sample.
    CLEAR_PERI_REG_MASK(0x60000D50, 0x02);    // force_en=0
    SET_PERI_REG_MASK(0x60000D50, 0x02);    // force_en=1
#endif

    // tout is 10 bits fraction
    return sar_dout;
}
