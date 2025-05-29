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

#include "mcc_generated_files/uart/uart1.h"
#include "scan.h"

static bool IsEven(unsigned int value);
static bool NibbleGet(uint8_t character, uint8_t* nibble);
static bool IsAlphaNumeric(char input);

/**
 * @ingroup  scan.c
 * @brief    prints a character to screen if it is alpha-numeric
 * 
 * @param    input - character to print
 * @return   none
 */
static void Echo(char input)
{
    if(IsAlphaNumeric(input))
    {
        (void)printf("%c", input);
    }
}

/**
 * @ingroup  scan.c
 * @brief    Scans a character from the terminal.
 * 
 * @param    echo - true if the scanned character should be echoed.
 * @return   char - the scanned character
 */
char SCAN_Char(bool echo)
{
    char input;
    
    while(UART1_IsRxReady() == false)
    {
    }
    
    input = (char)UART1_Read();
    
    if(echo == true)
    {
        Echo(input);
    }
    
    return input;
}

/**
 * @ingroup  scan.c
 * @brief    Scans a variable length hex string from the user input.  Exits
 *           early if the ESC key is pressed.
 * 
 * @param[out] buffer - the buffer where to write the scanned data
 * @param[in] count - number of hex bytes to read
 * @param[in] echo - if the input should be echoed as it is typed
 * @return   bool - if the full amount of requested characters were scanned.
 */
bool SCAN_Hex(uint8_t *buffer, const unsigned int count, bool echo)
{
    unsigned int characters = 0;
    uint8_t current = 0;   
    uint8_t *tail = &buffer[(count-1U)/2U];
    bool scanComplete = true;
    
    while(characters != count)  
    {
        uint8_t nibble;
        char input = SCAN_Char(false);
        
        if((unsigned int)input == SCAN_ESCAPE_CHARACTER_CODE)
        {
            scanComplete = false;
        }
        else 
        {
            if(NibbleGet(input, &nibble))
            {
                if(echo == true)
                {
                    Echo(input);
                }

                current <<= 4;
                current |= nibble;

                *tail = current;

                characters++;

                if(IsEven(characters))
                {
                    tail--;
                    current = 0;
                }
            }
        }
    } //while()
    
    return scanComplete;
}

/**
 * @ingroup  scan.c
 * @brief    determines if the input character is alpha-numeric
 * 
 * @param[in] input - character to test
 * @return bool - true if alpha-numeric
 */
static bool IsAlphaNumeric(char input)
{
    bool isNumeric = ('0' <= input) && (input <= '9');
    bool isAlpha = (('a' <= input) && (input <= 'z')) || (('A' <= input) && (input <= 'Z'));
    
    return isNumeric || isAlpha;
}

static bool IsEven(unsigned int value)
{
    return (value % 2U) == 0U;
}

/**
 * @ingroup  scan.c
 * @brief    Converts an ASCII character into a hex nibble
 * @param[in] character - ASCII character to convert
 * @param[out] nibble - pointer to where to store the resulting nibble
 * @return bool - true if the provided character was a valid hex nibble
 */
static bool NibbleGet(uint8_t character, uint8_t* nibble)
{
    bool result = false;
    
    switch(character)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            *nibble = (uint8_t)(character - (uint8_t)'0');
            result = true;
            break;
            
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            *nibble = (uint8_t)(character - (uint8_t)'a' + (uint8_t)0xA);
            result = true;
            break;
            
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            *nibble = (uint8_t)(character - (uint8_t)'A' + (uint8_t)0xA);
            result = true;
            break;
            
        default:
            break;
    }
    
    return result;
}
