/*
 * spi_section.h
 *
 *  Created on	: Mar 5, 2025
 *  Modified by	: Avik Mandal, Winter 2025 (This file functions have been adopted from Xilinx driver files)
 *.
 *  The file has all functions related to SPI
 *
 */

#ifndef SRC_SPI_SECTION_H_
#define SRC_SPI_SECTION_H_

#include "xil_types.h"
#include "xstatus.h"

#define TRANSFER_SIZE_IN_BYTES  1

int spiInit(u32 masterDeviceId, u32 slaveDeviceId);
void spiMasterTransfer(const u8 *tx, u8 *rx, int byteCount);
void spiSlaveTransfer(const u8 *tx, u8 *rx, int byteCount);

#endif /* SRC_SPI_SECTION_H_ */
