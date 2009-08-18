/*
 * Milkymist VJ SoC (Software support library)
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 3 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */

#ifndef __HW_VGA_H
#define __HW_VGA_H

#include <hw/common.h>

#define CSR_VGA_RESET 		MMPTR(0x80003000)

#define VGA_RESET		(0x01)

#define CSR_VGA_HRES 		MMPTR(0x80003004)
#define CSR_VGA_HSYNC_START	MMPTR(0x80003008)
#define CSR_VGA_HSYNC_END	MMPTR(0x8000300C)
#define CSR_VGA_HSCAN 		MMPTR(0x80003010)

#define CSR_VGA_VRES 		MMPTR(0x80003014)
#define CSR_VGA_VSYNC_START	MMPTR(0x80003018)
#define CSR_VGA_VSYNC_END	MMPTR(0x8000301C)
#define CSR_VGA_VSCAN 		MMPTR(0x80003020)

#define CSR_VGA_BASEADDRESS	MMPTR(0x80003024)
#define CSR_VGA_BASEADDRESS_ACT	MMPTR(0x80003028)

#define CSR_VGA_BURST_COUNT	MMPTR(0x8000302C)

#endif /* __HW_VGA_H */