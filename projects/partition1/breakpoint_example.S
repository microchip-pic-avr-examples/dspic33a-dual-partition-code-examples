;(c) [2025] Microchip Technology Inc. and its subsidiaries.
;
;    Subject to your compliance with these terms, you may use Microchip 
;    software and any derivatives exclusively with Microchip products. 
;    You are responsible for complying with 3rd party license terms  
;    applicable to your use of 3rd party software (including open source  
;    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
;    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
;    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
;    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
;    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
;    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
;    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
;    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
;    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
;    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
;    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
;    THIS SOFTWARE.

; void BreakpointExample(void)
;
; @brief  - This function is for demoing how breakpoints work when working 
;           with dual partition.  This function does nothing and returns.
; @return - None.
;
; NOTE - This function is located at a specific address and is the same in both
;        partitions.  Breakpoints are address based and not partition aware so
;        putting a breakpoint in the inactive partition will still cause a 
;        break while the active partition is running if it hits the same
;        address.  In this demo we are fixing the address of this function so
;        we can make sure that the breakpoints set don't overlap with any of the
;        other running code in the demo in the opposite partition.
;    
;        This file differs from the other partition in the contents of the
;        function so you can see more clearly which function was run.

    .global _BreakpointExample
    .section *,address(0x80A000),code
    
_BreakpointExample:
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    MOV W0, W0
    RETURN
