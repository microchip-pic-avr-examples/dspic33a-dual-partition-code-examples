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

#include "flash_region_5.h"

#define FLASH_REGION_5_TEST_CODE_ADDRESS 0xC05000

static bool LockOptionSet(uint32_t option);
static bool EraseTestArea(void);

struct FLASH_REGION flashRegion5 = {
    .lockOptionSet = LockOptionSet,
    .eraseTestArea = EraseTestArea
};

static bool LockOptionSet(uint32_t option)
{
    PR5LOCK = (FLASH_PROTECTION_KEY | option);
    return ((PR5LOCK == option) && (PR5CTRLbits.RTYPE != FLASH_PROTECTION_TYPE_IRT));
}

static bool EraseTestArea(void)
{
    bool pageErased = false;
    
    /* cppcheck-suppress misra-c2012-11.4
     * 
     *  (Rule 11.4) ADVISORY: A conversion should not be performed between a
     *  pointer to object and an integer type
     * 
     *  This is required.  The code needs to know the device address where the
     *  test code is located.  Since this is a fixed address and not a variable,
     *  it requires a cast.  The alternative would be to create a dummy variable
     *  at the address of the test code and point to that instead.  This
     *  alternative would take a custom addressed variable and corresponding
     *  considerations in the linker file.
     */
    const uint32_t* panelVal = (uint32_t *)(FLASH_REGION_5_TEST_CODE_ADDRESS);
    uint32_t physicalEraseAddress = FLASH_ErasePageAddressGet(FLASH_REGION_5_TEST_CODE_ADDRESS);
    
    /* Attempt to make the region writable. */
    PR5CTRLbits.WR = 1;  
       
    /* Erase the page. */
    (void)FLASH_PageErase(physicalEraseAddress, FLASH_UNLOCK_KEY);
    
    /* Test to see if the memory was erased. */
    if (*panelVal == BLANK_INSTRUCTION)
    {
        pageErased = true;
    }
    
    return pageErased;
}