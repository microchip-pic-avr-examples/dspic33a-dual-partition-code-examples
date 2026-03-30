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
#include <stddef.h>

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

#define ACTIVE_PARTITION_BASE_ADDRESS 0x800000UL
#define INACTIVE_PARTITION_BASE_ADDRESS 0xC00000UL

#define ACTIVE_SEQUENCE_NUMBER_ADDRESS 0x83FFF0UL
#define ACTIVE_SEQUENCE_NUMBER_PAGE 0x83F000UL

#define INACTIVE_SEQUENCE_NUMBER_ADDRESS 0xC3FFF0UL
#define INACTIVE_SEQUENCE_NUMBER_PAGE 0xC3F000UL

#define DEMO_PARTITION_SIZE 0x10000UL

#define FLASH_INSTRUCTION_SIZE_IN_BYTES (4UL)
#define FLASH_ERASE_PAGE_SIZE_IN_BYTES (FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS * 4UL)
#define FLASH_WRITE_SIZE_IN_BYTES (4UL * FLASH_INSTRUCTION_SIZE_IN_BYTES)    //4 instructions written at a time

#define PRINT_REGION_SIZE 128

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
static void EraseActiveTestArea(void);
static void EraseInactiveTestArea(void);
static void PrintActiveTestArea(void);
static void PrintInactiveTestArea(void);
static void WriteActiveTestArea(void);
static void WriteInactiveTestArea(void);
static void BulkErase(void);
static void SequenceNumberUpdate(void);
static void BootSwapRequested(void);
static void SequenceInfoPrint(void);
static void SequenceNumberActiveErase(void);
static void SequenceNumberInactiveErase(void);
static void BreakpointDemo(void);

void MENU_Print(void);

enum TYPE {
    TYPE_GROUP,
    TYPE_COMMAND
};

struct COMMAND {
    enum TYPE type;
    char code;
    char description[100];
    void (*execute)(void);
};

static struct COMMAND commands[] = {
    // Sequence Number Options
    {
        .type = TYPE_GROUP,
        .description = "Sequence Number Options",
    },
    {
        .type = TYPE_COMMAND,
        .code = 's',
        .description = "Write the sequence number of the inactive partition",
        .execute = SequenceNumberUpdate
    },
    {
        .type = TYPE_COMMAND,
        .code = 'a',
        .description = "Erase active partition sequence number",
        .execute = SequenceNumberActiveErase
    },
    {
        .type = TYPE_COMMAND,
        .code = 'i',
        .description = "Erase inactive partition sequence number",
        .execute = SequenceNumberInactiveErase
    },
    
    // Flash Panel Lock & Unlock Options
    {
        .type = TYPE_GROUP,
        .description = "Flash Panel Lock & Unlock Options",
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
    
    // Dual Partition Flash Protection Regions: Active Panel
    {
        .type = TYPE_GROUP,
        .description = "Active Partition - Dual Partition Flash Protection Regions (0x810000)",
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
    
    // Dual Partition Flash Protection Regions: Inactive Panel
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

static void SequenceNumberActiveErase(void)
{
    (void)FLASH_PageErase(ACTIVE_SEQUENCE_NUMBER_PAGE, FLASH_UNLOCK_KEY);
}

static void SequenceNumberInactiveErase(void)
{
    (void)FLASH_PageErase(INACTIVE_SEQUENCE_NUMBER_PAGE, FLASH_UNLOCK_KEY);
}

static char panelStrings[4][5] = {
    "DATA",
    "1   ",
    "   2",
    "BOTH"
};

static char* PartitionStringGet(struct FLASH_REGION * const region){
    enum PANEL panel = region->panelGet();
    char* result;
    
    if(panel == PANEL_BOTH){
        result = "BOTH    ";
    } else if(((panel == PANEL_1) && (PARTITION_ActiveGet() == 1)) ||
       ((panel == PANEL_2) && (PARTITION_ActiveGet() == 2))) {
        result = "ACTIVE  ";
    } else {
        result = "INACTIVE";
    }
    
    return result;    
}

static void FlashRegionInfoPrint(void)
{
    (void)printf("  Flash Regions\r\n");
    (void)printf("  -------------------------------------------\r\n");
    (void)printf("  NUMBER | PANEL | PARTITION | WRITE ENABLED \r\n");
    
    for(size_t i=0; i<(sizeof(flashRegion)/sizeof(struct FLASH_REGION * const)); i++) {
        struct FLASH_REGION * const region = flashRegion[i];
        
        (void)printf("   "); //text alignment
        (void)printf("%u        ", i);  //Number
        (void)printf("%s    ", panelStrings[region->panelGet()]); //Panel
        (void)printf("%s   ", PartitionStringGet(region));
        (void)printf("%s", region->isWriteEnabled() ? "true" : "false");
        (void)printf("\r\n");
    }
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

static void PrintRegion(uint32_t address)
{
    uint8_t data[PRINT_REGION_SIZE];
    
    memcpy(data, (void*)address, sizeof(data));
      
    for(int i=0; i<sizeof(data); i++){
        if((i % 16) == 0)
        {
            (void)printf("\r\n  %08lX: ", (unsigned long)(address + i));
        }
        
        printf(" %02lX ", (unsigned long)data[i]);
    }
    
    (void)printf("\r\n\r\n");
}

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


static void PrintActiveTestArea(void)
{
    PrintRegion(0x810000);
}

static void PrintInactiveTestArea(void)
{
    PrintRegion(0xC10000);
}

static void WriteActiveTestArea(void)
{
    flash_data_t data[PRINT_REGION_SIZE/4] = {
        0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F,
        0x10111213, 0x14151617, 0x18191A1B, 0x1C1D1E1F,
        0x20212223, 0x24252627, 0x28292A2B, 0x2C2D2E2F,
        0x30313233, 0x34353637, 0x38393A3B, 0x3C3D3E3F,
        0x40414243, 0x44454647, 0x48494A4B, 0x4C4D4E4F,
        0x50515253, 0x54555657, 0x58595A5B, 0x5C5D5E5F,
        0x60616263, 0x64656667, 0x68696A6B, 0x6C6D6E6F,
        0x70717273, 0x74757677, 0x78797A7B, 0x7C7D7E7F,
    };
        
    FLASH_WordWrite(0x810000, &data[0], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810010, &data[4], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810020, &data[8], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810030, &data[12], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810040, &data[16], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810050, &data[20], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810060, &data[24], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0x810070, &data[28], FLASH_UNLOCK_KEY);
    
    PrintRegion(0x810000);
}

static void WriteInactiveTestArea(void)
{    
    flash_data_t data[PRINT_REGION_SIZE/4] = {
        0xFCFDFEFF, 0xF8F9FAFB, 0xF4F5F6F7, 0xF0F1F2F3,
        0xECEDEEEF, 0xE8E9EAEB, 0xE4E5E6E7, 0xE0E1E2E3,
        0xDCDDDEDF, 0xD8D9DADB, 0xD4D5D6D7, 0xD0D1D2D3,
        0xCCCDCECF, 0xC8C9CACB, 0xC4C5C6C7, 0xC0C1C2C3,
        0xBDBDBEBF, 0xB8B9BABB, 0xB4B5B6B7, 0xB0B1B2B3,
        0xADADAEAF, 0xA8A9AAAB, 0xA4A5A6A7, 0xA0A1A2A3,
        0x9D9D9E9F, 0x98999A9B, 0x94959697, 0x90919293,
        0x8C8D8E8F, 0x88898A8B, 0x84858687, 0x80818283,
    };
        
    FLASH_WordWrite(0xC10000, &data[0], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10010, &data[4], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10020, &data[8], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10030, &data[12], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10040, &data[16], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10050, &data[20], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10060, &data[24], FLASH_UNLOCK_KEY);
    FLASH_WordWrite(0xC10070, &data[28], FLASH_UNLOCK_KEY);
    
    PrintRegion(0xC10000);
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
static void EraseActiveTestArea(void)
{
    FLASH_PageErase(0x810000, FLASH_UNLOCK_KEY);
    PrintRegion(0x810000);
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
static void EraseInactiveTestArea(void)
{
    FLASH_PageErase(0xC10000, FLASH_UNLOCK_KEY);
    PrintRegion(0xC10000);
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
 * @ingroup  menu.c
 * @brief    Determines if the specified sequence number is valid or not.
 * 
 * @param    *sequenceCode - pointer to 128-bit sequence code.
 * @return   bool - true if sequence code is valid
 */
static bool SequenceCodeIsValid(const uint32_t *sequenceCode)
{
    bool codeIsValid = true;
    
    uint16_t BTSEQn = sequenceCode[0] & 0xFFFU;
    uint16_t IBTSEQn = (sequenceCode[0] >> 12) & 0xFFFU;
    
    if( (BTSEQn + IBTSEQn) != 0xFFFU){
        codeIsValid = false;
    }
    
    if((sequenceCode[0] >> 24) != 0U){
        codeIsValid = false;
    }
    
    if((sequenceCode[1] != 0U) ||
       (sequenceCode[2] != 0U) ||
       (sequenceCode[3] != 0U)){
        codeIsValid = false;
    }
    
    return codeIsValid;
}

/**
 * @ingroup  menu.c
 * @brief    Prints sequence number analysis of the specified address
 * 
 * @param    address - the address of the sequence number to analyze
 * @return   none
 */
static void SequenceCodePrint(uint32_t address)
{
    uint32_t sequenceCode[4];
    
    /* cppcheck-suppress misra-c2012-11.6
     * 
     *  (Rule 11.6) REQUIRED: Required: A cast shall not be performed between 
     *  pointer to void and an arithmetic type
     * 
     *  Reasoning: This copies the sequence code value of the requested 
     *  partition into the sequenceCode buffer. Because the address may be the 
     *  inactive partition and therefore lives outside of active partition, 
     *  there is no way to create an object at that address to reference so an 
     *  integer address is used for the address.
     */
    memcpy(sequenceCode, (void*) address, sizeof(sequenceCode));
    
    (void)printf(" @%08X [%08X, %08X, %08X, %08X]", (unsigned int)address, (unsigned int)sequenceCode[0], (unsigned int)sequenceCode[1], (unsigned int)sequenceCode[2], (unsigned int)sequenceCode[3]);
    
    if(SequenceCodeIsValid(sequenceCode) == false){
        (void)printf(" -- INVALID!!");
    }
    
    (void)printf("\r\n");
}

/**
 * @ingroup  menu.c
 * @brief    Prints out the sequence number information for both partitions.
 *           Indicates which partition is currently active and if the sequence
 *           numbers of each partition are valid.
 * 
 * @param    none
 * @return   none
 */
static void SequenceInfoPrint(void)
{
    (void)printf("  Active Partition   (Panel %u) : ", PARTITION_ActiveGet());
    SequenceCodePrint(ACTIVE_SEQUENCE_NUMBER_ADDRESS);
    (void)printf("  Inactive Partition (Panel %u) : ", PARTITION_InactiveGet());
    SequenceCodePrint(INACTIVE_SEQUENCE_NUMBER_ADDRESS);
}
