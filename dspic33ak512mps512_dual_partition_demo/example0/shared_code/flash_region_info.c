/*
(c) [2025] Microchip Technology Inc. and its subsidiaries.
    Subject to your compliance with these terms, you may use Microchip
    software and any derivatives exclusively with Microchip products.
    You are responsible for complying with 3rd party license terms
    applicable to your use of 3rd party software (including open source
    software) that may accompany Microchip software. SOFTWARE IS “AS IS.”
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP’S
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR
    THIS SOFTWARE.
*/

#include <stdint.h>
#include <stdbool.h>
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

#include <xc.h>

#include "mcc_generated_files/flash/flash_types.h"

#include "partition.h"
#include "flash_region_info.h"

#define FLASH_REGION_NUMBER_OF_REGIONS 4U

#define FLASHREGION_NUMBER_WIDTH 6U
#define FLASHREGION_PARTITION_WIDTH 12U
#define FLASHREGION_TYPE_WIDTH 11U
#define FLASHREGION_ADDRESS_WIDTH 19U
#define FLASHREGION_WRITE_WIDTH 13U

#define FLASH_ACTIVE_SPACE_BASE 0x800000UL
#define FLASH_INACTIVE_SPACE_BASE 0xC00000UL

#define FLASH_REGION_0_TEST_CODE_ADDRESS 0x810000UL
#define FLASH_REGION_2_TEST_CODE_ADDRESS 0x812000UL
#define FLASH_REGION_3_TEST_CODE_ADDRESS 0xC12000UL


enum PARTITION
{
    PARTITION_1 = 0,
    PARTITION_2,
    PARTITION_BOTH
};

/* Reserve flash test blocks used for testing. These placeholder objects prevent application code from being linked into
 * the test ranges. The configuration bits define the test areas as follows:
 * Region 0: 0x810000-0x811FFF (Both Partitions)
 * Region 1: 0x811000-0x811FFF (Partition 2)
 * Region 2: 0x812000-0x812FFF (Partition 1)
 * Region 3: 0x812000-0x812FFF (Partition 2)
 */
static const volatile uint32_t flashRegion0and1TestReserve[(FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS * 2U) - 1U] __attribute__((address(FLASH_REGION_0_TEST_CODE_ADDRESS), space(prog), keep, used)) = {0};
static const volatile uint32_t flashRegion2TestReserve[FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS - 1U] __attribute__((address(FLASH_REGION_2_TEST_CODE_ADDRESS), space(prog), keep, used)) = {0};
static const volatile uint32_t flashRegion3TestReserve[FLASH_ERASE_PAGE_SIZE_IN_INSTRUCTIONS - 1U] __attribute__((address(FLASH_REGION_3_TEST_CODE_ADDRESS), space(prog), keep, used)) = {0};

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static const char *FlashRegionPartitionStringGet(uint8_t regionNumber);
static void PrintRepeatedChar(char ch, uint8_t count);
static void FlashRegionSeparatorPrint(void);
static uint32_t FlashRegionPartitionSelectGet(uint8_t regionNumber);
static uint32_t FlashRegionWriteEnableGet(uint8_t regionNumber);
static uint32_t FlashRegionTypeGet(uint8_t regionNumber);
static uint32_t FlashRegionStartFieldGet(uint8_t regionNumber);
static uint32_t FlashRegionEndFieldGet(uint8_t regionNumber);
static enum PARTITION FlashRegionPartitionGet(uint8_t regionNumber);
static bool FlashRegionIsWriteEnabled(uint8_t regionNumber);
static const char *FlashRegionTypeStringGet(uint8_t regionNumber);
static uint32_t FlashRegionAddressBuild(uint32_t addressField, bool activeSpace);
static void FlashRegionAddressStringGet(uint8_t regionNumber,
                                        uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize);

/*
 * @ingroup  menu.c
 * @brief    Returns the flash region partition string based on the region's
 *           partition assignment and the active partition.
 *
 * @param    regionNumber - flash region number
 * @return   const char* - flash region partition string
 */
static const char *FlashRegionPartitionStringGet(uint8_t regionNumber)
{
    enum PARTITION partition = FlashRegionPartitionGet(regionNumber);
    const char *result = "BOTH";

    if(partition == PARTITION_BOTH)
    {
        result = "BOTH";
    }
    else if(((partition == PARTITION_1) && (PARTITION_ActiveGet() == 1U)) ||
            ((partition == PARTITION_2) && (PARTITION_ActiveGet() == 2U)))
    {
        if(partition == PARTITION_1)
        {
            result = "1 (ACTIVE)";
        }
        else
        {
            result = "2 (ACTIVE)";
        }
    }
    else
    {
        if(partition == PARTITION_1)
        {
            result = "1 (INACTIVE)";
        }
        else
        {
            result = "2 (INACTIVE)";
        }
    }

    return result;
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
    PrintRepeatedChar('-', FLASHREGION_PARTITION_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_TYPE_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_ADDRESS_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_ADDRESS_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', FLASHREGION_WRITE_WIDTH);
    (void)printf("\r\n");
}

/**
 * @ingroup  menu.c
 * @brief    Gets the PSEL field of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - PSEL field
 */
static uint32_t FlashRegionPartitionSelectGet(uint8_t regionNumber)
{
    uint32_t partitionSelect = 0U;

    switch(regionNumber)
    {
        case 0U:
            partitionSelect = PR0CTRLbits.PSEL;
            break;
        case 1U:
            partitionSelect = PR1CTRLbits.PSEL;
            break;
        case 2U:
            partitionSelect = PR2CTRLbits.PSEL;
            break;
        case 3U:
            partitionSelect = PR3CTRLbits.PSEL;
            break;
        case 4U:
            partitionSelect = PR4CTRLbits.PSEL;
            break;
        case 5U:
            partitionSelect = PR5CTRLbits.PSEL;
            break;
        case 6U:
            partitionSelect = PR6CTRLbits.PSEL;
            break;
        case 7U:
            partitionSelect = PR7CTRLbits.PSEL;
            break;
        default:
            partitionSelect = 0U;
            break;
    }

    return partitionSelect;
}

/**
 * @ingroup  menu.c
 * @brief    Gets the WR field of the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   uint32_t - WR field
 */
static uint32_t FlashRegionWriteEnableGet(uint8_t regionNumber)
{
    uint32_t writeEnable = 0U;

    switch(regionNumber)
    {
        case 0U:
            writeEnable = PR0CTRLbits.WR;
            break;
        case 1U:
            writeEnable = PR1CTRLbits.WR;
            break;
        case 2U:
            writeEnable = PR2CTRLbits.WR;
            break;
        case 3U:
            writeEnable = PR3CTRLbits.WR;
            break;
        case 4U:
            writeEnable = PR4CTRLbits.WR;
            break;
        case 5U:
            writeEnable = PR5CTRLbits.WR;
            break;
        case 6U:
            writeEnable = PR6CTRLbits.WR;
            break;
        case 7U:
            writeEnable = PR7CTRLbits.WR;
            break;
        default:
            writeEnable = 0U;
            break;
    }

    return writeEnable;
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
    uint32_t regionType = 0U;

    switch(regionNumber)
    {
        case 0U:
            regionType = PR0CTRLbits.RTYPE;
            break;
        case 1U:
            regionType = PR1CTRLbits.RTYPE;
            break;
        case 2U:
            regionType = PR2CTRLbits.RTYPE;
            break;
        case 3U:
            regionType = PR3CTRLbits.RTYPE;
            break;
        case 4U:
            regionType = PR4CTRLbits.RTYPE;
            break;
        case 5U:
            regionType = PR5CTRLbits.RTYPE;
            break;
        case 6U:
            regionType = PR6CTRLbits.RTYPE;
            break;
        case 7U:
            regionType = PR7CTRLbits.RTYPE;
            break;
        default:
            regionType = 0U;
            break;
    }

    return regionType;
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
    uint32_t startField = 0U;

    switch(regionNumber)
    {
        case 0U:
            startField = PR0STbits.START;
            break;
        case 1U:
            startField = PR1STbits.START;
            break;
        case 2U:
            startField = PR2STbits.START;
            break;
        case 3U:
            startField = PR3STbits.START;
            break;
        case 4U:
            startField = PR4STbits.START;
            break;
        case 5U:
            startField = PR5STbits.START;
            break;
        case 6U:
            startField = PR6STbits.START;
            break;
        case 7U:
            startField = PR7STbits.START;
            break;
        default:
            startField = 0U;
            break;
    }

    return startField;
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
    uint32_t endField = 0U;

    switch(regionNumber)
    {
        case 0U:
            endField = PR0ENDbits.END;
            break;
        case 1U:
            endField = PR1ENDbits.END;
            break;
        case 2U:
            endField = PR2ENDbits.END;
            break;
        case 3U:
            endField = PR3ENDbits.END;
            break;
        case 4U:
            endField = PR4ENDbits.END;
            break;
        case 5U:
            endField = PR5ENDbits.END;
            break;
        case 6U:
            endField = PR6ENDbits.END;
            break;
        case 7U:
            endField = PR7ENDbits.END;
            break;
        default:
            endField = 0U;
            break;
    }

    return endField;
}

/**
 * @ingroup  menu.c
 * @brief    Returns the partition assignment for the specified flash region.
 *
 * @param    regionNumber - flash region number
 * @return   enum PARTITION - partition assignment
 */
static enum PARTITION FlashRegionPartitionGet(uint8_t regionNumber)
{
    enum PARTITION partition = PARTITION_BOTH;
    uint32_t partitionSelect = FlashRegionPartitionSelectGet(regionNumber);

    switch(partitionSelect)
    {
        case 0x1U:
            partition = PARTITION_1;
            break;
        case 0x2U:
            partition = PARTITION_2;
            break;
        case 0x3U:
            partition = PARTITION_BOTH;
            break;
        default:
            partition = PARTITION_BOTH;
            break;
    }

    return partition;
}

/**
 * @ingroup  menu.c
 * @brief    Returns true if write/erase is enabled for the specified region.
 *
 * @param    regionNumber - flash region number
 * @return   bool - true if write enabled
 */
static bool FlashRegionIsWriteEnabled(uint8_t regionNumber)
{
    bool isWriteEnabled = false;

    if(FlashRegionWriteEnableGet(regionNumber) != 0U)
    {
        isWriteEnabled = true;
    }

    return isWriteEnabled;
}

/**
 * @ingroup  menu.c
 * @brief    Returns the flash region type string.
 *
 * @param    regionNumber - flash region number
 * @return   const char* - region type string
 */
static const char *FlashRegionTypeStringGet(uint8_t regionNumber)
{
    const char *regionTypeString = "UNKNOWN";
    uint32_t regionType = FlashRegionTypeGet(regionNumber);

    switch(regionType)
    {
        case 0x1U:
            regionTypeString = "IRT";
            break;
        case 0x2U:
            regionTypeString = "OTP";
            break;
        case 0x3U:
            regionTypeString = "FIRMWARE";
            break;
        default:
            break;
    }

    return regionTypeString;
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
    uint32_t displayAddress = addressField;

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
 * @brief    Formats the region address string based on whether the region
 *           applies to active, inactive, or both partitions.
 *
 * @param    regionNumber - flash region number
 * @param    addressField - START or END field from PRxST/PRxEND
 * @param    buffer - destination string buffer
 * @param    bufferSize - destination buffer size
 * @return   none
 */
static void FlashRegionAddressStringGet(uint8_t regionNumber,
                                        uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize)
{
    enum PARTITION partition = FlashRegionPartitionGet(regionNumber);

    if(partition == PARTITION_BOTH)
    {
        uint32_t activeAddress = FlashRegionAddressBuild(addressField, true);
        uint32_t inactiveAddress = FlashRegionAddressBuild(addressField, false);

        (void)snprintf(buffer,
                       bufferSize,
                       "0x%06lX/0x%06lX",
                       (unsigned long)activeAddress,
                       (unsigned long)inactiveAddress);
    }
    else if(((partition == PARTITION_1) && (PARTITION_ActiveGet() == 1U)) ||
            ((partition == PARTITION_2) && (PARTITION_ActiveGet() == 2U)))
    {
        uint32_t activeAddress = FlashRegionAddressBuild(addressField, true);

        (void)snprintf(buffer,
                       bufferSize,
                       "0x%06lX",
                       (unsigned long)activeAddress);
    }
    else
    {
        uint32_t inactiveAddress = FlashRegionAddressBuild(addressField, false);

        (void)snprintf(buffer,
                       bufferSize,
                       "0x%06lX",
                       (unsigned long)inactiveAddress);
    }
}

/**
 * @ingroup  menu.c
 * @brief    Prints information about all flash regions, including partition,
 *           type, start/end addresses, and write enable status.
 *
 * @param    none
 * @return   none
 */
void FlashRegionInfoPrint(void)
{
    uint8_t i;
    char startAddressString[24];
    char endAddressString[24];

    (void)printf("  Flash Regions\r\n");
    (void)printf("  -----------------------------------------------------------------------------------------------\r\n");
    (void)printf("  %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                 FLASHREGION_NUMBER_WIDTH, "NUMBER",
                 FLASHREGION_PARTITION_WIDTH, "PARTITION",
                 FLASHREGION_TYPE_WIDTH, "REGION TYPE",
                 FLASHREGION_ADDRESS_WIDTH, "START ADDRESS",
                 FLASHREGION_ADDRESS_WIDTH, "END ADDRESS",
                 FLASHREGION_WRITE_WIDTH, "WRITE ENABLED");

    FlashRegionSeparatorPrint();

    for(i = 0U; i < FLASH_REGION_NUMBER_OF_REGIONS; i++)
    {
        FlashRegionAddressStringGet(i,
                                    FlashRegionStartFieldGet(i),
                                    startAddressString,
                                    sizeof(startAddressString));

        FlashRegionAddressStringGet(i,
                                    FlashRegionEndFieldGet(i),
                                    endAddressString,
                                    sizeof(endAddressString));

        (void)printf("  %-*u | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                     FLASHREGION_NUMBER_WIDTH, (unsigned int)i,
                     FLASHREGION_PARTITION_WIDTH, FlashRegionPartitionStringGet(i),
                     FLASHREGION_TYPE_WIDTH, FlashRegionTypeStringGet(i),
                     FLASHREGION_ADDRESS_WIDTH, startAddressString,
                     FLASHREGION_ADDRESS_WIDTH, endAddressString,
                     FLASHREGION_WRITE_WIDTH, FlashRegionIsWriteEnabled(i) ? "true" : "false");
    }
}