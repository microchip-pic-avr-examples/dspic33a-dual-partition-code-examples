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

#include "flash_region_2.h"

#define FLASH_REGION_2_TEST_CODE_ADDRESS 0x806000

static bool LockOptionSet(uint32_t option);
static bool EraseTestArea(void);

struct FLASH_REGION flashRegion2 = {
    .lockOptionSet = LockOptionSet,
    .eraseTestArea = EraseTestArea
};

static bool LockOptionSet(uint32_t option)
{
    PR2LOCK = (FLASH_PROTECTION_KEY | option);
    return ((PR2LOCK == option) && (PR2CTRLbits.RTYPE != FLASH_PROTECTION_TYPE_IRT));
}

static bool EraseTestArea(void)
{
    /* Create a test block within the flash region for testing permissions.  The 
    * configuration bits define this region to be from 0x806000-0x806FFF. We don't
    * want any code to link into this range if we successfully erase this range so
    * we also block out that entire block of memory. */
    static const volatile uint32_t flashRegion2TestCodeBuffer[FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS - 1U] __attribute__((address(FLASH_REGION_2_TEST_CODE_ADDRESS + 4U), space(prog), keep, used)) = {0};
    static const uint32_t flashRegion2TestCode __attribute__((address(FLASH_REGION_2_TEST_CODE_ADDRESS), space(prog), keep)) = 0x01234567UL;
    
    bool pageErased = false;
    
    const uint32_t* panelVal = &flashRegion2TestCode;
    uint32_t physicalEraseAddress = FLASH_ErasePageAddressGet(FLASH_REGION_2_TEST_CODE_ADDRESS);
    
    /* Attempt to make the region writable. */
    PR2CTRLbits.WR = 1;  
       
    /* Erase the page. */
    (void)FLASH_PageErase(physicalEraseAddress, FLASH_UNLOCK_KEY);
    
    /* Test to see if the memory was erased. */
    if (*panelVal == BLANK_INSTRUCTION)
    {
        pageErased = true;
    }
    
    return pageErased;
}