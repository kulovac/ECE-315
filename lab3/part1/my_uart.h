#ifndef SRC_MY_UART_H_
#define SRC_MY_UART_H_

#include "xil_types.h"
#include "xstatus.h"

int uartInit(u32 BaseAddress);
int uartReadByte(u8 *outByte);
void uartWriteByte(u8 byte);
void uartPrintMenu(void);

#endif /* SRC_MY_UART_H_ */
