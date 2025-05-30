; void BootSwap(void)
;
; @brief  - Swaps from the current (Active) partition to an inactive partition. 
;           Regardless of a successful swap or not, execution will jump to
;           address specified in the GOTO instruction.
; @return - None.  THIS FUNCTION WILL NOT RETURN ON A SUCCESSFUL SWAP!  Any
;           return from this function indicates a failed swap.

    .global _BootSwap
    
    ; NOTE - It is critical that the BOOTSWP and GOTO instruction exist at 
    ; the exact same address in both partitions.  
    .section *,address(0x801000),code
    
_BootSwap:
    ; Clear the failed swap flag
    BCLR NVMCON,#11 
    
    ; Load the sequence number of the inactive partition into W1
    MOV.L #0xC3FFF0, W0
    MOV.L [W0], W1
    
    ; Attempt to swap and jump to BootEntry
    BOOTSWP W1
    
    ; NOTE - Per the datasheet, the instruction that follows the BOOTSWP must 
    ; be an instruction that loads the PC and must be withing 32KB of the
    ; current instruction.  
    ;
    ; NOTE - The instruction following the BOOTSWP is executed out of the target
    ; partition and not the source partition of the swap.
    GOTO _BootEntry   
    
    ; This instruction should never be run.  A RESET is inserted here in case
    ; there is an error in the execution of the GOTO instruction that doesn't
    ; result in a trap.  We want to force execution to start at _BootEntry
    RESET

_BootEntry:
    ; Test the SFTSWP bit to see if the swap happened
    BTST NVMCON,#11 
    BRA Z,_BootSwapFailed
    
_BootSwapPassed:
    ; Swap passed.  We are in a new partition.  Jump to the entry point of this
    ; partitions code.
    GOTO __reset
        
_BootSwapFailed:
    ; On a failed swap attempt, return to where we tried to swap
    RETURN
