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
#include "partition.h"
#include "scan.h"
#include "sequence_info.h"

#define ACTIVE_SEQUENCE_NUMBER_ADDRESS      0x83FFF0UL
#define ACTIVE_SEQUENCE_NUMBER_PAGE         0x83F000UL
#define INACTIVE_SEQUENCE_NUMBER_ADDRESS    0xC3FFF0UL
#define INACTIVE_SEQUENCE_NUMBER_PAGE       0xC3F000UL

#define SEQINFO_PARTITION_WIDTH             16U
#define SEQINFO_STATE_WIDTH                 15U
#define SEQINFO_SEQNUM_WIDTH                 9U
#define SEQINFO_INVSEQNUM_WIDTH             17U
#define SEQINFO_VALID_WIDTH                  7U
#define SEQINFO_ADDRESS_WIDTH               17U

struct SEQUENCE_INFO
{
    uint32_t address;
    uint16_t sequenceNumber;
    uint16_t inverseSequenceNumber;
    bool valid;
};

/******************************************************************************/
/* Private Function Prototypes                                                */
/******************************************************************************/
static void SequenceNumberUpdate(flash_adr_t page, flash_adr_t address);
static bool SequenceCodeIsValid(const uint32_t *sequenceCode);
static void SequenceInfoGet(uint32_t address, struct SEQUENCE_INFO *info);
static const char* ValidStringGet(bool valid);
static void SequenceInfoRowPrint(uint8_t partitionNumber,
                                 const char *stateLabel,
                                 uint32_t address);
static void PrintRepeatedChar(char ch, uint8_t count);
static void SequenceInfoSeparatorPrint(void);

/**
 * @ingroup  command.c
 * @brief    Update the sequence number of the inactive partition.
 *
 *           NOTE: Invalid values are allowed to be programmed. This allows
 *           testing of invalid sequence numbers for boot swapping and reset.
 *
 * @param    page - flash page containing the sequence number
 * @param    address - flash address of the sequence number
 * @return   none
 */
static void SequenceNumberUpdate(flash_adr_t page, flash_adr_t address)
{
    uint32_t sequenceNumber = 0U;

    (void)printf("\r\nEnter a 24-bit sequence number (IBTSEQn + BTSEQn): ");
    if(SCAN_Hex((uint8_t*)&sequenceNumber, 6U, true))
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
void SequenceNumberActiveUpdate(void)
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
void SequenceNumberInactiveUpdate(void)
{
    SequenceNumberUpdate(INACTIVE_SEQUENCE_NUMBER_PAGE, INACTIVE_SEQUENCE_NUMBER_ADDRESS);
}

/**
 * @ingroup  menu.c
 * @brief    Determines if the specified sequence number is valid or not.
 *
 * @param    sequenceCode - pointer to 128-bit sequence code
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
    (void)memcpy(sequenceCode, (void*)address, sizeof(sequenceCode));

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
    const char* result = valid ? "Valid" : "Invalid";
    return result;
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
static void SequenceInfoRowPrint(uint8_t partitionNumber,
                                 const char *stateLabel,
                                 uint32_t address)
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
 * @brief    Prints sequence number information for active and inactive partitions
 *           in a table format.
 *
 * @param    none
 * @return   none
 */
void SequenceInfoPrint(void)
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