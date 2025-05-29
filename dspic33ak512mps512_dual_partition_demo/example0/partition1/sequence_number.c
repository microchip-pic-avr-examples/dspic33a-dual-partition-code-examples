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

/* Section 6.4.2.2.1 - Configure Active/Inactive Partitions of the datasheet
 * states:
 * 
 * When configured for a dual boot mode after a reset, the NVM Controller reads 
 * the boot sequence numbers contained in the BTSEQn words, which are the last 
 * 128-bit words of each user program partition.
 * 
 * ...
 * 
 * The Boot Sequence Number is stored in two parts: the actual value in the bit 
 * field, BTSEQn[11:0], and the one's complement of the value in the IBTSEQn 
 * bits field [23:12].
 * 
 * ...
 * 
 * BTSEQ [127:24] is reserved and must read as all 0's.
 * 
 * In the example below, we are using a VALID sequence number of 5.  
 * - The lowest 12-bits of the BTSEQ are set to the actual sequence number (0x005)
 * - The next 12-bits of the BTSEQ are set to the inverse of the sequence number
 *   (0xFFA in this example)
 * - The remaining bits of the 128-bit field are all 0s
 */
volatile static const uint32_t sequenceNumber[4] __attribute__((space(prog), address(0x83FFF0), keep)) = { 0x00FFB004, 0x00000000UL, 0x00000000UL, 0x00000000UL };
