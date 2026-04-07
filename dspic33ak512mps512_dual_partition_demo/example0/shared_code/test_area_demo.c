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
#include "test_area_demo.h"

#define ACTIVE_PARTITION_BASE_ADDRESS      0x800000UL
#define INACTIVE_PARTITION_BASE_ADDRESS    0xC00000UL

#define ACTIVE_TEST_AREA_ADDRESS           0x810000UL
#define INACTIVE_TEST_AREA_ADDRESS         0xC10000UL

#define PRINT_REGION_SIZE                  128U

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static void PrintRegion(uint32_t address);

/**
 * @ingroup  command.c
 * @brief    Prints a block of memory from the specified address.
 *
 * @param    address - start address to print
 * @return   none
 */
static void PrintRegion(uint32_t address)
{
    uint8_t data[PRINT_REGION_SIZE];
    int i;

    /* cppcheck-suppress misra-c2012-11.6
     *
     *  (Rule 11.6) REQUIRED: A cast shall not be performed between
     *  pointer to void and an arithmetic type
     *
     *  Reasoning: This reads the flash memory contents at a specified
     *  address for terminal display.
     */
    memcpy(data, (void*)address, sizeof(data));

    for(i = 0; i < (int)sizeof(data); i++)
    {
        if((i % 16) == 0)
        {
            (void)printf("\r\n  %08lX: ", (unsigned long)(address + (uint32_t)i));
        }

        (void)printf(" %02lX ", (unsigned long)data[i]);
    }

    (void)printf("\r\n\r\n");
}

/**
 * @ingroup  command.c
 * @brief    Prints the active partition test area.
 *
 * @param    none
 * @return   none
 */
void PrintActiveTestArea(void)
{
    PrintRegion(ACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Prints the inactive partition test area.
 *
 * @param    none
 * @return   none
 */
void PrintInactiveTestArea(void)
{
    PrintRegion(INACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Writes test data to the active partition test area.
 *
 * @param    none
 * @return   none
 */
void WriteActiveTestArea(void)
{
    flash_data_t data[PRINT_REGION_SIZE / 4U] =
    {
        0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F,
        0x10111213, 0x14151617, 0x18191A1B, 0x1C1D1E1F,
        0x20212223, 0x24252627, 0x28292A2B, 0x2C2D2E2F,
        0x30313233, 0x34353637, 0x38393A3B, 0x3C3D3E3F,
        0x40414243, 0x44454647, 0x48494A4B, 0x4C4D4E4F,
        0x50515253, 0x54555657, 0x58595A5B, 0x5C5D5E5F,
        0x60616263, 0x64656667, 0x68696A6B, 0x6C6D6E6F,
        0x70717273, 0x74757677, 0x78797A7B, 0x7C7D7E7F,
    };

    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x00UL, &data[0], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x10UL, &data[4], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x20UL, &data[8], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x30UL, &data[12], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x40UL, &data[16], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x50UL, &data[20], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x60UL, &data[24], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(ACTIVE_TEST_AREA_ADDRESS + 0x70UL, &data[28], FLASH_UNLOCK_KEY);

    PrintRegion(ACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Writes test data to the inactive partition test area.
 *
 * @param    none
 * @return   none
 */
void WriteInactiveTestArea(void)
{
    flash_data_t data[PRINT_REGION_SIZE / 4U] =
    {
        0xFCFDFEFF, 0xF8F9FAFB, 0xF4F5F6F7, 0xF0F1F2F3,
        0xECEDEEEF, 0xE8E9EAEB, 0xE4E5E6E7, 0xE0E1E2E3,
        0xDCDDDEDF, 0xD8D9DADB, 0xD4D5D6D7, 0xD0D1D2D3,
        0xCCCDCECF, 0xC8C9CACB, 0xC4C5C6C7, 0xC0C1C2C3,
        0xBDBDBEBF, 0xB8B9BABB, 0xB4B5B6B7, 0xB0B1B2B3,
        0xADADAEAF, 0xA8A9AAAB, 0xA4A5A6A7, 0xA0A1A2A3,
        0x9D9D9E9F, 0x98999A9B, 0x94959697, 0x90919293,
        0x8C8D8E8F, 0x88898A8B, 0x84858687, 0x80818283,
    };

    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x00UL, &data[0], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x10UL, &data[4], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x20UL, &data[8], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x30UL, &data[12], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x40UL, &data[16], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x50UL, &data[20], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x60UL, &data[24], FLASH_UNLOCK_KEY);
    (void)FLASH_WordWrite(INACTIVE_TEST_AREA_ADDRESS + 0x70UL, &data[28], FLASH_UNLOCK_KEY);

    PrintRegion(INACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Erases the test area in the active partition.
 *
 * @param    none
 * @return   none
 */
void EraseActiveTestArea(void)
{
    (void)FLASH_PageErase(ACTIVE_TEST_AREA_ADDRESS, FLASH_UNLOCK_KEY);
    PrintRegion(ACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Erases the test area in the inactive partition.
 *
 * @param    none
 * @return   none
 */
void EraseInactiveTestArea(void)
{
    (void)FLASH_PageErase(INACTIVE_TEST_AREA_ADDRESS, FLASH_UNLOCK_KEY);
    PrintRegion(INACTIVE_TEST_AREA_ADDRESS);
}

/**
 * @ingroup  command.c
 * @brief    Issues a bulk/panel/partition erase on the inactive partition.
 *
 * @param    none
 * @return   none
 */
void BulkErase(void)
{
    if(FLASH_BulkErase(INACTIVE_PARTITION_BASE_ADDRESS, FLASH_UNLOCK_KEY) == FLASH_NO_ERROR)
    {
        (void)printf("Inactive Partition successfully erased.\r\n\r\n");
    }
    else
    {
        (void)printf("Error! Inactive Partition could not be erased.\r\n\r\n");
    }
}