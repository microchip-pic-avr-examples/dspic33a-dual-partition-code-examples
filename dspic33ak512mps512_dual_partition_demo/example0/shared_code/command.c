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

#define ACTIVE_SEQUENCE_NUMBER_ADDRESS   0x83FFF0UL
#define ACTIVE_SEQUENCE_NUMBER_PAGE      0x83F000UL

#define INACTIVE_SEQUENCE_NUMBER_ADDRESS 0xC3FFF0UL
#define INACTIVE_SEQUENCE_NUMBER_PAGE    0xC3F000UL

#define DEMO_PARTITION_SIZE 0x10000UL

#define FLASH_INSTRUCTION_SIZE_IN_BYTES (4UL)
#define FLASH_ERASE_PAGE_SIZE_IN_BYTES (FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS * 4UL)
#define FLASH_WRITE_SIZE_IN_BYTES (4UL * FLASH_INSTRUCTION_SIZE_IN_BYTES)    //4 instructions written at a time

#define FLASH_REGION_NUMBER_OF_REGIONS   8U

#define FLASHREGION_NUMBER_WIDTH         6U
#define FLASHREGION_PANEL_WIDTH          5U
#define FLASHREGION_PARTITION_WIDTH      9U
#define FLASHREGION_TYPE_WIDTH           11U
#define FLASHREGION_ADDRESS_WIDTH        19U
#define FLASHREGION_LOCK_STATUS_WIDTH    11U
#define FLASHREGION_WRITE_WIDTH          13U

#define FLASH_REGION_LOCK_MASK               0x00000003UL
#define FLASH_REGION_LOCKED                  0x00000000UL
#define FLASH_REGION_LOCKED_UNTIL_RESET      0x00000001UL
#define FLASH_REGION_UNLOCKED                0x00000003UL

#define FLASH_ACTIVE_SPACE_BASE              0x800000UL
#define FLASH_INACTIVE_SPACE_BASE            0xC00000UL

#define PRINT_REGION_SIZE 128

#define SEQINFO_PARTITION_WIDTH     16U
#define SEQINFO_STATE_WIDTH         15U
#define SEQINFO_SEQNUM_WIDTH         9U
#define SEQINFO_INVSEQNUM_WIDTH     17U
#define SEQINFO_VALID_WIDTH          7U
#define SEQINFO_ADDRESS_WIDTH        17U

struct SEQUENCE_INFO
{
    uint32_t address;
    uint16_t sequenceNumber;
    uint16_t inverseSequenceNumber;
    bool valid;
};

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
static void BootSwapRequested(void);
static void SequenceInfoPrint(void);
static void BreakpointDemo(void);
static void SequenceNumberActiveUpdate(void);
static void SequenceNumberInactiveUpdate(void);
static void SequenceInfoGet(uint32_t address, struct SEQUENCE_INFO *info);
static const char* ValidStringGet(bool valid);
static void SequenceInfoRowPrint(uint8_t partitionNumber, const char *stateLabel, uint32_t address);
static void PrintRepeatedChar(char ch, uint8_t count);
static void SequenceInfoSeparatorPrint(void);
static void FlashRegionInfoPrint(void);
static void FlashRegionSeparatorPrint(void);
static uint32_t FlashRegionTypeGet(uint8_t regionNumber);
static uint32_t FlashRegionStartFieldGet(uint8_t regionNumber);
static uint32_t FlashRegionEndFieldGet(uint8_t regionNumber);
static uint32_t FlashRegionLockGet(uint8_t regionNumber);
static const char* FlashRegionTypeStringGet(uint8_t regionNumber);
static const char* FlashRegionLockStatusStringGet(uint8_t regionNumber);
static uint32_t FlashRegionAddressBuild(uint32_t addressField, bool activeSpace);
static void FlashRegionAddressStringGet(uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize);

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
static void SequenceNumberUpdate(flash_adr_t page, flash_adr_t address)
{
    uint32_t sequenceNumber = 0;

    (void)printf("\r\nEnter a 24-bit sequence number (IBTSEQn + BTSEQn): ");

    if(SCAN_Hex((uint8_t*)&sequenceNumber, 6, true))
    {
        uint32_t sequenceNumberArray[4] = {0};

        sequenceNumberArray[0] = sequenceNumber;

        (void)FLASH_PageErase(page, FLASH_UNLOCK_KEY);
        (void)FLASH_WordWrite(address, sequenceNumberArray, FLASH_UNLOCK_KEY);
    }
    else
    {
        (void)printf("\r\n\r\nSequence number not written\r\n\r\n");
    }
}

/**
 * @ingroup  menu.c
 * @brief    Increment and update active partition sequence number
 *
 * @param    none
 * @return   none
 */
static void SequenceNumberActiveUpdate(void)
{
    SequenceNumberUpdate(ACTIVE_SEQUENCE_NUMBER_PAGE, ACTIVE_SEQUENCE_NUMBER_ADDRESS);
}

/**
 * @ingroup  menu.c
 * @brief    Increment and update inactive partition sequence number
 *
 * @param    none
 * @return   none
 */
static void SequenceNumberInactiveUpdate(void)
{
    SequenceNumberUpdate(INACTIVE_SEQUENCE_NUMBER_PAGE, INACTIVE_SEQUENCE_NUMBER_ADDRESS);
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

    if((BTSEQn + IBTSEQn) != 0xFFFU)
    {
        codeIsValid = false;
    }

    if((sequenceCode[0] >> 24) != 0U)
    {
        codeIsValid = false;
    }

    if((sequenceCode[1] != 0U) ||
       (sequenceCode[2] != 0U) ||
       (sequenceCode[3] != 0U))
    {
        codeIsValid = false;
    }

    return codeIsValid;
}

/**
 * @ingroup  menu.c
 * @brief    Reads and decodes the sequence information at the specified address.
 *
 * @param    address - address of the sequence number
 * @param    info - destination structure
 * @return   none
 */
static void SequenceInfoGet(uint32_t address, struct SEQUENCE_INFO *info)
{
    uint32_t sequenceCode[4];

    /* cppcheck-suppress misra-c2012-11.6
     *
     *  (Rule 11.6) REQUIRED: A cast shall not be performed between
     *  pointer to void and an arithmetic type
     *
     *  Reasoning: This copies the sequence code value of the requested
     *  partition into the sequenceCode buffer. Because the address may live
     *  outside of active partition, there is no way to create an object at
     *  that address to reference so an integer address is used.
     */
    memcpy(sequenceCode, (void*)address, sizeof(sequenceCode));

    info->address = address;
    info->sequenceNumber = (uint16_t)(sequenceCode[0] & 0xFFFU);
    info->inverseSequenceNumber = (uint16_t)((sequenceCode[0] >> 12) & 0xFFFU);
    info->valid = SequenceCodeIsValid(sequenceCode);
}

/**
 * @ingroup  menu.c
 * @brief    Returns "Valid" or "Invalid" for sequence number display.
 *
 * @param    valid - validity state
 * @return   const char* - validity string
 */
static const char* ValidStringGet(bool valid)
{
    if(valid)
    {
        return "Valid";
    }
    else
    {
        return "Invalid";
    }
}

/**
 * @ingroup  menu.c
 * @brief    Prints one row of the sequence number table.
 *
 * @param    partitionNumber - current partition number
 * @param    stateLabel - displayed state label ("Active" or "Inactive")
 * @param    address - sequence number address
 * @return   none
 */
static void SequenceInfoRowPrint(uint8_t partitionNumber, const char *stateLabel, uint32_t address)
{
    struct SEQUENCE_INFO info;
    char addressString[18];

    SequenceInfoGet(address, &info);

    (void)sprintf(addressString, "0x%06lX", (unsigned long)info.address);

    (void)printf("  %-*s | %-*s | %-*.3X | %-*.3X | %-*s | %-*s\r\n",
                 SEQINFO_PARTITION_WIDTH,
                 (partitionNumber == 1U) ? "Partition 1" : "Partition 2",
                 SEQINFO_STATE_WIDTH,
                 stateLabel,
                 SEQINFO_SEQNUM_WIDTH,
                 (unsigned int)info.sequenceNumber,
                 SEQINFO_INVSEQNUM_WIDTH,
                 (unsigned int)info.inverseSequenceNumber,
                 SEQINFO_VALID_WIDTH,
                 ValidStringGet(info.valid),
                 SEQINFO_ADDRESS_WIDTH,
                 addressString);
}

/**
 * @ingroup  menu.c
 * @brief    Prints sequence number information for active and inactive partitions
 *           in a table format. 
 *
 * @param    none
 * @return   none
 */
static void SequenceInfoPrint(void)
{
    (void)printf("  Sequence Number\r\n");
    (void)printf("  ------------------------------------------------------------------------------------------------\r\n");
    (void)printf("  %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                 SEQINFO_PARTITION_WIDTH, "Partition Number",
                 SEQINFO_STATE_WIDTH, "Active/Inactive",
                 SEQINFO_SEQNUM_WIDTH, "Seq. Num.",
                 SEQINFO_INVSEQNUM_WIDTH, "Inverse Seq. Num.",
                 SEQINFO_VALID_WIDTH, "Valid",
                 SEQINFO_ADDRESS_WIDTH, "Seq. Num. Address");

    SequenceInfoSeparatorPrint();

    SequenceInfoRowPrint(PARTITION_ActiveGet(), "Active", ACTIVE_SEQUENCE_NUMBER_ADDRESS);
    SequenceInfoRowPrint(PARTITION_InactiveGet(), "Inactive", INACTIVE_SEQUENCE_NUMBER_ADDRESS);
}

/**
 * @ingroup  menu.c
 * @brief    Prints a specified character a given number of times.
 *
 * @param    ch - character to print
 * @param    count - number of times to print
 * @return   none
 */
static void PrintRepeatedChar(char ch, uint8_t count)
{
    uint8_t i;

    for(i = 0U; i < count; i++)
    {
        (void)putchar((int)ch);
    }
}

/**
 * @ingroup  menu.c
 * @brief    Prints a separator row for the sequence number information table.
 *
 * @param    none
 * @return   none
 */
static void SequenceInfoSeparatorPrint(void)
{
    (void)printf("  ");
    PrintRepeatedChar('-', SEQINFO_PARTITION_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', SEQINFO_STATE_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', SEQINFO_SEQNUM_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', SEQINFO_INVSEQNUM_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', SEQINFO_VALID_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', SEQINFO_ADDRESS_WIDTH);
    (void)printf("\r\n");
}

/**
 * @ingroup  menu.c
 * @brief    Prints the flash region table separator line.
 *
 * @param    none
 * @return   none
 */
static void FlashRegionSeparatorPrint(void)
{
    (void)printf("  ");
    PrintRepeatedChar('-', FLASHREGION_NUMBER_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_PANEL_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_PARTITION_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_TYPE_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_ADDRESS_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_ADDRESS_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_LOCK_STATUS_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_WRITE_WIDTH);
    (void)printf("\r\n");
}

/**
 * @ingroup  menu.c
 * @brief    Gets the RTYPE field of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - RTYPE field
 */
static uint32_t FlashRegionTypeGet(uint8_t regionNumber)
{
    switch(regionNumber)
    {
        case 0U: return PR0CTRLbits.RTYPE;
        case 1U: return PR1CTRLbits.RTYPE;
        case 2U: return PR2CTRLbits.RTYPE;
        case 3U: return PR3CTRLbits.RTYPE;
        case 4U: return PR4CTRLbits.RTYPE;
        case 5U: return PR5CTRLbits.RTYPE;
        case 6U: return PR6CTRLbits.RTYPE;
        case 7U: return PR7CTRLbits.RTYPE;
        default: return 0U;
    }
}

/**
 * @ingroup  menu.c
 * @brief    Gets the START field of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - START field
 */
static uint32_t FlashRegionStartFieldGet(uint8_t regionNumber)
{
    switch(regionNumber)
    {
        case 0U: return PR0STbits.START;
        case 1U: return PR1STbits.START;
        case 2U: return PR2STbits.START;
        case 3U: return PR3STbits.START;
        case 4U: return PR4STbits.START;
        case 5U: return PR5STbits.START;
        case 6U: return PR6STbits.START;
        case 7U: return PR7STbits.START;
        default: return 0U;
    }
}

/**
 * @ingroup  menu.c
 * @brief    Gets the END field of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - END field
 */
static uint32_t FlashRegionEndFieldGet(uint8_t regionNumber)
{
    switch(regionNumber)
    {
        case 0U: return PR0ENDbits.END;
        case 1U: return PR1ENDbits.END;
        case 2U: return PR2ENDbits.END;
        case 3U: return PR3ENDbits.END;
        case 4U: return PR4ENDbits.END;
        case 5U: return PR5ENDbits.END;
        case 6U: return PR6ENDbits.END;
        case 7U: return PR7ENDbits.END;
        default: return 0U;
    }
}

/**
 * @ingroup  menu.c
 * @brief    Gets the LOCK register value of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - lock register value
 */
static uint32_t FlashRegionLockGet(uint8_t regionNumber)
{
    switch(regionNumber)
    {
        case 0U: return PR0LOCK;
        case 1U: return PR1LOCK;
        case 2U: return PR2LOCK;
        case 3U: return PR3LOCK;
        case 4U: return PR4LOCK;
        case 5U: return PR5LOCK;
        case 6U: return PR6LOCK;
        case 7U: return PR7LOCK;
        default: return 0U;
    }
}

/**
 * @ingroup  menu.c
 * @brief    Returns the flash region type string.
 *
 * @param    regionNumber - flash region number
 * @return   const char* - region type string
 */
static const char* FlashRegionTypeStringGet(uint8_t regionNumber)
{
    uint32_t regionType = FlashRegionTypeGet(regionNumber);

    switch(regionType)
    {
        case 0x1U:
            return "IRT";

        case 0x2U:
            return "OTP";

        case 0x3U:
            return "FIRMWARE";

        default:
            return "UNKNOWN";
    }
}

/**
 * @ingroup  menu.c
 * @brief    Returns the flash region lock status string.
 *
 * @param    regionNumber - flash region number
 * @return   const char* - lock status string
 */
static const char* FlashRegionLockStatusStringGet(uint8_t regionNumber)
{
    uint32_t lockValue = FlashRegionLockGet(regionNumber) & FLASH_REGION_LOCK_MASK;

    switch(lockValue)
    {
        case FLASH_REGION_LOCKED:
            return "Locked";

        case FLASH_REGION_LOCKED_UNTIL_RESET:
            return "Until Reset";

        case FLASH_REGION_UNLOCKED:
            return "Unlocked";

        default:
            return "UNKNOWN";
    }
}

/**
 * @ingroup  menu.c
 * @brief    Builds a display address in active or inactive address space.
 *
 * @param    addressField - START or END field from PRxST/PRxEND
 * @param    activeSpace - true for 0x8xxxxx, false for 0xCxxxxx
 * @return   uint32_t - display address
 */
static uint32_t FlashRegionAddressBuild(uint32_t addressField, bool activeSpace)
{
    uint32_t displayAddress;

    /* NOTE:
     * If START/END fields require shifting/scaling, update this line.
     */
    displayAddress = addressField;

    if(activeSpace)
    {
        displayAddress |= FLASH_ACTIVE_SPACE_BASE;
    }
    else
    {
        displayAddress |= FLASH_INACTIVE_SPACE_BASE;
    }

    return displayAddress;
}

/**
 * @ingroup  menu.c
 * @brief    Formats the active/inactive address pair string.
 *
 * @param    addressField - START or END field from PRxST/PRxEND
 * @param    buffer - destination string buffer
 * @param    bufferSize - destination buffer size
 * @return   none
 */
static void FlashRegionAddressStringGet(uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize)
{
    uint32_t activeAddress = FlashRegionAddressBuild(addressField, true);
    uint32_t inactiveAddress = FlashRegionAddressBuild(addressField, false);

    (void)snprintf(buffer,
                   bufferSize,
                   "0x%06lX/0x%06lX",
                   (unsigned long)activeAddress,
                   (unsigned long)inactiveAddress);
}

/**
 * @ingroup  menu.c
 * @brief    Prints information about all flash regions, including panel,
 *           partition, type, start/end addresses, lock status, and write
 *           enable status.
 *
 * @param    none
 * @return   none
 */
static void FlashRegionInfoPrint(void)
{
    uint8_t i;
    char startAddressString[24];
    char endAddressString[24];

    (void)printf("  Flash Regions\r\n");
    (void)printf("  ----------------------------------------------------------------------------------------------------------------------\r\n");
    (void)printf("  %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                 FLASHREGION_NUMBER_WIDTH, "NUMBER",
                 FLASHREGION_PANEL_WIDTH, "PANEL",
                 FLASHREGION_PARTITION_WIDTH, "PARTITION",
                 FLASHREGION_TYPE_WIDTH, "REGION TYPE",
                 FLASHREGION_ADDRESS_WIDTH, "START ADDRESS",
                 FLASHREGION_ADDRESS_WIDTH, "END ADDRESS",
                 FLASHREGION_LOCK_STATUS_WIDTH, "LOCK STATUS",
                 FLASHREGION_WRITE_WIDTH, "WRITE ENABLED");

    FlashRegionSeparatorPrint();

    for(i = 0U; i < FLASH_REGION_NUMBER_OF_REGIONS; i++)
    {
        FlashRegionAddressStringGet(FlashRegionStartFieldGet(i),
                                    startAddressString,
                                    sizeof(startAddressString));

        FlashRegionAddressStringGet(FlashRegionEndFieldGet(i),
                                    endAddressString,
                                    sizeof(endAddressString));

        (void)printf("  %-*u | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                     FLASHREGION_NUMBER_WIDTH, (unsigned int)i,
                     FLASHREGION_PANEL_WIDTH, panelStrings[flashRegion[i]->panelGet()],
                     FLASHREGION_PARTITION_WIDTH, PartitionStringGet(flashRegion[i]),
                     FLASHREGION_TYPE_WIDTH, FlashRegionTypeStringGet(i),
                     FLASHREGION_ADDRESS_WIDTH, startAddressString,
                     FLASHREGION_ADDRESS_WIDTH, endAddressString,
                     FLASHREGION_LOCK_STATUS_WIDTH, FlashRegionLockStatusStringGet(i),
                     FLASHREGION_WRITE_WIDTH, flashRegion[i]->isWriteEnabled() ? "true" : "false");
    }
}