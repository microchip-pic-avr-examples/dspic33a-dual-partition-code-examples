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

#include "flash_region_4.h"

#define FLASH_REGION_4_TEST_CODE_ADDRESS 0xC03000

static bool LockOptionSet(uint32_t option);
static bool EraseTestArea(void);

struct FLASH_REGION flashRegion4 = {
    .lockOptionSet = LockOptionSet,
    .eraseTestArea = EraseTestArea
};

static bool LockOptionSet(uint32_t option)
{
    PR4LOCK = (FLASH_PROTECTION_KEY | option);
    return ((PR4LOCK == option) && (PR4CTRLbits.RTYPE != FLASH_PROTECTION_TYPE_IRT));
}

static bool EraseTestArea(void)
{
    static const uint32_t flashRegion4TestCode __attribute__((address(FLASH_REGION_4_TEST_CODE_ADDRESS), space(prog), keep)) = 0x01234567UL;
    
    bool pageErased = false;
    
    const uint32_t* panelVal = &flashRegion4TestCode;
    uint32_t physicalEraseAddress = FLASH_ErasePageAddressGet(FLASH_REGION_4_TEST_CODE_ADDRESS);
    
    /* Attempt to make the region writable. */
    PR4CTRLbits.WR = 1;  
       
    /* Erase the page. */
    (void)FLASH_PageErase(physicalEraseAddress, FLASH_UNLOCK_KEY);
    
    /* Test to see if the memory was erased. */
    if (*panelVal == BLANK_INSTRUCTION)
    {
        pageErased = true;
    }
    
    return pageErased;
}