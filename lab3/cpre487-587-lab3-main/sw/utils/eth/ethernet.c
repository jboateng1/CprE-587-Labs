
/***************************** Include Files ********************************/
#include "ethernet.h"
#include "xil_exception.h"

#ifndef __MICROBLAZE__
#include "xil_mmu.h"
#endif


static void XEmacPsSendHandler(void *Callback);
static void XEmacPsRecvHandler(void *Callback);
static void XEmacPsErrorHandler(void *Callback, u8 Direction, u32 ErrorWord);

/****************************************************************************/
/**
*
* This function demonstrates the usage of the EmacPs driver by sending by
* sending and receiving frames in interrupt driven DMA mode.
*
*
* @param	IntcInstancePtr is a pointer to the instance of the Intc driver.
* @param	EmacPsInstancePtr is a pointer to the instance of the EmacPs
*		driver.
* @param	EmacPsDeviceId is Device ID of the EmacPs Device , typically
*		XPAR_<EMACPS_instance>_DEVICE_ID value from xparameters.h.
* @param	EmacPsIntrId is the Interrupt ID and is typically
*		XPAR_<EMACPS_instance>_INTR value from xparameters.h.
*
* @return	XST_SUCCESS to indicate success, otherwise XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
LONG EmacPsDmaIntrInit(INTC * IntcInstancePtr,
			  XEmacPs * EmacPsInstancePtr,
			  u16 EmacPsDeviceId,
			  u16 EmacPsIntrId)
{
	LONG Status;
	XEmacPs_Config *Config;
	XEmacPs_Bd BdTemplate;

	/*************************************/
	/* Setup device for first-time usage */
	/*************************************/

#if EL1_NONSECURE
	/* Request device to indicate it is in use by this application */
#ifdef XPAR_PSV_ETHERNET_0_DEVICE_ID
	if (EmacPsIntrId == XPS_GEM0_INT_ID) {
		Xil_Smc(PM_REQUEST_DEVICE_SMC_FID, DEV_GEM_0, 1, 0, 100, 1, 0, 0);
	}
#endif
#ifdef XPAR_PSV_ETHERNET_1_DEVICE_ID
	if (EmacPsIntrId == XPS_GEM1_INT_ID) {
		Xil_Smc(PM_REQUEST_DEVICE_SMC_FID, DEV_GEM_1, 1, 0, 100, 1, 0, 0);
	}
#endif
#endif

	/*
	 *  Initialize instance. Should be configured for DMA
	 *  This example calls _CfgInitialize instead of _Initialize due to
	 *  retiring _Initialize. So in _CfgInitialize we use
	 *  XPAR_(IP)_BASEADDRESS to make sure it is not virtual address.
	 */
	Config = XEmacPs_LookupConfig(EmacPsDeviceId);

	Status = XEmacPs_CfgInitialize(EmacPsInstancePtr, Config,
					Config->BaseAddress);

	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error in initialize");
		return XST_FAILURE;
	}

	GemVersion = ((Xil_In32(Config->BaseAddress + 0xFC)) >> 16) & 0xFFF;

	if (GemVersion == GEMVERSION_VERSAL) {
		Platform = Xil_In32(VERSAL_VERSION);
	} else if (GemVersion > 2) {
		Platform = Xil_In32(CSU_VERSION);
	}
	/* Enable jumbo frames for zynqmp */
	if (GemVersion > 2) {
		XEmacPs_SetOptions(EmacPsInstancePtr, XEMACPS_JUMBO_ENABLE_OPTION);
	}

	XEmacPsClkSetup(EmacPsInstancePtr, EmacPsIntrId);

	/*
	 * Set the MAC address
	 */
	Status = XEmacPs_SetMacAddress(EmacPsInstancePtr, EmacPsMAC, 1);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setting MAC address");
		return XST_FAILURE;
	}
	/*
	 * Setup callbacks
	 */
	Status = XEmacPs_SetHandler(EmacPsInstancePtr,
				     XEMACPS_HANDLER_DMASEND,
				     (void *) XEmacPsSendHandler,
				     EmacPsInstancePtr);
	Status |=
		XEmacPs_SetHandler(EmacPsInstancePtr,
				    XEMACPS_HANDLER_DMARECV,
				    (void *) XEmacPsRecvHandler,
				    EmacPsInstancePtr);
	Status |=
		XEmacPs_SetHandler(EmacPsInstancePtr, XEMACPS_HANDLER_ERROR,
				    (void *) XEmacPsErrorHandler,
				    EmacPsInstancePtr);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error assigning handlers");
		return XST_FAILURE;
	}

	/* Gem IP version on Zynq-7000 */
	if (GemVersion == 2)
	{
		/*
		 * The BDs need to be allocated in uncached memory. Hence the 1 MB
		 * address range that starts at "bd_space" is made uncached.
		 */
#if !defined(__MICROBLAZE__) && !defined(ARMR5)
		Xil_SetTlbAttributes((INTPTR)bd_space, DEVICE_MEMORY);
#else
		Xil_DCacheDisable();
#endif

	}

	if (GemVersion > 2) {

#if defined (ARMR5)
		Xil_SetTlbAttributes((INTPTR)bd_space, STRONG_ORDERD_SHARED |
					PRIV_RW_USER_RW);
#endif
#if defined __aarch64__
		Xil_SetTlbAttributes((UINTPTR)bd_space, NORM_NONCACHE |
					INNER_SHAREABLE);
#endif
	}

	/* Allocate Rx and Tx BD space each */
	RxBdSpacePtr = &(bd_space[0]);
	TxBdSpacePtr = &(bd_space[0x10000]);

	/*
	 * Setup RxBD space.
	 *
	 * We have already defined a properly aligned area of memory to store
	 * RxBDs at the beginning of this source code file so just pass its
	 * address into the function. No MMU is being used so the physical
	 * and virtual addresses are the same.
	 *
	 * Setup a BD template for the Rx channel. This template will be
	 * copied to every RxBD. We will not have to explicitly set these
	 * again.
	 */
	XEmacPs_BdClear(&BdTemplate);

	/*
	 * Create the RxBD ring
	 */
	Status = XEmacPs_BdRingCreate(&(XEmacPs_GetRxRing
				       (EmacPsInstancePtr)),
				       (UINTPTR) RxBdSpacePtr,
				       (UINTPTR) RxBdSpacePtr,
				       XEMACPS_BD_ALIGNMENT,
				       RXBD_CNT);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up RxBD space, BdRingCreate");
		return XST_FAILURE;
	}

	Status = XEmacPs_BdRingClone(&(XEmacPs_GetRxRing(EmacPsInstancePtr)),
				      &BdTemplate, XEMACPS_RECV);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up RxBD space, BdRingClone");
		return XST_FAILURE;
	}

	/*
	 * Setup TxBD space.
	 *
	 * Like RxBD space, we have already defined a properly aligned area
	 * of memory to use.
	 *
	 * Also like the RxBD space, we create a template. Notice we don't
	 * set the "last" attribute. The example will be overriding this
	 * attribute so it does no good to set it up here.
	 */
	XEmacPs_BdClear(&BdTemplate);
	XEmacPs_BdSetStatus(&BdTemplate, XEMACPS_TXBUF_USED_MASK);

	/*
	 * Create the TxBD ring
	 */
	Status = XEmacPs_BdRingCreate(&(XEmacPs_GetTxRing
				       (EmacPsInstancePtr)),
				       (UINTPTR) TxBdSpacePtr,
				       (UINTPTR) TxBdSpacePtr,
				       XEMACPS_BD_ALIGNMENT,
				       TXBD_CNT);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up TxBD space, BdRingCreate");
		return XST_FAILURE;
	}
	Status = XEmacPs_BdRingClone(&(XEmacPs_GetTxRing(EmacPsInstancePtr)),
				      &BdTemplate, XEMACPS_SEND);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up TxBD space, BdRingClone");
		return XST_FAILURE;
	}

	if (GemVersion > 2)
	{
		/*
		 * This version of GEM supports priority queuing and the current
		 * driver is using tx priority queue 1 and normal rx queue for
		 * packet transmit and receive. The below code ensure that the
		 * other queue pointers are parked to known state for avoiding
		 * the controller to malfunction by fetching the descriptors
		 * from these queues.
		 */
		XEmacPs_BdClear(&BdRxTerminate);
		XEmacPs_BdSetAddressRx(&BdRxTerminate, (XEMACPS_RXBUF_NEW_MASK |
						XEMACPS_RXBUF_WRAP_MASK));
		XEmacPs_Out32((Config->BaseAddress + XEMACPS_RXQ1BASE_OFFSET),
			       (UINTPTR)&BdRxTerminate);
		XEmacPs_BdClear(&BdTxTerminate);
		XEmacPs_BdSetStatus(&BdTxTerminate, (XEMACPS_TXBUF_USED_MASK |
						XEMACPS_TXBUF_WRAP_MASK));
		XEmacPs_Out32((Config->BaseAddress + XEMACPS_TXQBASE_OFFSET),
			       (UINTPTR)&BdTxTerminate);
		if (Config->IsCacheCoherent == 0) {
			Xil_DCacheFlushRange((UINTPTR)(&BdTxTerminate), 64);
		}
	}

	/*
	 * Set emacps to phy loopback
	 */
//	if (GemVersion == 2)
//	{
//		XEmacPs_SetMdioDivisor(EmacPsInstancePtr, MDC_DIV_224);
//		EmacpsDelay(1);
//		EmacPsUtilEnterLoopback(EmacPsInstancePtr, EMACPS_LOOPBACK_SPEED_1G);
//		XEmacPs_SetOperatingSpeed(EmacPsInstancePtr, EMACPS_LOOPBACK_SPEED_1G);
//	}
//	else
//	{
//		if ((Platform & PLATFORM_MASK_VERSAL) == PLATFORM_VERSALEMU) {
//			XEmacPs_SetMdioDivisor(EmacPsInstancePtr, MDC_DIV_8);
//		} else {
//			XEmacPs_SetMdioDivisor(EmacPsInstancePtr, MDC_DIV_224);
//		}
//
//		if (((Platform & PLATFORM_MASK) == PLATFORM_SILICON) ||
//			((Platform & PLATFORM_MASK_VERSAL) == PLATFORM_VERSALSIL)) {
//			EmacPsUtilEnterLoopback(EmacPsInstancePtr, EMACPS_LOOPBACK_SPEED_1G);
//			XEmacPs_SetOperatingSpeed(EmacPsInstancePtr,EMACPS_LOOPBACK_SPEED_1G);
//		} else {
//			EmacPsUtilEnterLoopback(EmacPsInstancePtr, EMACPS_LOOPBACK_SPEED);
//			XEmacPs_SetOperatingSpeed(EmacPsInstancePtr,EMACPS_LOOPBACK_SPEED);
//		}
//	}

	/*
	 * Setup the interrupt controller and enable interrupts
	 */
	Status = EmacPsSetupIntrSystem(IntcInstancePtr,
					EmacPsInstancePtr, EmacPsIntrId);

	return XST_SUCCESS;
}


void EmacPsDmaCleanUp(INTC * IntcInstancePtr, XEmacPs *EmacPsInstancePtr, u16 EmacPsIntrId) {
	/*
	 * Disable the interrupts for the EmacPs device
	 */
	EmacPsDisableIntrSystem(IntcInstancePtr, EmacPsIntrId);

	/*
	 * Stop the device
	 */
	XEmacPs_Stop(EmacPsInstancePtr);
}


/****************************************************************************/
/**
* This function resets the device but preserves the options set by the user.
*
* The descriptor list could be reinitialized with the same calls to
* XEmacPs_BdRingClone() as used in main(). Doing this is a matter of
* preference.
* In many cases, an OS may have resources tied up in the descriptors.
* Reinitializing in this case may bad for the OS since its resources may be
* permamently lost.
*
* @param	EmacPsInstancePtr is a pointer to the instance of the EmacPs
*		driver.
*
* @return	XST_SUCCESS if successful, else XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
LONG EmacPsResetDevice(XEmacPs * EmacPsInstancePtr)
{
	LONG Status = 0;
	u8 MacSave[6];
	u32 Options;
	XEmacPs_Bd BdTemplate;


	/*
	 * Stop device
	 */
	XEmacPs_Stop(EmacPsInstancePtr);

	/*
	 * Save the device state
	 */
	XEmacPs_GetMacAddress(EmacPsInstancePtr, &MacSave, 1);
	Options = XEmacPs_GetOptions(EmacPsInstancePtr);

	/*
	 * Stop and reset the device
	 */
	XEmacPs_Reset(EmacPsInstancePtr);

	/*
	 * Restore the state
	 */
	XEmacPs_SetMacAddress(EmacPsInstancePtr, &MacSave, 1);
	Status |= XEmacPs_SetOptions(EmacPsInstancePtr, Options);
	Status |= XEmacPs_ClearOptions(EmacPsInstancePtr, ~Options);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error restoring state after reset");
		return XST_FAILURE;
	}

	/*
	 * Setup callbacks
	 */
	Status = XEmacPs_SetHandler(EmacPsInstancePtr,
				     XEMACPS_HANDLER_DMASEND,
				     (void *) XEmacPsSendHandler,
				     EmacPsInstancePtr);
	Status |= XEmacPs_SetHandler(EmacPsInstancePtr,
				    XEMACPS_HANDLER_DMARECV,
				    (void *) XEmacPsRecvHandler,
				    EmacPsInstancePtr);
	Status |= XEmacPs_SetHandler(EmacPsInstancePtr, XEMACPS_HANDLER_ERROR,
				    (void *) XEmacPsErrorHandler,
				    EmacPsInstancePtr);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error assigning handlers");
		return XST_FAILURE;
	}

	/*
	 * Setup RxBD space.
	 *
	 * We have already defined a properly aligned area of memory to store
	 * RxBDs at the beginning of this source code file so just pass its
	 * address into the function. No MMU is being used so the physical and
	 * virtual addresses are the same.
	 *
	 * Setup a BD template for the Rx channel. This template will be copied
	 * to every RxBD. We will not have to explicitly set these again.
	 */
	XEmacPs_BdClear(&BdTemplate);

	/*
	 * Create the RxBD ring
	 */
	Status = XEmacPs_BdRingCreate(&(XEmacPs_GetRxRing
				      (EmacPsInstancePtr)),
				      (UINTPTR) RxBdSpacePtr,
				      (UINTPTR) RxBdSpacePtr,
				      XEMACPS_BD_ALIGNMENT,
				      RXBD_CNT);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up RxBD space, BdRingCreate");
		return XST_FAILURE;
	}

	Status = XEmacPs_BdRingClone(&
				      (XEmacPs_GetRxRing(EmacPsInstancePtr)),
				      &BdTemplate, XEMACPS_RECV);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up RxBD space, BdRingClone");
		return XST_FAILURE;
	}

	/*
	 * Setup TxBD space.
	 *
	 * Like RxBD space, we have already defined a properly aligned area of
	 * memory to use.
	 *
	 * Also like the RxBD space, we create a template. Notice we don't set
	 * the "last" attribute. The examples will be overriding this
	 * attribute so it does no good to set it up here.
	 */
	XEmacPs_BdClear(&BdTemplate);
	XEmacPs_BdSetStatus(&BdTemplate, XEMACPS_TXBUF_USED_MASK);

	/*
	 * Create the TxBD ring
	 */
	Status = XEmacPs_BdRingCreate(&(XEmacPs_GetTxRing
				      (EmacPsInstancePtr)),
				      (UINTPTR) TxBdSpacePtr,
				      (UINTPTR) TxBdSpacePtr,
				      XEMACPS_BD_ALIGNMENT,
				      TXBD_CNT);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up TxBD space, BdRingCreate");
		return XST_FAILURE;
	}
	Status = XEmacPs_BdRingClone(&
				      (XEmacPs_GetTxRing(EmacPsInstancePtr)),
				      &BdTemplate, XEMACPS_SEND);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Error setting up TxBD space, BdRingClone");
		return XST_FAILURE;
	}

	/*
	 * Restart the device
	 */
	XEmacPs_Start(EmacPsInstancePtr);

	return XST_SUCCESS;
}


/****************************************************************************/
/**
*
* This function setups the interrupt system so interrupts can occur for the
* EMACPS.
* @param	IntcInstancePtr is a pointer to the instance of the Intc driver.
* @param	EmacPsInstancePtr is a pointer to the instance of the EmacPs
*		driver.
* @param	EmacPsIntrId is the Interrupt ID and is typically
*		XPAR_<EMACPS_instance>_INTR value from xparameters.h.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
LONG EmacPsSetupIntrSystem(INTC *IntcInstancePtr,
				  XEmacPs *EmacPsInstancePtr,
				  u16 EmacPsIntrId)
{
	LONG Status;

#ifdef XPAR_INTC_0_DEVICE_ID

	#ifndef TESTAPP_GEN
	/*
	 * Initialize the interrupt controller driver so that it's ready to
	 * use.
	 */
	Status = XIntc_Initialize(IntcInstancePtr, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	#endif

	/*
	 * Connect the handler that will be called when an interrupt
	 * for the device occurs, the handler defined above performs the
	 * specific interrupt processing for the device.
	 */
	Status = XIntc_Connect(IntcInstancePtr, EmacPsIntrId,
		(XInterruptHandler) XEmacPs_IntrHandler, EmacPsInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	#ifndef TESTAPP_GEN
/*
 * Start the interrupt controller such that interrupts are enabled for
 * all devices that cause interrupts.
 */

	Status = XIntc_Start(IntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	#endif

	/*
	 * Enable the interrupt from the hardware
	 */
	XIntc_Enable(IntcInstancePtr, EmacPsIntrId);

	#ifndef TESTAPP_GEN
	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler) XIntc_InterruptHandler,
					IntcInstancePtr);
	#endif

#else
#ifndef TESTAPP_GEN
	XScuGic_Config *GicConfig;
	Xil_ExceptionInit();

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	GicConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == GicConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, GicConfig,
		    GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_InterruptHandler,
			IntcInstancePtr);
#endif

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, EmacPsIntrId,
			(Xil_InterruptHandler) XEmacPs_IntrHandler,
			(void *) EmacPsInstancePtr);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap
			("Unable to connect ISR to interrupt controller");
		return XST_FAILURE;
	}

	/*
	 * Enable interrupts from the hardware
	 */
	XScuGic_Enable(IntcInstancePtr, EmacPsIntrId);
#endif
#ifndef TESTAPP_GEN
	/*
	 * Enable interrupts in the processor
	 */
	Xil_ExceptionEnable();
#endif
	return XST_SUCCESS;
}


/****************************************************************************/
/**
*
* This function disables the interrupts that occur for EmacPs.
*
* @param	IntcInstancePtr is the pointer to the instance of the ScuGic
*		driver.
* @param	EmacPsIntrId is interrupt ID and is typically
*		XPAR_<EMACPS_instance>_INTR value from xparameters.h.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void EmacPsDisableIntrSystem(INTC * IntcInstancePtr,
				     u16 EmacPsIntrId)
{
	/*
	 * Disconnect and disable the interrupt for the EmacPs device
	 */
#ifdef XPAR_INTC_0_DEVICE_ID
	XIntc_Disconnect(IntcInstancePtr, EmacPsIntrId);
#else
	XScuGic_Disconnect(IntcInstancePtr, EmacPsIntrId);
#endif

}

/****************************************************************************/
/**
*
* This the Transmit handler callback function and will increment a shared
* counter that can be shared by the main thread of operation.
*
* @param	Callback is the pointer to the instance of the EmacPs device.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
static void XEmacPsSendHandler(void *Callback)
{
	xil_printf("Send Handler\r\n");
	XEmacPs *EmacPsInstancePtr = (XEmacPs *) Callback;

	/*
	 * Disable the transmit related interrupts
	 */
	XEmacPs_IntDisable(EmacPsInstancePtr, (XEMACPS_IXR_TXCOMPL_MASK |
		XEMACPS_IXR_TX_ERR_MASK));
	if (GemVersion > 2) {
	XEmacPs_IntQ1Disable(EmacPsInstancePtr, XEMACPS_INTQ1_IXR_ALL_MASK);
	}
	/*
	 * Increment the counter so that main thread knows something
	 * happened.
	 */
	FramesTx++;
}


/****************************************************************************/
/**
*
* This is the Receive handler callback function and will increment a shared
* counter that can be shared by the main thread of operation.
*
* @param	Callback is a pointer to the instance of the EmacPs device.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
static void XEmacPsRecvHandler(void *Callback)
{
	xil_printf("Recieve Handler\r\n");
	XEmacPs *EmacPsInstancePtr = (XEmacPs *) Callback;

	/*
	 * Disable the transmit related interrupts
	 */
	XEmacPs_IntDisable(EmacPsInstancePtr, (XEMACPS_IXR_FRAMERX_MASK |
		XEMACPS_IXR_RX_ERR_MASK));
	/*
	 * Increment the counter so that main thread knows something
	 * happened.
	 */
	FramesRx++;
	if (EmacPsInstancePtr->Config.IsCacheCoherent == 0) {
		Xil_DCacheInvalidateRange((UINTPTR)&RxFrame, sizeof(EthernetFrame));
	}
	if (GemVersion > 2) {
		if (EmacPsInstancePtr->Config.IsCacheCoherent == 0) {
			Xil_DCacheInvalidateRange((UINTPTR)RxBdSpacePtr, 64);
		}
	}
}


/****************************************************************************/
/**
*
* This is the Error handler callback function and this function increments
* the error counter so that the main thread knows the number of errors.
*
* @param	Callback is the callback function for the driver. This
*		parameter is not used in this example.
* @param	Direction is passed in from the driver specifying which
*		direction error has occurred.
* @param	ErrorWord is the status register value passed in.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
static void XEmacPsErrorHandler(void *Callback, u8 Direction, u32 ErrorWord)
{
	xil_printf("Error Handler\r\n");
	XEmacPs *EmacPsInstancePtr = (XEmacPs *) Callback;

	/*
	 * Increment the counter so that main thread knows something
	 * happened. Reset the device and reallocate resources ...
	 */
	DeviceErrors++;

	switch (Direction) {
	case XEMACPS_RECV:
		if (ErrorWord & XEMACPS_RXSR_HRESPNOK_MASK) {
			EmacPsUtilErrorTrap("Receive DMA error");
		}
		if (ErrorWord & XEMACPS_RXSR_RXOVR_MASK) {
			EmacPsUtilErrorTrap("Receive over run");
		}
		if (ErrorWord & XEMACPS_RXSR_BUFFNA_MASK) {
			EmacPsUtilErrorTrap("Receive buffer not available");
		}
		break;
	case XEMACPS_SEND:
		if (ErrorWord & XEMACPS_TXSR_HRESPNOK_MASK) {
			EmacPsUtilErrorTrap("Transmit DMA error");
		}
		if (ErrorWord & XEMACPS_TXSR_URUN_MASK) {
			EmacPsUtilErrorTrap("Transmit under run");
		}
		if (ErrorWord & XEMACPS_TXSR_BUFEXH_MASK) {
			EmacPsUtilErrorTrap("Transmit buffer exhausted");
		}
		if (ErrorWord & XEMACPS_TXSR_RXOVR_MASK) {
			EmacPsUtilErrorTrap("Transmit retry excessed limits");
		}
		if (ErrorWord & XEMACPS_TXSR_FRAMERX_MASK) {
			EmacPsUtilErrorTrap("Transmit collision");
		}
		if (ErrorWord & XEMACPS_TXSR_USEDREAD_MASK) {
			EmacPsUtilErrorTrap("Transmit buffer not available");
		}
		break;
	}
	/*
	 * Bypassing the reset functionality as the default tx status for q0 is
	 * USED BIT READ. so, the first interrupt will be tx used bit and it resets
	 * the core always.
	 */
	if (GemVersion == 2) {
	EmacPsResetDevice(EmacPsInstancePtr);
	}
}

/****************************************************************************/
/**
*
* This function sets up the clock divisors for 1000Mbps.
*
* @param	EmacPsInstancePtr is a pointer to the instance of the EmacPs
*			driver.
* @param	EmacPsIntrId is the Interrupt ID and is typically
*			XPAR_<EMACPS_instance>_INTR value from xparameters.h.
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void XEmacPsClkSetup(XEmacPs *EmacPsInstancePtr, u16 EmacPsIntrId)
{
	u32 ClkCntrl;
	if (GemVersion == 2)
	{
		/*************************************/
		/* Setup device for first-time usage */
		/*************************************/

	/* SLCR unlock */
	*(volatile unsigned int *)(SLCR_UNLOCK_ADDR) = SLCR_UNLOCK_KEY_VALUE;
#ifndef __MICROBLAZE__
	if (EmacPsIntrId == XPS_GEM0_INT_ID) {
#ifdef XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0
		/* GEM0 1G clock configuration*/
		ClkCntrl =
		*(volatile unsigned int *)(SLCR_GEM0_CLK_CTRL_ADDR);
		ClkCntrl &= EMACPS_SLCR_DIV_MASK;
		ClkCntrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV1 << 20);
		ClkCntrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << 8);
		*(volatile unsigned int *)(SLCR_GEM0_CLK_CTRL_ADDR) =
								ClkCntrl;
#endif
	} else if (EmacPsIntrId == XPS_GEM1_INT_ID) {
#ifdef XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV1
		/* GEM1 1G clock configuration*/
		ClkCntrl =
		*(volatile unsigned int *)(SLCR_GEM1_CLK_CTRL_ADDR);
		ClkCntrl &= EMACPS_SLCR_DIV_MASK;
		ClkCntrl |= (XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV1 << 20);
		ClkCntrl |= (XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV0 << 8);
		*(volatile unsigned int *)(SLCR_GEM1_CLK_CTRL_ADDR) =
								ClkCntrl;
#endif
	}
#else
#ifdef XPAR_AXI_INTC_0_PROCESSING_SYSTEM7_0_IRQ_P2F_ENET0_INTR
	if (EmacPsIntrId == XPAR_AXI_INTC_0_PROCESSING_SYSTEM7_0_IRQ_P2F_ENET0_INTR) {
#ifdef XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0
		/* GEM0 1G clock configuration*/
		ClkCntrl =
		*(volatile unsigned int *)(SLCR_GEM0_CLK_CTRL_ADDR);
		ClkCntrl &= EMACPS_SLCR_DIV_MASK;
		ClkCntrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV1 << 20);
		ClkCntrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << 8);
		*(volatile unsigned int *)(SLCR_GEM0_CLK_CTRL_ADDR) =
								ClkCntrl;
#endif
	}
#endif
#ifdef XPAR_AXI_INTC_0_PROCESSING_SYSTEM7_1_IRQ_P2F_ENET1_INTR
	if (EmacPsIntrId == XPAR_AXI_INTC_0_PROCESSING_SYSTEM7_1_IRQ_P2F_ENET1_INTR) {
#ifdef XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV1
		/* GEM1 1G clock configuration*/
		ClkCntrl =
		*(volatile unsigned int *)(SLCR_GEM1_CLK_CTRL_ADDR);
		ClkCntrl &= EMACPS_SLCR_DIV_MASK;
		ClkCntrl |= (XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV1 << 20);
		ClkCntrl |= (XPAR_PS7_ETHERNET_1_ENET_SLCR_1000MBPS_DIV0 << 8);
		*(volatile unsigned int *)(SLCR_GEM1_CLK_CTRL_ADDR) =
								ClkCntrl;
#endif
	}
#endif
#endif
	/* SLCR lock */
	*(unsigned int *)(SLCR_LOCK_ADDR) = SLCR_LOCK_KEY_VALUE;
	#ifndef __MICROBLAZE__
	sleep(1);
	#else
	unsigned long count=0;
	while(count < 0xffff)
	{
		count++;
	}
	#endif
	}

	if ((GemVersion == GEMVERSION_ZYNQMP) && ((Platform & PLATFORM_MASK) == PLATFORM_SILICON)) {

#ifdef XPAR_PSU_CRL_APB_S_AXI_BASEADDR
#ifdef XPAR_PSU_ETHERNET_0_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM0_INT_ID) {
			/* GEM0 1G clock configuration*/
			ClkCntrl =
			*(volatile unsigned int *)(CRL_GEM0_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_MASK;
			ClkCntrl |= CRL_GEM_1G_DIV1;
			ClkCntrl |= CRL_GEM_1G_DIV0;
			*(volatile unsigned int *)(CRL_GEM0_REF_CTRL) =
									ClkCntrl;

		}
#endif
#ifdef XPAR_PSU_ETHERNET_1_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM1_INT_ID) {

			/* GEM1 1G clock configuration*/
			ClkCntrl =
			*(volatile unsigned int *)(CRL_GEM1_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_MASK;
			ClkCntrl |= CRL_GEM_1G_DIV1;
			ClkCntrl |= CRL_GEM_1G_DIV0;
			*(volatile unsigned int *)(CRL_GEM1_REF_CTRL) =
									ClkCntrl;
		}
#endif
#ifdef XPAR_PSU_ETHERNET_2_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM2_INT_ID) {

			/* GEM2 1G clock configuration*/
			ClkCntrl =
			*(volatile unsigned int *)(CRL_GEM2_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_MASK;
			ClkCntrl |= CRL_GEM_1G_DIV1;
			ClkCntrl |= CRL_GEM_1G_DIV0;
			*(volatile unsigned int *)(CRL_GEM2_REF_CTRL) =
									ClkCntrl;

		}
#endif
#ifdef XPAR_PSU_ETHERNET_3_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM3_INT_ID) {
			/* GEM3 1G clock configuration*/
			ClkCntrl =
			*(volatile unsigned int *)(CRL_GEM3_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_MASK;
			ClkCntrl |= CRL_GEM_1G_DIV1;
			ClkCntrl |= CRL_GEM_1G_DIV0;
			*(volatile unsigned int *)(CRL_GEM3_REF_CTRL) =
									ClkCntrl;
		}
#endif
#endif
	}
	if ((GemVersion == GEMVERSION_VERSAL) &&
		((Platform & PLATFORM_MASK_VERSAL) == PLATFORM_VERSALSIL)) {

#ifdef XPAR_PSV_CRL_0_S_AXI_BASEADDR
#ifdef XPAR_PSV_ETHERNET_0_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM0_INT_ID) {
			/* GEM0 1G clock configuration*/
#if EL1_NONSECURE
			Xil_Smc(PM_SET_DIVIDER_SMC_FID, (((u64)XPAR_PSV_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << 32) | CLK_GEM0_REF), 0, 0, 0, 0, 0, 0);
#else
			ClkCntrl = Xil_In32((UINTPTR)CRL_GEM0_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_VERSAL_MASK;
			ClkCntrl |= XPAR_PSV_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << CRL_GEM_DIV_VERSAL_SHIFT;
			Xil_Out32((UINTPTR)CRL_GEM0_REF_CTRL, ClkCntrl);
#endif
		}
#endif
#ifdef XPAR_PSV_ETHERNET_1_DEVICE_ID
		if (EmacPsIntrId == XPS_GEM1_INT_ID) {

			/* GEM1 1G clock configuration*/
#if EL1_NONSECURE
			Xil_Smc(PM_SET_DIVIDER_SMC_FID, (((u64)XPAR_PSV_ETHERNET_1_ENET_SLCR_1000MBPS_DIV0 << 32) | CLK_GEM1_REF), 0, 0, 0, 0, 0, 0);
#else
			ClkCntrl = Xil_In32((UINTPTR)CRL_GEM1_REF_CTRL);
			ClkCntrl &= ~CRL_GEM_DIV_VERSAL_MASK;
			ClkCntrl |= XPAR_PSV_ETHERNET_1_ENET_SLCR_1000MBPS_DIV0 << CRL_GEM_DIV_VERSAL_SHIFT;
			Xil_Out32((UINTPTR)CRL_GEM1_REF_CTRL, ClkCntrl);
#endif
		}
#endif
#endif
	}
}





/************************** Variable Definitions ****************************/

XEmacPs EmacPsInstance;	/* XEmacPs instance used throughout examples */

/*
 * Local MAC address
 */
char EmacPsMAC[] = { 0x00, 0x0a, 0x35, 0x01, 0x02, 0x03 };

u32 Platform;

/****************************************************************************/
/**
*
* Set the MAC addresses in the frame.
*
* @param    FramePtr is the pointer to the frame.
* @param    DestAddr is the Destination MAC address.
*
* @return   None.
*
* @note     None.
*
*****************************************************************************/
void EmacPsUtilFrameHdrFormatMAC(EthernetFrame * FramePtr, char *DestAddr)
{
	char *Frame = (char *) FramePtr;
	char *SourceAddress = EmacPsMAC;
	s32 Index;

	/* Destination address */
	for (Index = 0; Index < (s32) XEMACPS_MAC_ADDR_SIZE; Index++) {
		*Frame++ = *DestAddr++;
	}

	/* Source address */
	for (Index = 0; Index < (s32) XEMACPS_MAC_ADDR_SIZE; Index++) {
		*Frame++ = *SourceAddress++;
	}
}

/****************************************************************************/
/**
*
* Set the frame type for the specified frame.
*
* @param    FramePtr is the pointer to the frame.
* @param    FrameType is the Type to set in frame.
*
* @return   None.
*
* @note     None.
*
*****************************************************************************/
void EmacPsUtilFrameHdrFormatType(EthernetFrame * FramePtr, u16 FrameType)
{
	char *Frame = (char *) FramePtr;

	/*
	 * Increment to type field
	 */
	Frame = Frame + 12;
	/*
	 * Do endian swap from little to big-endian.
	 */
	FrameType = Xil_EndianSwap16(FrameType);
	/*
	 * Set the type
	 */
	*(u16 *) Frame = FrameType;
}

/****************************************************************************/
/**
* This function places a pattern in the payload section of a frame. The pattern
* is a  8 bit incrementing series of numbers starting with 0.
* Once the pattern reaches 256, then the pattern changes to a 16 bit
* incrementing pattern:
* <pre>
*   0, 1, 2, ... 254, 255, 00, 00, 00, 01, 00, 02, ...
* </pre>
*
* @param    FramePtr is a pointer to the frame to change.
* @param    PayloadSize is the number of bytes in the payload that will be set.
*
* @return   None.
*
* @note     None.
*
*****************************************************************************/
void EmacPsUtilFrameSetPayloadData(EthernetFrame * FramePtr, u32 PayloadSize)
{
	u32 BytesLeft = PayloadSize;
	u8 *Frame;
	u16 Counter = 0;

	/*
	 * Set the frame pointer to the start of the payload area
	 */
	Frame = (u8 *) FramePtr + XEMACPS_HDR_SIZE;

	/*
	 * Insert 8 bit incrementing pattern
	 */
	while (BytesLeft && (Counter < 256)) {
		*Frame++ = (u8) Counter++;
		BytesLeft--;
	}

	/*
	 * Switch to 16 bit incrementing pattern
	 */
	while (BytesLeft) {
		*Frame++ = (u8) (Counter >> 8);	/* high */
		BytesLeft--;

		if (!BytesLeft)
			break;

		*Frame++ = (u8) Counter++;	/* low */
		BytesLeft--;
	}
}

/****************************************************************************/
/**
* This function verifies the frame data against a CheckFrame.
*
* Validation occurs by comparing the ActualFrame to the header of the
* CheckFrame. If the headers match, then the payload of ActualFrame is
* verified for the same pattern Util_FrameSetPayloadData() generates.
*
* @param    CheckFrame is a pointer to a frame containing the 14 byte header
*           that should be present in the ActualFrame parameter.
* @param    ActualFrame is a pointer to a frame to validate.
*
* @return   XST_SUCCESS if successful, else XST_FAILURE.
*
* @note     None.
*****************************************************************************/
LONG EmacPsUtilFrameVerify(EthernetFrame * CheckFrame,
			   EthernetFrame * ActualFrame)
{
	char *CheckPtr = (char *) CheckFrame;
	char *ActualPtr = (char *) ActualFrame;
	u16 BytesLeft;
	u16 Counter;
	u32 Index;

	/*
	 * Compare the headers
	 */
	for (Index = 0; Index < XEMACPS_HDR_SIZE; Index++) {
		if (CheckPtr[Index] != ActualPtr[Index]) {
			return XST_FAILURE;
		}
	}

	/*
	 * Get the length of the payload
	 */
	BytesLeft = *(u16 *) &ActualPtr[12];
	/*
	 * Do endian swap from big back to little-endian.
	 */
	BytesLeft = Xil_EndianSwap16(BytesLeft);
	/*
	 * Validate the payload
	 */
	Counter = 0;
	ActualPtr = &ActualPtr[14];

	/*
	 * Check 8 bit incrementing pattern
	 */
	while (BytesLeft && (Counter < 256)) {
		if (*ActualPtr++ != (char) Counter++) {
			return XST_FAILURE;
		}
		BytesLeft--;
	}

	/*
	 * Check 16 bit incrementing pattern
	 */
	while (BytesLeft) {
		if (*ActualPtr++ != (char) (Counter >> 8)) {	/* high */
			return XST_FAILURE;
		}

		BytesLeft--;

		if (!BytesLeft)
			break;

		if (*ActualPtr++ != (char) Counter++) {	/* low */
			return XST_FAILURE;
		}

		BytesLeft--;
	}

	return XST_SUCCESS;
}

/****************************************************************************/
/**
* This function sets all bytes of a frame to 0.
*
* @param    FramePtr is a pointer to the frame itself.
*
* @return   None.
*
* @note     None.
*
*****************************************************************************/
void EmacPsUtilFrameMemClear(EthernetFrame * FramePtr)
{
	u32 *Data32Ptr = (u32 *) FramePtr;
	u32 WordsLeft = sizeof(EthernetFrame) / sizeof(u32);

	/* frame should be an integral number of words */
	while (WordsLeft--) {
		*Data32Ptr++ = 0xDEADBEEF;
	}
}


/****************************************************************************/
/**
*
* This function copies data from source to desitnation for n bytes.
*
* @param    Destination is the targeted string to copy to.
* @param    Source is the source string to copy from.
* @param    n is number of bytes to be copied.
*
* @note     This function is similar to strncpy(), however strncpy will
*           stop either at null byte or n bytes is been copied.
*           This function will copy n bytes without checking the content.
*
*****************************************************************************/
void EmacPsUtilstrncpy(char *Destination, const char *Source, u32 n)
{
	do {
		*Destination++ = *Source++;
	} while (--n != 0);
}


/****************************************************************************/
/**
*
* This function sets the emacps to loopback mode.
*
* @param    EmacPsInstancePtr is the XEmacPs driver instance.
*
* @note     None.
*
*****************************************************************************/
void EmacPsUtilEnterLocalLoopback(XEmacPs * EmacPsInstancePtr)
{
	u32 reg;

	reg = XEmacPs_ReadReg(EmacPsInstancePtr->Config.BaseAddress,
				XEMACPS_NWCTRL_OFFSET);
	XEmacPs_WriteReg(EmacPsInstancePtr->Config.BaseAddress,
			   XEMACPS_NWCTRL_OFFSET,
			   reg | XEMACPS_NWCTRL_LOOPEN_MASK);
}


/****************************************************************************/
/**
*
* This function detects the PHY address by looking for successful MII status
* register contents.
*
* @param    The XEMACPS driver instance
*
* @return   The address of the PHY (defaults to 32 if none detected)
*
* @note     None.
*
*****************************************************************************/
#define PHY_DETECT_REG1 2
#define PHY_DETECT_REG2 3

#define PHY_ID_MARVELL	0x141
#define PHY_ID_TI		0x2000

u32 XEmacPsDetectPHY(XEmacPs * EmacPsInstancePtr)
{
	u32 PhyAddr;
	u32 Status;
	u16 PhyReg1;
	u16 PhyReg2;

	for (PhyAddr = 0; PhyAddr <= 31; PhyAddr++) {
		Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr,
					  PHY_DETECT_REG1, &PhyReg1);

		Status |= XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr,
					   PHY_DETECT_REG2, &PhyReg2);

		if ((Status == XST_SUCCESS) &&
		    (PhyReg1 > 0x0000) && (PhyReg1 < 0xffff) &&
		    (PhyReg2 > 0x0000) && (PhyReg2 < 0xffff)) {
			/* Found a valid PHY address */
			return PhyAddr;
		}
	}

	return PhyAddr;		/* default to 32(max of iteration) */
}


/****************************************************************************/
/**
*
* This function sets the PHY to loopback mode.
*
* @param    The XEMACPS driver instance
* @param    Speed is the loopback speed 10/100 Mbit.
*
* @return   XST_SUCCESS if successful, else XST_FAILURE.
*
* @note     None.
*
*****************************************************************************/
#define PHY_REG0_RESET    0x8000
#define PHY_REG0_LOOPBACK 0x4000
#define PHY_REG0_10       0x0100
#define PHY_REG0_100      0x2100
#define PHY_REG0_1000     0x0140
#define PHY_REG21_10      0x0030
#define PHY_REG21_100     0x2030
#define PHY_REG21_1000    0x0070
#define PHY_LOOPCR		0xFE
#define PHY_REGCR		0x0D
#define PHY_ADDAR		0x0E
#define PHY_RGMIIDCTL	0x86
#define PHY_RGMIICTL	0x32

#define PHY_REGCR_ADDR	0x001F
#define PHY_REGCR_DATA	0x401F

/* RGMII RX and TX tuning values */
#define PHY_TI_RGMII_ZCU102	0xA8
#define PHY_TI_RGMII_VERSALEMU	0xAB

LONG EmacPsUtilMarvellPhyLoopback(XEmacPs * EmacPsInstancePtr,
									u32 Speed, u32 PhyAddr)
{
	LONG Status;
	u16 PhyReg0  = 0;
	u16 PhyReg21  = 0;
	u16 PhyReg22  = 0;

	/*
	 * Setup speed and duplex
	 */
	switch (Speed) {
	case 10:
		PhyReg0 |= PHY_REG0_10;
		PhyReg21 |= PHY_REG21_10;
		break;
	case 100:
		PhyReg0 |= PHY_REG0_100;
		PhyReg21 |= PHY_REG21_100;
		break;
	case 1000:
		PhyReg0 |= PHY_REG0_1000;
		PhyReg21 |= PHY_REG21_1000;
		break;
	default:
		EmacPsUtilErrorTrap("Error: speed not recognized ");
		return XST_FAILURE;
	}

	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);
	/*
	 * Make sure new configuration is in effect
	 */
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	/*
	 * Switching to PAGE2
	 */
	PhyReg22 = 0x2;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 22, PhyReg22);

	/*
	 * Adding Tx and Rx delay. Configuring loopback speed.
	 */
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 21, PhyReg21);
	/*
	 * Make sure new configuration is in effect
	 */
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 21, &PhyReg21);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setting Reg 21 in Page 2");
		return XST_FAILURE;
	}
	/*
	 * Switching to PAGE0
	 */
	PhyReg22 = 0x0;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 22, PhyReg22);

	/*
	 * Issue a reset to phy
	 */
	Status  = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	PhyReg0 |= PHY_REG0_RESET;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);

	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error reset phy");
		return XST_FAILURE;
	}

	/*
	 * Enable loopback
	 */
	PhyReg0 |= PHY_REG0_LOOPBACK;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);

	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy loopback");
		return XST_FAILURE;
	}

	EmacpsDelay(1);

	return XST_SUCCESS;
}

LONG EmacPsUtilTiPhyLoopback(XEmacPs * EmacPsInstancePtr,
								u32 Speed, u32 PhyAddr)
{
	LONG Status;
	u16 PhyReg0  = 0, LoopbackSpeed = 0;
	u16 RgmiiTuning = PHY_TI_RGMII_ZCU102;

	/*
	 * Setup speed and duplex
	 */
	switch (Speed) {
	case 10:
		PhyReg0 |= PHY_REG0_10;
		break;
	case 100:
		PhyReg0 |= PHY_REG0_100;
		break;
	case 1000:
		PhyReg0 |= PHY_REG0_1000;
		break;
	default:
		EmacPsUtilErrorTrap("Error: speed not recognized ");
		return XST_FAILURE;
	}
	LoopbackSpeed = PhyReg0;

	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);
	/*
	 * Make sure new configuration is in effect
	 */
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 1, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	/* Write PHY_LOOPCR */
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_LOOPCR);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, 0xEF20);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	/* Read PHY_LOOPCR */
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_LOOPCR);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	/* SW reset */
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0x1F, 0x4000);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy speed");
		return XST_FAILURE;
	}

	/* issue a reset to phy */
	Status  = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	PhyReg0 |= PHY_REG0_RESET;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);

	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error reset phy");
		return XST_FAILURE;
	}

	EmacpsDelay(1);

	/* enable loopback */
	PhyReg0 = LoopbackSpeed | PHY_REG0_LOOPBACK;
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0, PhyReg0);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy loopback");
		return XST_FAILURE;
	}

	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, 0x10, 0x5048);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0x10, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy loopback");
		return XST_FAILURE;
	}

	/* Write to PHY_RGMIIDCTL */
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_RGMIIDCTL);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	if ((Platform & PLATFORM_MASK_VERSAL) == PLATFORM_VERSALEMU) {
		RgmiiTuning = PHY_TI_RGMII_VERSALEMU;
	}
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, RgmiiTuning);
		if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error in tuning");
		return XST_FAILURE;
	}

	/* Read PHY_RGMIIDCTL */
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_RGMIIDCTL);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error in tuning");
		return XST_FAILURE;
	}

	/* Write PHY_RGMIICTL */
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_RGMIICTL);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	Status = XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, 0xD3);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error in tuning");
		return XST_FAILURE;
	}

	/* Read PHY_RGMIICTL */
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, PHY_RGMIICTL);
	XEmacPs_PhyWrite(EmacPsInstancePtr, PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, PHY_ADDAR, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error in tuning");
		return XST_FAILURE;
	}

	Status = XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, 0x11, &PhyReg0);
	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy loopback");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function provides delays in seconds
*
* @param    Delay in seconds
*
* @return   None.
*
* @note     for microblaze the delay is not accurate and it need to tuned.
*
*****************************************************************************/
void EmacpsDelay(u32 delay)
{

#ifndef __MICROBLAZE__
	sleep(delay);
#else
	unsigned long count=0;
	while(count < (delay * 0xffff))
	{
		count++;
	}
#endif

}


LONG EmacPsUtilEnterLoopback(XEmacPs * EmacPsInstancePtr, u32 Speed)
{
	LONG Status;
	u16 PhyIdentity;
	u32 PhyAddr;

	/*
	 * Detect the PHY address
	 */
	PhyAddr = XEmacPsDetectPHY(EmacPsInstancePtr);

	if (PhyAddr >= 32) {
		EmacPsUtilErrorTrap("Error detect phy");
		return XST_FAILURE;
	}

	XEmacPs_PhyRead(EmacPsInstancePtr, PhyAddr, PHY_DETECT_REG1, &PhyIdentity);

	if (PhyIdentity == PHY_ID_MARVELL) {
		Status = EmacPsUtilMarvellPhyLoopback(EmacPsInstancePtr, Speed, PhyAddr);
	}

	if (PhyIdentity == PHY_ID_TI) {
		Status = EmacPsUtilTiPhyLoopback(EmacPsInstancePtr, Speed, PhyAddr);
	}

	if (Status != XST_SUCCESS) {
		EmacPsUtilErrorTrap("Error setup phy loopback");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function is called by example code when an error is detected. It
* can be set as a breakpoint with a debugger or it can be used to print out
* the given message if there is a UART or STDIO device.
*
* @param    Message is the text explaining the error
*
* @return   None
*
* @note     None
*
*****************************************************************************/
void EmacPsUtilErrorTrap(const char *Message)
{
	static u32 Count = 0;

	Count++;

#ifdef STDOUT_BASEADDRESS
	xil_printf("%s\r\n", Message);
#else
	(void) Message;
#endif
}
