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
#include <stddef.h>

#include <xc.h>
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
#include "flash_region_info.h"
#include "sequence_info.h"
#include "test_area_demo.h"
#include "flash_regions/flash_region.h"
#include "flash_regions/flash_region_0.h"
#include "flash_regions/flash_region_1.h"
#include "flash_regions/flash_region_2.h"
#include "flash_regions/flash_region_3.h"
#include "flash_regions/flash_region_4.h"
#include "flash_regions/flash_region_5.h"
#include "flash_regions/flash_region_6.h"
#include "flash_regions/flash_region_7.h"

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
static void BootSwapRequested(void);
static void BreakpointDemo(void);

void MENU_Print(void);

enum TYPE {
    TYPE_GROUP,
    TYPE_COMMAND
};

struct COMMAND {
    enum TYPE type;
    char code;
    const char *description;
    void (*execute)(void);
};

static struct COMMAND commands[] = {    
    // Flash Partition Lock & Unlock Options
    {
        .type = TYPE_GROUP,
        .description = "Flash Partition Lock & Unlock Options",
    },
    {
        .type = TYPE_COMMAND,
        .code = 'u',
        .description = "Set a flash protection region to UNLOCKED",
        .execute = UnlockRegion
    },
    {
        .type = TYPE_COMMAND,
        .code = 'l',
        .description = "Set a flash protection region to LOCKED",
        .execute = LockRegion
    },
    {
        .type = TYPE_COMMAND,
        .code = 'x',
        .description = "Set a flash protection region to LOCKED UNTIL RESET",
        .execute = LockRegionUntilReset
    },
    
    // Dual Partition Flash Protection Regions: Active Partition
    {
        .type = TYPE_GROUP,
        .description = "Active Partition",
    },
    {
        .type = TYPE_COMMAND,
        .code = 'p',
        .description = "Print the test area of the active partition",
        .execute = PrintActiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 'e',
        .description = "Erase the test area of the active partition",
        .execute = EraseActiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 'w',
        .description = "Write test data to the test area of the active partition",
        .execute = WriteActiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 's',
        .description = "Write the sequence number of the active partition",
        .execute = SequenceNumberActiveUpdate
    },
    
    // Dual Partition Flash Protection Regions: Inactive Partition
    {
        .type = TYPE_GROUP,
        .description = "Inactive Partition - Dual Partition Flash Protection Regions (0xC10000)",
    },
    {
        .type = TYPE_COMMAND,
        .code = 'P',
        .description = "Print the test area of the inactive partition",
        .execute = PrintInactiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 'E',
        .description = "Erase the test area of the inactive partition",
        .execute = EraseInactiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 'W',
        .description = "Write test data to the test area of the inactive partition",
        .execute = WriteInactiveTestArea
    },
    {
        .type = TYPE_COMMAND,
        .code = 'T',
        .description = "Bulk erase the inactive partition",
        .execute = BulkErase
    },
    {
        .type = TYPE_COMMAND,
        .code = 'S',
        .description = "Write the sequence number of the inactive partition",
        .execute = SequenceNumberInactiveUpdate
    },
    
    // Bootswap, Debug, & Reset Options
    {
        .type = TYPE_GROUP,
        .description = "Bootswap, Debug, & Reset Options",
    },
    {
        .type = TYPE_COMMAND,
        .code = 'b',
        .description = "Swap active/inactive partitions: BOOTSWP",
        .execute = BootSwapRequested
    },
    {
        .type = TYPE_COMMAND,
        .code = 'q',
        .description = "Run the breakpoint function",
        .execute = BreakpointDemo
    },
    {
        .type = TYPE_COMMAND,
        .code = 'r',
        .description = "Issue software RESET",
        .execute = RESET_DeviceReset
    },
};

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

static void BreakpointDemo(void)
{
    /* Wait until all UART data is transmitted before triggering the
     * breakpoint to prevent garbled terminal data. */
    while(UART1_IsTxDone() == false)
    {
    }

    BreakpointExample();
}

/**
 * @ingroup  menu.c
 * @brief    Prints the menu options.
 * 
 * @param    none
 * @return   none
 */
void MENU_Print(void)
{
    (void)printf("\r\n");
    (void)printf("\r\n");
    (void)printf("=============================================\r\n");
    (void)printf("Dual Partition Demo\r\n");
    (void)printf("=============================================\r\n");

    RESET_PrintResetSources();
    
    /* Clear all sources */
    RCON = 0;
    
    (void)printf("\r\n");
    
    SequenceInfoPrint();
    
    (void)printf("\r\n");
    
    FlashRegionInfoPrint();
    
    for(size_t i=0; i<(sizeof(commands)/sizeof(struct COMMAND)); i++)
    {
        struct COMMAND* command = &commands[i];
        
        if(command->type == TYPE_GROUP){
            (void)printf("\r\n");
            (void)printf("  %s:\r\n", command->description);
            (void)printf("  -------------------------------------------\r\n");
        } else {
            (void)printf("    '%c' : %s\r\n", command->code, command->description);
        }
    }
    
    (void)printf("\r\n");
    (void)printf("Command: ");
}

/*
 * @ingroup  command.c
 * @brief    Processes the user's command input.
 * @param    none
 * @return   none
 */
void COMMAND_Process(void)
{    
    struct COMMAND* command = NULL;

    MENU_Print();

    char command_code = SCAN_Char(true);

    (void)printf("\r\n\r\n");

    for(size_t i=0; i<(sizeof(commands)/sizeof(struct COMMAND)); i++) {
        if(commands[i].code == command_code) {
            command = &commands[i];
            break;
        }
    }
    
    if(command == NULL) {
        (void)printf("Invalid request.\r\n");
    } else {
        command->execute();
    }
}

/*
 * @ingroup  command.c
 * @brief    Checks if the specified region number is valid.
 * @param    regionNum - the region number to check
 * @return   true if the region number is valid, false otherwise
 */
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