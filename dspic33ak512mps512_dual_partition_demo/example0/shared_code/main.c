/*
(c) [2024] Microchip Technology Inc. and its subsidiaries.

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

#include <stdint.h>

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

#include "bsp/board.h"
#include "bsp/task.h"
#include "bsp/led0.h"
#include "bsp/led7.h"

#include "mcc_generated_files/system/system.h"
#include "partition.h"
#include "scan.h"
#include "command.h"

static void BlinkAlive(void);

int main(void)
{        
    SYSTEM_Initialize();
    BOARD_Initialize();

    /* Request LED to blink every 500ms. */
    (void)TASK_Request(BlinkAlive, 500);
    
    while(1)
    {   
        COMMAND_Process();
    }
}

/**
 * @ingroup  main.c
 * @brief    Toggles LED0 when partition 1 is active.  Toggles LED7 when 
 *           partition 2 is active.
 * 
 * @param    none
 * @return   none
 */
static void BlinkAlive(void)
{
    if(PARTITION_ActiveGet() == 1U)
    {
        led0.toggle();
    }
    else
    {
        led7.toggle();
    }
}

