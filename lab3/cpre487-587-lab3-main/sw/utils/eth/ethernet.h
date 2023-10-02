#ifndef ETHERNET_H
#define ETHERNET_H

/***************************** Include Files ********************************/
#include <stdio.h>
#include <stdlib.h>
#include "xparameters.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xemacps.h"		/* defines XEmacPs API */
#ifdef XPAR_INTC_0_DEVICE_ID
#include "xintc.h"
#else
#include "xscugic.h"
#endif

#ifndef __MICROBLAZE__
#include "sleep.h"
#include "xparameters_ps.h"	/* defines XPAR values */
#include "xpseudo_asm.h"
#include "xil_mmu.h"
#endif

#define RXBD_CNT       32	/* Number of RxBDs to use */
#define TXBD_CNT       32	/* Number of TxBDs to use */


/************************** Constant Definitions ****************************/

#define EMACPS_LOOPBACK_SPEED    100	/* 100Mbps */
#define EMACPS_LOOPBACK_SPEED_1G 1000	/* 1000Mbps */
#define EMACPS_PHY_DELAY_SEC     4	/* Amount of time to delay waiting on
					   PHY to reset */
#define EMACPS_SLCR_DIV_MASK	0xFC0FC0FF

#define CSU_VERSION		0xFFCA0044
#define PLATFORM_MASK		0xF000
#define PLATFORM_SILICON	0x0000
#define VERSAL_VERSION		0xF11A0004
#define PLATFORM_MASK_VERSAL	0xF000000
#define PLATFORM_VERSALEMU	0x1000000
#define PLATFORM_VERSALSIL	0x0000000

#define EMACPS_IRPT_INTR	XPS_GEM0_INT_ID
#define EMACPS_DEVICE_ID	XPAR_XEMACPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID


/**************************** Type Definitions ******************************/

/*
 * Define an aligned data type for an ethernet frame. This declaration is
 * specific to the GNU compiler
 */
typedef char EthernetFrame[XEMACPS_MAX_VLAN_FRAME_SIZE_JUMBO]
	__attribute__ ((aligned(64)));

/************************** Variable Definitions ****************************/

extern XEmacPs EmacPsInstance;	/* Device instance used throughout examples */
extern char EmacPsMAC[];	/* Local MAC address */
extern u32 Platform;

/*
 * SLCR setting
 */
#define SLCR_LOCK_ADDR			(XPS_SYS_CTRL_BASEADDR + 0x4)
#define SLCR_UNLOCK_ADDR		(XPS_SYS_CTRL_BASEADDR + 0x8)
#define SLCR_GEM0_CLK_CTRL_ADDR		(XPS_SYS_CTRL_BASEADDR + 0x140)
#define SLCR_GEM1_CLK_CTRL_ADDR		(XPS_SYS_CTRL_BASEADDR + 0x144)


#define SLCR_LOCK_KEY_VALUE		0x767B
#define SLCR_UNLOCK_KEY_VALUE		0xDF0D
#define SLCR_ADDR_GEM_RST_CTRL		(XPS_SYS_CTRL_BASEADDR + 0x214)

/* CRL APB registers for GEM clock control */
#define CRL_GEM_DIV_MASK	0x003F3F00
#define CRL_GEM_1G_DIV0		0x00000C00
#define CRL_GEM_1G_DIV1		0x00010000

#define CRL_GEM_DIV_VERSAL_MASK		0x0003FF00
#define CRL_GEM_DIV_VERSAL_SHIFT	8

#define JUMBO_FRAME_SIZE	10240
#define FRAME_HDR_SIZE		18

#define GEMVERSION_ZYNQMP	0x7
#define GEMVERSION_VERSAL	0x107

EthernetFrame TxFrame;		/* Transmit buffer */
EthernetFrame RxFrame;		/* Receive buffer */

/*
 * Buffer descriptors are allocated in uncached memory. The memory is made
 * uncached by setting the attributes appropriately in the MMU table.
 * The minimum region for which attribute settings take effect is 2MB for
 * arm 64 variants(A53) and 1MB for the rest (R5 and A9). Hence the same
 * is allocated, even if not used fully by this example, to make sure none
 * of the adjacent global memory is affected.
 */
#if defined __aarch64__
u8 bd_space[0x200000] __attribute__ ((aligned (0x200000)));
#else
u8 bd_space[0x100000] __attribute__ ((aligned (0x100000)));
#endif

/*
 * Counters to be incremented by callbacks
 */
volatile s32 FramesRx;		/* Frames have been received */
volatile s32 FramesTx;		/* Frames have been sent */
volatile s32 DeviceErrors;	/* Number of errors detected in the device */

u32 TxFrameLength;
u32 GemVersion;

u8 *RxBdSpacePtr;
u8 *TxBdSpacePtr;

XEmacPs_Bd BdTxTerminate __attribute__ ((aligned(64)));
XEmacPs_Bd BdRxTerminate __attribute__ ((aligned(64)));

#define INTC		XScuGic

/************************** Function Prototypes *****************************/

/*
 * Utility functions implemented in xemacps_example_util.c
 */
void EmacPsUtilSetupUart(void);
void EmacPsUtilFrameHdrFormatMAC(EthernetFrame * FramePtr, char *DestAddr);
void EmacPsUtilFrameHdrFormatType(EthernetFrame * FramePtr, u16 FrameType);
void EmacPsUtilFrameSetPayloadData(EthernetFrame * FramePtr, u32 PayloadSize);
LONG EmacPsUtilFrameVerify(EthernetFrame * CheckFrame,
			   EthernetFrame * ActualFrame);
void EmacPsUtilFrameMemClear(EthernetFrame * FramePtr);
LONG EmacPsUtilEnterLoopback(XEmacPs * XEmacPsInstancePtr, u32 Speed);
void EmacPsUtilstrncpy(char *Destination, const char *Source, u32 n);
void EmacPsUtilErrorTrap(const char *Message);
void EmacpsDelay(u32 delay);


// ---------- Interrupt Handlers ----------

LONG EmacPsDmaIntrInit(INTC * IntcInstancePtr,
			  XEmacPs * EmacPsInstancePtr,
			  u16 EmacPsDeviceId,
			  u16 EmacPsIntrId);

void EmacPsDmaCleanUp(INTC * IntcInstancePtr, XEmacPs *EmacPsInstancePtr, u16 EmacPsIntrId);


/*
 * Interrupt setup and Callbacks for ethernet
 */

LONG EmacPsSetupIntrSystem(INTC * IntcInstancePtr,
				  XEmacPs * EmacPsInstancePtr,
				  u16 EmacPsIntrId);

void EmacPsDisableIntrSystem(INTC * IntcInstancePtr,
				     u16 EmacPsIntrId);


/*
 * Utility routines
 */
LONG EmacPsResetDevice(XEmacPs * EmacPsInstancePtr);
void XEmacPsClkSetup(XEmacPs *EmacPsInstancePtr, u16 EmacPsIntrId);
void XEmacPs_SetMdioDivisor(XEmacPs *InstancePtr, XEmacPs_MdcDiv Divisor);

#endif // ETHERNET_H
