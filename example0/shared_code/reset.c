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

#include <xc.h>

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

#include "reset.h"

#define RESET_DELAY_COUNT (1048575U)

void RESET_DeviceReset(void)
{
    /* Delay for a bit to allow final hardware events to finalize before the
     * reset, like UART shift registers completely clearing. */
    for (uint32_t i = 0U; i < RESET_DELAY_COUNT; i++)
    {
        Nop();
    }
    
    asm("reset");
}

void RESET_PrintResetSources(void)
{
    if(RCONbits.VREG3R == 1U)
    {
        (void)printf("  RESET: Voltage Domain 3 lost regulation\r\n");
    }

    if(RCONbits.VREG2R == 1U)
    {
        (void)printf("  RESET: Voltage Domain 2 lost regulation\r\n");
    }
   
    if(RCONbits.BUCKR == 1U)
    {
        (void)printf("  RESET: Buck regulator lost regulation\r\n");
    }
    
    if(RCONbits.EXTR == 1U)
    {
        (void)printf("  RESET: Master Clear\r\n");
    }
    
    if(RCONbits.SWR == 1U)
    {
        (void)printf("  RESET: Software\r\n");
    }
    
    if(RCONbits.WDTO == 1U)
    {
        (void)printf("  RESET: Watchdog timer\r\n");
    }
    
    if(RCONbits.BOR == 1U)
    {
        (void)printf("  RESET: Brownout\r\n");
    }
    
    if(RCONbits.POR == 1U)
    {
        (void)printf("  RESET: Power on reset\r\n");
    }
}