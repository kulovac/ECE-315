#include "my_uart.h"
#include <stddef.h>
#include "xuartps.h"
#include "xil_printf.h"

static XUartPs uartInst;
static XUartPs_Config *uartCfg;

int uartInit(u32 BaseAddress)
{
	int status;

	uartCfg = XUartPs_LookupConfig(BaseAddress);
	if (uartCfg == NULL) {
		return XST_FAILURE;
	}

	status = XUartPs_CfgInitialize(&uartInst, uartCfg, uartCfg->BaseAddress);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XUartPs_SetOperMode(&uartInst, XUARTPS_OPER_MODE_NORMAL);
	return XST_SUCCESS;
}

int uartReadByte(u8 *outByte)
{
	if ((outByte == NULL) || (uartCfg == NULL)) {
		return 0;
	}

	if (XUartPs_IsReceiveData(uartCfg->BaseAddress)) {
		*outByte = XUartPs_ReadReg(uartCfg->BaseAddress, XUARTPS_FIFO_OFFSET);
		return 1;
	}

	return 0;
}

void uartWriteByte(u8 byte)
{
	while (XUartPs_IsTransmitFull(uartCfg->BaseAddress));
	XUartPs_WriteReg(uartCfg->BaseAddress, XUARTPS_FIFO_OFFSET, byte);
}
