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
#include <xc.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "partition.h"
#include "reset.h"
#include "menu.h"

#define ACTIVE_SEQUENCE_NUMBER_ADDRESS 0x83FFF0UL
#define ACTIVE_SEQUENCE_NUMBER_PAGE 0x83F000UL

#define INACTIVE_SEQUENCE_NUMBER_ADDRESS 0xC3FFF0UL
#define INACTIVE_SEQUENCE_NUMBER_PAGE 0xC3F000UL

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
    (void)printf("  Active Partition  (%u) : ", PARTITION_ActiveGet());
    SequenceCodePrint(ACTIVE_SEQUENCE_NUMBER_ADDRESS);
    (void)printf("  Inactive Partition(%u) : ", PARTITION_InactiveGet());
    SequenceCodePrint(INACTIVE_SEQUENCE_NUMBER_ADDRESS);
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
    
    (void)printf("  Sequence Number Options:\r\n");
    (void)printf("  -------------------------------------------\r\n");
    (void)printf("    's' : Write the sequence number of the inactive partition\r\n");
    (void)printf("    'a' : Erase active partition sequence number\r\n");
    (void)printf("    'i' : Erase inactive partition sequence number\r\n");
    (void)printf("\r\n");

    (void)printf("  Flash Panel Lock & Unlock Options:\r\n");
    (void)printf("  -------------------------------------------\r\n");
    (void)printf("    'u' : Set a flash protection region to UNLOCKED\r\n");
    (void)printf("    'l' : Set a flash protection region to LOCKED\r\n");
    (void)printf("    'x' : Set a flash protection region to LOCKED UNTIL RESET\r\n");
    (void)printf("\r\n");

    (void)printf("  Copy & Erase Options:\r\n");
    (void)printf("  -------------------------------------------\r\n");
    (void)printf("    'c' : Copy the current partition code to the inactive partition\r\n");
    (void)printf("    'e' : Erase a PAGE in a flash protection region\r\n");
    (void)printf("    'p' : Erase the inactive PARTITION\r\n");
    (void)printf("\r\n");

    (void)printf("  Bootswap, Debug, & Reset Options:\r\n");
    (void)printf("  -------------------------------------------\r\n");
    (void)printf("    'b' : BOOTSWP\r\n");
    (void)printf("    'q' : run the breakpoint function\r\n");
    (void)printf("    'r' : Reset\r\n");
    (void)printf("\r\n");
    
    (void)printf("Command: ");
}