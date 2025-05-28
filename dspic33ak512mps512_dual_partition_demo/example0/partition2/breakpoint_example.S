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
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    RETURN
