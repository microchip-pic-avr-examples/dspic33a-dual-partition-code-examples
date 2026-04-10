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
#include <string.h>

#include "mcc_generated_files/flash/flash_types.h"
#include "mcc_generated_files/flash/flash_nonblocking.h"
#include "scan.h"
#include "test_area_demo.h"

#define INACTIVE_PARTITION_BASE_ADDRESS    0xC00000UL

#define ACTIVE_TEST_AREA_ADDRESS           0x810000UL
#define INACTIVE_TEST_AREA_ADDRESS         0xC10000UL

#define WRITE_RANGE_SIZE_BYTES             256U
#define ERASE_RANGE_SIZE_BYTES             4096U
#define PRINT_REGION_SIZE                  256U

#define TEST_AREA_RANGE_COUNT              6U

#define TESTAREA_SEL_WIDTH                 3U
#define TESTAREA_RANGE_WIDTH               19U
#define TESTAREA_PARTITION_WIDTH           8U

struct TEST_AREA_RANGE
{
    uint8_t selection;
    uint32_t startAddress;
    uint32_t endAddress;
    const char *partitionLabel;
};

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static void PrintRegion(uint32_t address, uint16_t byteCount);
static void PrintRepeatedChar(char ch, uint8_t count);
static void TestAreaTableSeparatorPrint(void);
static void TestAreaRangeTablePrint(const char *title,
                                    const struct TEST_AREA_RANGE *ranges,
                                    uint8_t count);
static bool TestAreaSelectionGet(const struct TEST_AREA_RANGE *ranges,
                                 uint8_t count,
                                 const struct TEST_AREA_RANGE **selectedRange);
static void WritePatternToRange(const struct TEST_AREA_RANGE *range);
static void EraseRange(const struct TEST_AREA_RANGE *range);

/******************************************************************************/
/* Private Data                                                               */
/******************************************************************************/
static const struct TEST_AREA_RANGE writeRanges[TEST_AREA_RANGE_COUNT] =
{
    {1U, 0x810000UL, 0x8100FFUL, "Active"},
    {2U, 0x811000UL, 0x8110FFUL, "Active"},
    {3U, 0x812000UL, 0x8120FFUL, "Active"},
    {4U, 0xC10000UL, 0xC100FFUL, "Inactive"},
    {5U, 0xC11000UL, 0xC110FFUL, "Inactive"},
    {6U, 0xC12000UL, 0xC120FFUL, "Inactive"},
};

/**
 * @ingroup  command.c
 * @brief    Prints a block of memory from the specified address.
 *
 * @param    address - start address to print
 * @param    byteCount - number of bytes to print
 * @return   none
 */
static void PrintRegion(uint32_t address, uint16_t byteCount)
{
    uint8_t data[PRINT_REGION_SIZE];
    uint16_t i;
    uint16_t bytesToPrint = byteCount;

    if(byteCount > PRINT_REGION_SIZE)
    {
        bytesToPrint = PRINT_REGION_SIZE;
    }

    /* cppcheck-suppress misra-c2012-11.6
     *
     *  (Rule 11.6) REQUIRED: A cast shall not be performed between
     *  pointer to void and an arithmetic type
     *
     *  Reasoning: This reads flash memory contents at a specified address
     *  for terminal display.
     */
    (void)memcpy(data, (void*)address, bytesToPrint);

    for(i = 0U; i < bytesToPrint; i++)
    {
        if((i % 16U) == 0U)
        {
            (void)printf("\r\n  %08lX: ", (unsigned long)(address + (uint32_t)i));
        }

        (void)printf(" %02X ", data[i]);
    }

    (void)printf("\r\n\r\n");
}

/**
 * @ingroup  command.c
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
 * @ingroup  command.c
 * @brief    Prints a separator row for the test-area selection table.
 *
 * @param    none
 * @return   none
 */
static void TestAreaTableSeparatorPrint(void)
{
    (void)printf("  ");
    PrintRepeatedChar('-', TESTAREA_SEL_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', TESTAREA_RANGE_WIDTH);
    (void)printf(" | ");
    PrintRepeatedChar('-', TESTAREA_PARTITION_WIDTH);
    (void)printf("\r\n");
}

/**
 * @ingroup  command.c
 * @brief    Prints a test-area range selection table.
 *
 * @param    title - table title
 * @param    ranges - range table
 * @param    count - number of entries
 * @return   none
 */
static void TestAreaRangeTablePrint(const char *title,
                                    const struct TEST_AREA_RANGE *ranges,
                                    uint8_t count)
{
    uint8_t i;

    (void)printf("  %s\r\n", title);
    (void)printf("  ----------------------------------------------------\r\n");
    (void)printf("  %-*s | %-*s | %-*s\r\n",
                 TESTAREA_SEL_WIDTH, "SEL",
                 TESTAREA_RANGE_WIDTH, "ADDRESS RANGE",
                 TESTAREA_PARTITION_WIDTH, "PARTITION");

    TestAreaTableSeparatorPrint();

    for(i = 0U; i < count; i++)
    {
        (void)printf("  %-*u | 0x%06lX-0x%06lX | %-*s\r\n",
                     TESTAREA_SEL_WIDTH, (unsigned int)ranges[i].selection,
                     (unsigned long)ranges[i].startAddress,
                     (unsigned long)ranges[i].endAddress,
                     TESTAREA_PARTITION_WIDTH, ranges[i].partitionLabel);
    }

    (void)printf("\r\n");
}

/**
 * @ingroup  command.c
 * @brief    Prompts the user to select one of the test-area ranges.
 *
 * @param    ranges - range table
 * @param    count - number of entries
 * @param    selectedRange - selected output pointer
 * @return   bool - true if valid selection
 */
static bool TestAreaSelectionGet(const struct TEST_AREA_RANGE *ranges,
                                 uint8_t count,
                                 const struct TEST_AREA_RANGE **selectedRange)
{
    bool isValid = false;
    uint8_t i;
    uint8_t selection = 0U;
    char inputChar;

    (void)printf("Select range (1-6): ");

    inputChar = SCAN_Char(true);
    (void)printf("\r\n\r\n");

    selection = (uint8_t)((uint8_t)inputChar - (uint8_t)'0');

    for(i = 0U; i < count; i++)
    {
        if(ranges[i].selection == selection)
        {
            *selectedRange = &ranges[i];
            isValid = true;
            break;
        }
    }

    if(isValid == false)
    {
        (void)printf("Invalid range selection.\r\n\r\n");
    }

    return isValid;
}

/**
 * @ingroup  command.c
 * @brief    Writes a 256-byte test pattern to the selected range.
 *
 * @param    range - selected write range
 * @return   none
 */
static void WritePatternToRange(const struct TEST_AREA_RANGE *range)
{
    flash_data_t data[WRITE_RANGE_SIZE_BYTES / 4U] =
    {
        0x00010203UL, 0x04050607UL, 0x08090A0BUL, 0x0C0D0E0FUL,
        0x10111213UL, 0x14151617UL, 0x18191A1BUL, 0x1C1D1E1FUL,
        0x20212223UL, 0x24252627UL, 0x28292A2BUL, 0x2C2D2E2FUL,
        0x30313233UL, 0x34353637UL, 0x38393A3BUL, 0x3C3D3E3FUL,
        0x40414243UL, 0x44454647UL, 0x48494A4BUL, 0x4C4D4E4FUL,
        0x50515253UL, 0x54555657UL, 0x58595A5BUL, 0x5C5D5E5FUL,
        0x60616263UL, 0x64656667UL, 0x68696A6BUL, 0x6C6D6E6FUL,
        0x70717273UL, 0x74757677UL, 0x78797A7BUL, 0x7C7D7E7FUL,
        0x80818283UL, 0x84858687UL, 0x88898A8BUL, 0x8C8D8E8FUL,
        0x90919293UL, 0x94959697UL, 0x98999A9BUL, 0x9C9D9E9FUL,
        0xA0A1A2A3UL, 0xA4A5A6A7UL, 0xA8A9AAABUL, 0xACADAEAFUL,
        0xB0B1B2B3UL, 0xB4B5B6B7UL, 0xB8B9BABBUL, 0xBCBDBEBFUL,
        0xC0C1C2C3UL, 0xC4C5C6C7UL, 0xC8C9CACBUL, 0xCCCDCECFUL,
        0xD0D1D2D3UL, 0xD4D5D6D7UL, 0xD8D9DADBUL, 0xDCDDDEDFUL,
        0xE0E1E2E3UL, 0xE4E5E6E7UL, 0xE8E9EAEBUL, 0xECEDEEEFUL,
        0xF0F1F2F3UL, 0xF4F5F6F7UL, 0xF8F9FAFBUL, 0xFCFDFEFFUL
    };
    
    bool writeFailed = false;
    enum FLASH_RETURN_STATUS status;

    status = FLASH_PageErase(range->startAddress, FLASH_UNLOCK_KEY);

    if(status != FLASH_NO_ERROR)
    {
        (void)printf("Erase failed before write at 0x%06lX\r\n\r\n",
                     (unsigned long)range->startAddress);
        writeFailed = true;
    }

    if(writeFailed == false)
    {
        uint8_t i;
        for(i = 0U; i < (WRITE_RANGE_SIZE_BYTES / 16U); i++)
        {
            status = FLASH_WordWrite(range->startAddress + ((uint32_t)i * 16UL),
                                     &data[i * 4U],
                                     FLASH_UNLOCK_KEY);

            if(status != FLASH_NO_ERROR)
            {
                (void)printf("Write failed at 0x%06lX\r\n\r\n",
                             (unsigned long)(range->startAddress + ((uint32_t)i * 16UL)));
                writeFailed = true;
                break;
            }
        }
    }

    if(writeFailed == false)
    {
        PrintRegion(range->startAddress, WRITE_RANGE_SIZE_BYTES);
    }
}

/**
 * @ingroup  command.c
 * @brief    Erases the selected 4KB test range.
 *
 * @param    range - selected erase range
 * @return   none
 */
static void EraseRange(const struct TEST_AREA_RANGE *range)
{
    enum FLASH_RETURN_STATUS status =
        FLASH_PageErase(range->startAddress, FLASH_UNLOCK_KEY);

    if(status != FLASH_NO_ERROR)
    {
        (void)printf("Erase failed at 0x%06lX\r\n\r\n",
                     (unsigned long)range->startAddress);
    }
    else
    {
        PrintRegion(range->startAddress, 256U);
    }
}

/**
 * @ingroup  command.c
 * @brief    Displays the write-range table, prompts for selection,
 *           and writes the selected test area.
 *
 * @param    none
 * @return   none
 */
void WriteTestArea(void)
{
    const struct TEST_AREA_RANGE *selectedRange = NULL;

    TestAreaRangeTablePrint("Write Test Area Options",
                            writeRanges,
                            TEST_AREA_RANGE_COUNT);

    if(TestAreaSelectionGet(writeRanges,
                            TEST_AREA_RANGE_COUNT,
                            &selectedRange))
    {
        WritePatternToRange(selectedRange);
    }
}

/**
 * @ingroup  command.c
 * @brief    Displays the erase-range table, prompts for selection,
 *           and erases the selected test area.
 *
 * @param    none
 * @return   none
 */
void EraseTestArea(void)
{
    static const struct TEST_AREA_RANGE eraseRanges[TEST_AREA_RANGE_COUNT] =
    {
        {1U, 0x810000UL, 0x810FFFUL, "Active"},
        {2U, 0x811000UL, 0x811FFFUL, "Active"},
        {3U, 0x812000UL, 0x812FFFUL, "Active"},
        {4U, 0xC10000UL, 0xC10FFFUL, "Inactive"},
        {5U, 0xC11000UL, 0xC11FFFUL, "Inactive"},
        {6U, 0xC12000UL, 0xC12FFFUL, "Inactive"},
    };

    const struct TEST_AREA_RANGE *selectedRange = NULL;

    TestAreaRangeTablePrint("Erase Test Area Options",
                            eraseRanges,
                            TEST_AREA_RANGE_COUNT);

    if(TestAreaSelectionGet(eraseRanges,
                            TEST_AREA_RANGE_COUNT,
                            &selectedRange))
    {
        EraseRange(selectedRange);
    }
}

/**
 * @ingroup  command.c
 * @brief    Displays the print-range table, prompts for selection,
 *           and prints the selected test area.
 *
 * @param    none
 * @return   none
 */
void PrintTestArea(void)
{
    const struct TEST_AREA_RANGE *selectedRange = NULL;

    TestAreaRangeTablePrint("Print Test Area Options",
                            writeRanges,
                            TEST_AREA_RANGE_COUNT);

    if(TestAreaSelectionGet(writeRanges,
                            TEST_AREA_RANGE_COUNT,
                            &selectedRange))
    {
        PrintRegion(selectedRange->startAddress, WRITE_RANGE_SIZE_BYTES);
    }
}

/**
 * @ingroup  command.c
 * @brief    Issues a bulk erase on the inactive partition.
 *
 * @param    none
 * @return   none
 */
void BulkErase(void)
{
    enum FLASH_RETURN_STATUS status =
        FLASH_BulkErase(INACTIVE_PARTITION_BASE_ADDRESS, FLASH_UNLOCK_KEY);

    if(status == FLASH_NO_ERROR)
    {
        (void)printf("Inactive Partition successfully erased.\r\n\r\n");
    }
    else
    {
        (void)printf("Error! Inactive Partition could not be erased.\r\n\r\n");
    }
}