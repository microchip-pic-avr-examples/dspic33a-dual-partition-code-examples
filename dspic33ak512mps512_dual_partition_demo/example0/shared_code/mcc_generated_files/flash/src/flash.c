/**
 * FLASH Generated Driver Source File
 *
 * @file      flash.c
 *
 * @ingroup   flashdriver
 *
 * @brief     This is the generated driver source file for FLASH driver
 *
 * @skipline @version   Firmware Driver Version 1.0.0
 *
 * @skipline @version   PLIB Version 1.0.2
 *
 * @skipline  Device : dsPIC33AK512MPS512
*/

/*disclaimer*/

#include <stddef.h>
#include <xc.h>
#include "../flash.h"
#include "../flash_nonblocking.h"

enum FLASH_PROGRAM_ERASE_ERROR_CODE getProgramEraseErrorCode(void);

enum FLASH_PROGRAM_ERASE_ERROR_CODE {
    ERROR_INVALID_OP = 1,
    ERROR_SECURITY_ACCESS_VIOLATION,
    ERROR_FLASH_PANEL_CONTROL_LOGIC,
    ERROR_ROW_OP_SYSTEM_BUS,
    ERROR_ROW_OP_WARM_RESET
};

static bool FLASH_IsBusy(void);

enum FLASH_RETURN_STATUS FLASH_PageErase(flash_adr_t flashAddress, flash_key_t unlockKey)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_NO_ERROR;

    if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
    {
        flashReturnStatus = FLASH_INVALID_KEY;
    }
    else if (0U != (flashAddress & ~FLASH_ERASE_PAGE_MASK))
    {
        flashReturnStatus = FLASH_INVALID_ADDRESS;
    }
    else
    {
        NVMADR = flashAddress;
        NVMCON = FLASH_PAGE_ERASE_OPCODE;
        NVMCONbits.WR = 1U;
        while(1U == NVMCONbits.WR){};

        flashReturnStatus = getProgramEraseErrorCode();
    }
    return flashReturnStatus;
}

enum FLASH_RETURN_STATUS FLASH_WordWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_NO_ERROR;
    if (NULL == data)
    {
        flashReturnStatus = FLASH_INVALID_DATA;
    }
    else if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
    {
        flashReturnStatus = FLASH_INVALID_KEY;
    }
    else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
    {
        flashReturnStatus = FLASH_INVALID_ADDRESS;
    }
    else
    {
	    NVMDATA0 = *data;
        NVMDATA1 = *(data + 1U);
        NVMDATA2 = *(data + 2U);
        NVMDATA3 = *(data + 3U);

        NVMADR = flashAddress;
        NVMCON = FLASH_WORD_WRITE_OPCODE;

        NVMCONbits.WR = 1U;
        while (1U == NVMCONbits.WR){};

        flashReturnStatus = getProgramEraseErrorCode();
    }
    return flashReturnStatus;
}

enum FLASH_RETURN_STATUS  FLASH_RowWrite(flash_adr_t flashAddress, flash_data_t *data, flash_key_t unlockKey)
{
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_NO_ERROR;
    if (NULL == data)
    {
        flashReturnStatus = FLASH_INVALID_DATA;
    }
    else if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
    {
        flashReturnStatus = FLASH_INVALID_KEY;
    }
    else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
    {
        flashReturnStatus = FLASH_INVALID_ADDRESS;
    }
    else
    {
        NVMSRCADR = (uint32_t)data;
        NVMADR = flashAddress;
        NVMCON = FLASH_ROW_WRITE_OPCODE;

        NVMCONbits.WR = 1U;
        while (1U == NVMCONbits.WR){};

        flashReturnStatus = getProgramEraseErrorCode();
    }
    return flashReturnStatus;
}

enum FLASH_RETURN_STATUS FLASH_Read(flash_adr_t flashAddress, size_t count, flash_data_t *data)
{
    flash_adr_t *flashReadAddress = (flash_adr_t *)flashAddress;
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_NO_ERROR;

    if ((NULL == data) || (0U == count))
    {
        flashReturnStatus = FLASH_INVALID_DATA;
    }
    else if (0U != (flashAddress % FLASH_ADDRESS_MASK))
    {
        flashReturnStatus = FLASH_INVALID_ADDRESS;
    }
    else
    {
        uint32_t index;
        for(index = 0; index < count ; index++)
        {
            *data = *(flashReadAddress+index);
            data++;
        }
    }
    return flashReturnStatus;
}

uint16_t FLASH_ErasePageOffsetGet(flash_adr_t address)
{
    return ((~FLASH_ERASE_PAGE_MASK) & address);
}

uint32_t FLASH_ErasePageAddressGet(flash_adr_t address)
{
    return (FLASH_ERASE_PAGE_MASK & address);
}

enum FLASH_PROGRAM_ERASE_ERROR_CODE getProgramEraseErrorCode(void)
{
    enum FLASH_RETURN_STATUS flashErrorCode = FLASH_NO_ERROR;
    if(1U == NVMCONbits.WRERR)
    {
       flashErrorCode = FLASH_WRITE_ERROR;
    }
    else if(ERROR_INVALID_OP == NVMCONbits.WREC)
    {
        flashErrorCode = FLASH_INVALID_OEPRATION;
    }
    else if(ERROR_SECURITY_ACCESS_VIOLATION == NVMCONbits.WREC)
    {
        flashErrorCode = FLASH_SECURITY_ACCESS_CONTROL_ERROR;
    }
    else if(ERROR_FLASH_PANEL_CONTROL_LOGIC == NVMCONbits.WREC)
    {
        flashErrorCode = FLASH_PANEL_CONTROL_LOGIC_ERROR;
    }
    else if(ERROR_ROW_OP_SYSTEM_BUS == NVMCONbits.WREC)
    {
        flashErrorCode = FLASH_ROW_OP_SYSTEM_BUS_ERROR;
    }
    else if(ERROR_ROW_OP_WARM_RESET == NVMCONbits.WREC)
    {
        flashErrorCode = FLASH_ROW_OP_WARM_RESET_ERROR;
    }
    else
    {
        flashErrorCode = FLASH_NO_ERROR;
    }
    return flashErrorCode;
}

enum FLASH_RETURN_STATUS FLASH_BulkErase(flash_adr_t flashAddress, flash_key_t unlockKey)
{    
    enum FLASH_RETURN_STATUS flashReturnStatus = FLASH_OP_IN_PROGRESS;

    if(unlockKey != (flash_key_t)FLASH_UNLOCK_KEY)
    {
        flashReturnStatus = FLASH_INVALID_KEY;
    }
    if (0U != (flashAddress & ~FLASH_ERASE_PAGE_MASK))
    {
        flashReturnStatus = FLASH_INVALID_ADDRESS;
    }
    else if(0U != NVMCONbits.WR)
    {
        flashReturnStatus = FLASH_OP_BUSY;
    }
    else
    { 
        NVMADR = flashAddress;
        NVMCON = FLASH_INACTIVE_PANEL_ERASE_OPCODE;
        NVMCONbits.WR = 1U;
        while(FLASH_IsBusy()){
        }

        flashReturnStatus = getProgramEraseErrorCode();
    }
    
    return flashReturnStatus;
}

static bool FLASH_IsBusy(void)
{
    enum FLASH_RETURN_STATUS status = FLASH_OperationStatusGet();
    
    return ((status == FLASH_OP_BUSY) || (status == FLASH_OP_IN_PROGRESS));
}
