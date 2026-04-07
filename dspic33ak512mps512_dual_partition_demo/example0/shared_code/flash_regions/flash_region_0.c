/*
(c) [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

#include "../mcc_generated_files/flash/flash_types.h"
#include "../mcc_generated_files/flash/flash.h"
#include "../mcc_generated_files/flash/flash_interface.h"

#include "flash_region_0.h"

/*******************************************************************************
 * FLASH REGION 0
 * ---------------------------
 * 
 * The flash protection regions are create/defined using the associated
 * configuration bits and are manipulated at run time via the associated
 * registers.  
 * 
 * For flash region 0, the following configuration registers define the region:
 * FPR0CTRL - Defines the region type and permissions
 * FPR0ST - Defines the start address of the region 
 * FPR0END -  Defines the end address of the region
 *
 * The configuration bits are loaded into SFRs on reset from the configuration
 * bits associated with the partition selected to boot based on the sequence
 * numbers of each partition (when in dual partition mode).
 *
 * In this example, region 0 is configured as:
 * - A firmware region (permissions can thus be modified after unlocking)
 * - Read/Execution enabled by default, Writes disabled by default
 * - Applied to BOTH panels/partitions.  The same rules defined above will apply
 *   when reading/writing/erasing the same memory range of the active or inactive
 *   panel/partition.
 * 
 * See config_bits.c for the configuration settings of this region.
 ******************************************************************************/

#define FLASH_REGION_0_TEST_CODE_ADDRESS 0x802000

static bool LockOptionSet(uint32_t option);
static bool IsWriteEnabled(void);
static enum PANEL PanelGet(void);

struct FLASH_REGION flashRegion0 = {
    .lockOptionSet = LockOptionSet,
    .isWriteEnabled = IsWriteEnabled,
    .panelGet = PanelGet
};

/**
 * @ingroup    flash_region_0
 * @brief      Changes the lock/permissions options as specified in the 
 *             parameter for flash region 0.  This can be used to lock/unlock 
 *             the region or enable/disable the read/write/execute permissions
 * @param[in]  option - the options to write to the flash region 0 lock register
 * @return     bool - true if the operation was successful
 */
static bool LockOptionSet(uint32_t option)
{
    PR0LOCK = (FLASH_PROTECTION_KEY | option);
    return ((PR0LOCK == option) && (PR0CTRLbits.RTYPE != FLASH_PROTECTION_TYPE_IRT));
}

static bool IsWriteEnabled(void)
{
    return PR0CTRLbits.WR == 1;
}

static enum PANEL PanelGet(void)
{
    return PR0CTRLbits.PSEL;
}