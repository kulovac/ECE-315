#include "my_spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xspips.h"
#include "xspips_hw.h"
#include <stddef.h>

#define SpiPs_SendByte(BaseAddress, Data)                                      \
    XSpiPs_Out32((BaseAddress) + XSPIPS_TXD_OFFSET, (Data))

#define SpiPs_RecvByte(BaseAddress)                                            \
    (u8) XSpiPs_In32((BaseAddress) + XSPIPS_RXD_OFFSET)

static XSpiPs spiMasterInst;
static XSpiPs spiSlaveInst;

/******************************************************************************
/* General SPI functions */
/******************************************************************************/
static void spiWrite(XSpiPs *inst, const u8 *sendBuffer, int byteCount) {
    int count;
    u32 baseAddr;

    if ((inst == NULL) || (sendBuffer == NULL) || (byteCount <= 0)) {
        return;
    }

    baseAddr = inst->Config.BaseAddress;
    for (count = 0; count < byteCount; count++) {
        SpiPs_SendByte(baseAddr, sendBuffer[count]);
    }
}

static void spiRead(XSpiPs *inst, u8 *recvBuffer, int byteCount) {
    int count;
    u32 baseAddr;
    u32 statusReg;

    if ((inst == NULL) || (recvBuffer == NULL) || (byteCount <= 0)) {
        return;
    }

    baseAddr = inst->Config.BaseAddress;
    do {
        statusReg = XSpiPs_ReadReg(baseAddr, XSPIPS_SR_OFFSET);
    } while (!(statusReg & XSPIPS_IXR_RXNEMPTY_MASK));

    for (count = 0; count < byteCount; count++) {
        recvBuffer[count] = SpiPs_RecvByte(baseAddr);
    }
}

/******************************************************************************
/* SPI Master specific Functions*/
/******************************************************************************/
void spiMasterWrite(const u8 *tx, int byteCount) {
    // TODO 4: write the body for this function
    for (int i = 0; i < byteCount; ++i) {
        while (
            XSpiPs_ReadReg(spiMasterInst.Config.BaseAddress, XSPIPS_SR_OFFSET) &
            XSPIPS_IXR_TXFULL_MASK)
            ;
        SpiPs_SendByte(spiMasterInst.Config.BaseAddress, tx[i]);
    }
}

void spiMasterRead(u8 *rx, int byteCount) {
    // TODO 5: write the body for this function
}

void spiMasterTransfer(const u8 *tx, u8 *rx, int byteCount) {
    // TODO 6: write the body for this function using spiMasterWrite and
    // spiMasterRead
}

/******************************************************************************
/* SPI Sub specific Functions */
/******************************************************************************/
void spiSlaveWrite(const u8 *tx, int byteCount) {
    // TODO 7: write the body for this function
}

void spiSlaveRead(u8 *rx, int byteCount) {
    // TODO 8: write the body for this function
}

void spiSlaveTransfer(const u8 *tx, u8 *rx, int byteCount) {
    spiSlaveWrite(tx, byteCount);
    spiSlaveRead(rx, byteCount);
}

int spiInit(u32 masterDeviceId, u32 slaveDeviceId) {
    int status;
    XSpiPs_Config *masterCfg;
    XSpiPs_Config *slaveCfg;

    masterCfg = XSpiPs_LookupConfig(masterDeviceId);
    if (masterCfg == NULL) {
        return XST_FAILURE;
    }

    status =
        XSpiPs_CfgInitialize(&spiMasterInst, masterCfg, masterCfg->BaseAddress);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    slaveCfg = XSpiPs_LookupConfig(slaveDeviceId);
    if (slaveCfg == NULL) {
        return XST_FAILURE;
    }

    status =
        XSpiPs_CfgInitialize(&spiSlaveInst, slaveCfg, slaveCfg->BaseAddress);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    status = XSpiPs_SetOptions(&spiMasterInst, (XSPIPS_CR_CPHA_MASK) |
                                                   (XSPIPS_MASTER_OPTION) |
                                                   (XSPIPS_CR_CPOL_MASK));
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    status = XSpiPs_SetOptions(&spiSlaveInst,
                               (XSPIPS_CR_CPHA_MASK) | (XSPIPS_CR_CPOL_MASK));
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}
