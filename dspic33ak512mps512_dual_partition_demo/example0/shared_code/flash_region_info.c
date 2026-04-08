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

#include "partition.h"
#include "flash_region_info.h"
#include "flash_regions/flash_region.h"
#include "flash_regions/flash_region_0.h"
#include "flash_regions/flash_region_1.h"
#include "flash_regions/flash_region_2.h"
#include "flash_regions/flash_region_3.h"
#include "flash_regions/flash_region_4.h"
#include "flash_regions/flash_region_5.h"
#include "flash_regions/flash_region_6.h"
#include "flash_regions/flash_region_7.h"

#define FLASH_REGION_NUMBER_OF_REGIONS 8U

#define FLASHREGION_NUMBER_WIDTH 6U
#define FLASHREGION_PARTITION_WIDTH 12U
#define FLASHREGION_TYPE_WIDTH 11U
#define FLASHREGION_ADDRESS_WIDTH 19U
#define FLASHREGION_LOCK_STATUS_WIDTH 11U
#define FLASHREGION_WRITE_WIDTH 13U

#define FLASH_REGION_LOCK_MASK 0x00000003UL
#define FLASH_REGION_LOCKED 0x00000000UL
#define FLASH_REGION_LOCKED_UNTIL_RESET 0x00000001UL
#define FLASH_REGION_UNLOCKED 0x00000003UL

#define FLASH_ACTIVE_SPACE_BASE 0x800000UL
#define FLASH_INACTIVE_SPACE_BASE 0xC00000UL

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static const char *FlashRegionPartitionStringGet(struct FLASH_REGION *const region);
static void PrintRepeatedChar(char ch, uint8_t count);
static void FlashRegionSeparatorPrint(void);
static uint32_t FlashRegionTypeGet(uint8_t regionNumber);
static uint32_t FlashRegionStartFieldGet(uint8_t regionNumber);
static uint32_t FlashRegionEndFieldGet(uint8_t regionNumber);
static uint32_t FlashRegionLockGet(uint8_t regionNumber);
static const char *FlashRegionTypeStringGet(uint8_t regionNumber);
static const char *FlashRegionLockStatusStringGet(uint8_t regionNumber);
static uint32_t FlashRegionAddressBuild(uint32_t addressField, bool activeSpace);
static void FlashRegionAddressStringGet(struct FLASH_REGION *const region,
                                        uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize);

/******************************************************************************/
/* Private Data                                                               */
/******************************************************************************/
static struct FLASH_REGION *const flashRegion[] =
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

/*
 * @ingroup  menu.c
 * @brief    Returns the flash region partition string based on the region's
 *           partition and the active partition.
 *
 * @param    region - pointer to the flash region struct
 * @return   const char* - flash region partition string
 */
static const char *FlashRegionPartitionStringGet(struct FLASH_REGION *const region)
{
    enum PARTITION partition = region->partitionGet();

    if (partition == PARTITION_BOTH)
    {
        return "BOTH";
    }
    else if (((partition == PARTITION_1) && (PARTITION_ActiveGet() == 1U)) ||
             ((partition == PARTITION_2) && (PARTITION_ActiveGet() == 2U)))
    {
        if (partition == PARTITION_1)
        {
            return "1 (ACTIVE)";
        }
        else
        {
            return "2 (ACTIVE)";
        }
    }
    else
    {
        if (partition == PARTITION_1)
        {
            return "1 (INACTIVE)";
        }
        else
        {
            return "2 (INACTIVE)";
        }
    }
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

    for (i = 0U; i < count; i++)
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
    switch (regionNumber)
    {
    case 0U:
        return PR0CTRLbits.RTYPE;
    case 1U:
        return PR1CTRLbits.RTYPE;
    case 2U:
        return PR2CTRLbits.RTYPE;
    case 3U:
        return PR3CTRLbits.RTYPE;
    case 4U:
        return PR4CTRLbits.RTYPE;
    case 5U:
        return PR5CTRLbits.RTYPE;
    case 6U:
        return PR6CTRLbits.RTYPE;
    case 7U:
        return PR7CTRLbits.RTYPE;
    default:
        return 0U;
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
    switch (regionNumber)
    {
    case 0U:
        return PR0STbits.START;
    case 1U:
        return PR1STbits.START;
    case 2U:
        return PR2STbits.START;
    case 3U:
        return PR3STbits.START;
    case 4U:
        return PR4STbits.START;
    case 5U:
        return PR5STbits.START;
    case 6U:
        return PR6STbits.START;
    case 7U:
        return PR7STbits.START;
    default:
        return 0U;
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
    switch (regionNumber)
    {
    case 0U:
        return PR0ENDbits.END;
    case 1U:
        return PR1ENDbits.END;
    case 2U:
        return PR2ENDbits.END;
    case 3U:
        return PR3ENDbits.END;
    case 4U:
        return PR4ENDbits.END;
    case 5U:
        return PR5ENDbits.END;
    case 6U:
        return PR6ENDbits.END;
    case 7U:
        return PR7ENDbits.END;
    default:
        return 0U;
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
    switch (regionNumber)
    {
    case 0U:
        return PR0LOCK;
    case 1U:
        return PR1LOCK;
    case 2U:
        return PR2LOCK;
    case 3U:
        return PR3LOCK;
    case 4U:
        return PR4LOCK;
    case 5U:
        return PR5LOCK;
    case 6U:
        return PR6LOCK;
    case 7U:
        return PR7LOCK;
    default:
        return 0U;
    }
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
    uint32_t regionType = FlashRegionTypeGet(regionNumber);

    switch (regionType)
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
static const char *FlashRegionLockStatusStringGet(uint8_t regionNumber)
{
    uint32_t lockValue = FlashRegionLockGet(regionNumber) & FLASH_REGION_LOCK_MASK;

    switch (lockValue)
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

    displayAddress = addressField;

    if (activeSpace)
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
static void FlashRegionAddressStringGet(struct FLASH_REGION *const region,
                                        uint32_t addressField,
                                        char *buffer,
                                        size_t bufferSize)
{
    enum PARTITION partition = region->partitionGet();

    if (partition == PARTITION_BOTH)
    {
        uint32_t activeAddress = FlashRegionAddressBuild(addressField, true);
        uint32_t inactiveAddress = FlashRegionAddressBuild(addressField, false);

        (void)snprintf(buffer,
                       bufferSize,
                       "0x%06lX/0x%06lX",
                       (unsigned long)activeAddress,
                       (unsigned long)inactiveAddress);
    }
    else if (((partition == PARTITION_1) && (PARTITION_ActiveGet() == 1U)) ||
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
 *           partition, type, start/end addresses, lock status, and write
 *           enable status.
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
    (void)printf("  -------------------------------------------------------------------------------------------------------------\r\n");
    (void)printf("  %-*s | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                 FLASHREGION_NUMBER_WIDTH, "NUMBER",
                 FLASHREGION_PARTITION_WIDTH, "PARTITION",
                 FLASHREGION_TYPE_WIDTH, "REGION TYPE",
                 FLASHREGION_ADDRESS_WIDTH, "START ADDRESS",
                 FLASHREGION_ADDRESS_WIDTH, "END ADDRESS",
                 FLASHREGION_LOCK_STATUS_WIDTH, "LOCK STATUS",
                 FLASHREGION_WRITE_WIDTH, "WRITE ENABLED");

    FlashRegionSeparatorPrint();

    for (i = 0U; i < FLASH_REGION_NUMBER_OF_REGIONS; i++)
    {
        FlashRegionAddressStringGet(flashRegion[i],
                                    FlashRegionStartFieldGet(i),
                                    startAddressString,
                                    sizeof(startAddressString));

        FlashRegionAddressStringGet(flashRegion[i],
                                    FlashRegionEndFieldGet(i),
                                    endAddressString,
                                    sizeof(endAddressString));

        (void)printf("  %-*u | %-*s | %-*s | %-*s | %-*s | %-*s | %-*s\r\n",
                     FLASHREGION_NUMBER_WIDTH, (unsigned int)i,
                     FLASHREGION_PARTITION_WIDTH, FlashRegionPartitionStringGet(flashRegion[i]),
                     FLASHREGION_TYPE_WIDTH, FlashRegionTypeStringGet(i),
                     FLASHREGION_ADDRESS_WIDTH, startAddressString,
                     FLASHREGION_ADDRESS_WIDTH, endAddressString,
                     FLASHREGION_LOCK_STATUS_WIDTH, FlashRegionLockStatusStringGet(i),
                     FLASHREGION_WRITE_WIDTH, flashRegion[i]->isWriteEnabled() ? "true" : "false");
    }
}