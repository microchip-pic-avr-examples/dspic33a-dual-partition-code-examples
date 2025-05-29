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

#include <stddef.h>

/* cppcheck-suppress misra-c2012-21.6
 * 
 *  (Rule 21.6) Required: The standard library input/output functions shall 
 *  not be used
 * 
 *  Use of stdio.h is required in order to run this terminal based demo. 
 *  This demo is for learning purposes only. However in production code, 
 *  stdio.h should be avoided, as streams and file I/O functions lack type 
 *  safety and have undefined and unspecified behavior at run time. 
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "xc.h"
#include "bsp/led0.h"
#include "bsp/led7.h"

#include "mcc_generated_files/flash/flash_types.h"
#include "mcc_generated_files/flash/flash_nonblocking.h"
#include "mcc_generated_files/uart/uart1.h"
#include "mcc_generated_files/timer/tmr1.h"

#include "reset.h"
#include "partition.h"
#include "scan.h"
#include "command.h"
#include "flash_regions/flash_region.h"
#include "flash_regions/flash_region_0.h"
#include "flash_regions/flash_region_1.h"
#include "flash_regions/flash_region_2.h"
#include "flash_regions/flash_region_3.h"
#include "flash_regions/flash_region_4.h"
#include "flash_regions/flash_region_5.h"
#include "flash_regions/flash_region_6.h"
#include "flash_regions/flash_region_7.h"

#define INACTIVE_SEQUENCE_NUMBER_ADDRESS 0xC3FFF0UL
#define INACTIVE_SEQUENCE_NUMBER_PAGE 0xC3F000UL

#define ACTIVE_SEQUENCE_NUMBER_ADDRESS 0x83FFF0UL
#define ACTIVE_SEQUENCE_NUMBER_PAGE 0x83F000UL

#define ACTIVE_PARTITION_BASE_ADDRESS 0x800000UL
#define INACTIVE_PARTITION_BASE_ADDRESS 0xC00000UL

#define DEMO_PARTITION_SIZE 0x10000UL

#define FLASH_INSTRUCTION_SIZE_IN_BYTES (4UL)
#define FLASH_ERASE_PAGE_SIZE_IN_BYTES (FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS * 4UL)
#define FLASH_WRITE_SIZE_IN_BYTES (4UL * FLASH_INSTRUCTION_SIZE_IN_BYTES)    //4 instructions written at a time

/******************************************************************************/
/* Extern Function Prototypes                                                 */
/******************************************************************************/
extern void BreakpointExample(void);
extern void BootSwap(void);

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static bool GetRegionSelection(uint8_t* regionNum);
static void UnlockRegion(void);
static void LockRegion(void);
static void LockRegionUntilReset(void);
static void EraseTestArea(void);
static void BulkErase(void);
static void SequenceNumberUpdate(void);
static void BootSwapRequested(void);
static void ImageCopy(void);

static struct FLASH_REGION * const flashRegion[] = 
{
    &flashRegion0,
    &flashRegion1,
    &flashRegion2,
    &flashRegion3,
    &flashRegion4,
    &flashRegion5,
    &flashRegion6,
    &flashRegion7,
};

void COMMAND_Process(char command)
{    
    switch(command)
    {
        case 's':
            SequenceNumberUpdate();
            break;
        
        case 'a':
            (void)FLASH_PageErase(ACTIVE_SEQUENCE_NUMBER_PAGE, FLASH_UNLOCK_KEY);
            break;
        
        case 'i':
            (void)FLASH_PageErase(INACTIVE_SEQUENCE_NUMBER_PAGE, FLASH_UNLOCK_KEY);
            break;

        case 'u':
            UnlockRegion();
            break;
            
        case 'l':
            LockRegion();
            break;
            
        case 'x':
            LockRegionUntilReset();
            break;
            
        case 'e':
            EraseTestArea();
            break;
            
        case 'c':
            ImageCopy();
            break;
        
        case 'p':
            BulkErase();
            break;
            
        case 'b':
            BootSwapRequested();
            break;
                    
        case 'q':
            /* Wait until all UART data is transmitted before triggering the
             * breakpoint to prevent garbled terminal data. */
            while(UART1_IsTxDone() == false)
            {
            }
            
            BreakpointExample();
            break;
            
        case 'r':
            RESET_DeviceReset();
            break;

        default:
            (void)printf("Invalid request.\r\n");
            break;
    }
}

static bool RegionNumIsValid(uint8_t regionNum)
{  
    return (regionNum <= 7U);
}

/**
 * @ingroup  command.c
 * @brief    Scan user input for a flash region (0-7 allowed)
 * 
 * @param    none
 * @return   none
 */
static bool GetRegionSelection(uint8_t* regionNum)
{    
    (void)printf("Enter the region number (0-7): ");
    
    char inputChar = SCAN_Char(true);  // Store the result of SCAN_Char
    *regionNum = (uint8_t)((uint8_t)inputChar - (uint8_t)'0');  // Convert from ASCII to integer
    
    (void)printf("\r\n\r\n");
    
    bool isValid = RegionNumIsValid(*regionNum);
        
    if(isValid == false)
    {
        (void)printf("Invalid region number.\r\n\r\n");
    }

    return isValid;
}

/**
 * @ingroup  command.c
 * @brief    Unlocks the specified flash protection region and leaves it
 *           unlocked.
 * 
 * @param    none
 * @return   none
 */
static void UnlockRegion(void)
{
    uint8_t regionNum = 0;
    bool validInput = GetRegionSelection(&regionNum);

    if(validInput)
    {
        struct FLASH_REGION * const region = flashRegion[regionNum];
        
        if (region->lockOptionSet(FLASH_PROTECTION_UNLOCKED))
        {
            (void)printf("Region %i successfully unlocked.\r\n\r\n", (char)regionNum);
        } 
        else
        {
            (void)printf("Region %i failed to unlock.\r\n\r\n", (char)regionNum);
        }
    }
}

/**
 * @ingroup  command.c
 * @brief    Locks the specified flash region but the region is still unlockable
 *           with an unlock request.
 * 
 * @param    none
 * @return   none
 */
static void LockRegion(void)
{
    uint8_t regionNum = 0;
    bool validInput = GetRegionSelection(&regionNum);

    if(validInput)
    {
        struct FLASH_REGION * const region = flashRegion[regionNum];
        
        if (region->lockOptionSet(FLASH_PROTECTION_LOCKED))
        {
            (void)printf("Region %i successfully locked (can be unlocked).\r\n\r\n", regionNum);
        } 
        else
        {
            (void)printf("Region %i failed to lock.\r\n\r\n", regionNum);
        }
    }
}

/**
 * @ingroup  command.c
 * @brief    Locks the specified flash region until the next reset.  This can't
 *           be unlocked without a reset.
 * 
 * @param    none
 * @return   none
 */
static void LockRegionUntilReset(void)
{
    uint8_t regionNum = 0;
    bool validInput = GetRegionSelection(&regionNum);

    if(validInput)
    {
        struct FLASH_REGION * const region = flashRegion[regionNum];
        
        if (region->lockOptionSet(FLASH_PROTECTION_LOCKED_UNTIL_RESET))
        {
            (void)printf("Region %i successfully locked until reset.\r\n\r\n", regionNum);
        } 
        else
        {
            (void)printf("Region %i failed to lock until reset.\r\n\r\n", regionNum);
        }  
    }
}

/**
 * @ingroup  command.c
 * @brief    Erases the test area in the specified flash region. Used to test
 *           the flash region protection settings. Use the unlock('u'), lock('l')
 *           and lock until reset('x') commands to test out various situations
 *           regarding the flash region lock.
 * 
 * @param    none
 * @return   none
 */
static void EraseTestArea(void)
{
    uint8_t regionNum = 0;
    bool validInput = GetRegionSelection(&regionNum);

    if(validInput)
    {
        struct FLASH_REGION * const region = flashRegion[regionNum];
        
        if (region->eraseTestArea())
        {
            (void)printf("Page from flash region %i successfully erased.\r\n\r\n", regionNum);
        } 
        else
        {
            (void)printf("Page from flash region %i failed erase.\r\n\r\n", regionNum);
        }
    }
}

/**
 * @ingroup  command.c
 * @brief    Issues a bulk/panel/partition erase on the inactive partition.
 * @param    none
 * @return   none
 */
static void BulkErase(void)
{
    if (FLASH_BulkErase(INACTIVE_PARTITION_BASE_ADDRESS, FLASH_UNLOCK_KEY) == FLASH_NO_ERROR)
    {
        (void)printf("Inactive Partition successfully erased.\r\n\r\n");
    }
    else 
    {
        (void)printf("Error! Inactive Partition could not be erased.\r\n\r\n");
    }
}

/**
 * @ingroup  command.c
 * @brief    Update the sequence number of the inactive partition.
 * 
 *           NOTE: Invalid values are allowed to be programmed.  This allows 
 *           testing of invalid sequence numbers for boot swapping and reset.
 * @param    none
 * @return   none
 */
static void SequenceNumberUpdate(void)
{
    uint32_t sequenceNumber = 0;

    (void)printf("\r\nEnter a 24-bit sequence number (IBTSEQn + BTSEQn): ");

    if(SCAN_Hex((uint8_t*)&sequenceNumber, 6, true))
    {
        uint32_t sequenceNumberArray[4] = {0};

        sequenceNumberArray[0] = sequenceNumber;

        (void)FLASH_PageErase(INACTIVE_SEQUENCE_NUMBER_PAGE, FLASH_UNLOCK_KEY);
        (void)FLASH_WordWrite(INACTIVE_SEQUENCE_NUMBER_ADDRESS, sequenceNumberArray, FLASH_UNLOCK_KEY);
    }
    else
    {
        (void)printf("\r\n\r\nSequence number not written\r\n\r\n");
    }
}

/**
 * @ingroup  command.c
 * @brief    Attempts to swap to the inactive partition.  NOTE: this function
 *           will not return if the swap was successful.  We disable the timer
 *           before the swap because aren't sure if the interrupt handling 
 *           routines in the inactive partition are configured identically to
 *           this partition.  In this demo we re-initialize the entire code base
 *           on a swap which covers the C initializer and interrupt
 *           configuration.
 * 
 *           If this function returns, then the swap has failed.
 * @param    none
 * @return   none
 */
static void BootSwapRequested(void)
{
    /* Stop the timer in case we swap. */
    TMR1_Stop();

    /* Swap partitions - this function will not return on a
     * successful swap. */
    BootSwap();

    /* If the above function returned, then the swap failed.
     * Print a message and restart the interrupts. */
    (void)printf("  BOOT SWAP FAILED!!\r\n\r\n");
    TMR1_Start();
}

/**
 * @ingroup  command.c
 * @brief    Copies the code from the active partition to the inactive partition.
 *           This could be needed if some of the tests are run that erase part
 *           of the code in the inactive partition.
 * 
 *           NOTE: This does not copy the entire code memory.  It copies a 
 *           DEMO_PARTITION_SIZE size block.
 * @param    none
 * @return   none
 */
static void ImageCopy(void)
{
    /* cppcheck-suppress misra-c2012-11.6
     * 
     *  (Rule 11.6) REQUIRED: Required: A cast shall not be performed between 
     *  pointer to void and an arithmetic type
     * 
     *  Reasoning: This checks that the active partition and inactive partition
     *  are not already equal before performing an image copy. Because the 
     *  address of the inactive partition lives outside of active partition,
     *  there is no way to create an object at that address to reference so
     *  an integer address is used for the inactive partition base address.
     */
    if(memcmp((void*)ACTIVE_PARTITION_BASE_ADDRESS, (void*)INACTIVE_PARTITION_BASE_ADDRESS, DEMO_PARTITION_SIZE) != 0)
    {
        (void)printf("Copying demo code into inactive partition.\r\n\r\n");
        
        /* Erase the inactive partition just in case. */
        for(uint32_t offset = 0; offset < DEMO_PARTITION_SIZE; offset += FLASH_ERASE_PAGE_SIZE_IN_BYTES)
        {
            (void)FLASH_PageErase(INACTIVE_PARTITION_BASE_ADDRESS + offset, FLASH_UNLOCK_KEY);
        }

        /* Copy the demo code over from the active partition to the inactive partition. */
        for(uint32_t offset = 0; offset < DEMO_PARTITION_SIZE; offset += FLASH_WRITE_SIZE_IN_BYTES)
        {
            //Copy the active partition into the inactive partition
            
            /* cppcheck-suppress misra-c2012-11.4
            * 
            *  (Rule 11.4) ADVISORY: A conversion should not be performed between a
            *  pointer to object and an integer type
            * 
            *  This is required.  The code needs to know the device address
            *  where the active partition is located.  Since this is a fixed 
            *  address and not a variable, it requires a cast.  The alternative 
            *  would be to create a dummy variable at the address of the active 
            *  partition start and point to that instead.  This alternative 
            *  would take a custom addressed variable and corresponding
            *  considerations in the linker file.
            */
            FLASH_WordWrite(INACTIVE_PARTITION_BASE_ADDRESS + offset, (flash_data_t *)(ACTIVE_PARTITION_BASE_ADDRESS + offset), FLASH_UNLOCK_KEY);
        }
    }
}
