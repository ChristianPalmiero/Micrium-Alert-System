/*
 *********************************************************************************************************
 *
 *                                            ASSIGNMENT 1
 *
 *                                        Freescale Kinetis K64
 *                                               on the
 *
 *                                         Freescale FRDM-K64F
 *                                          Evaluation Board
 *
 * Filename      : app.c
 * Version       : V1.00
 * Programmer(s) : Palmiero Christian (S231599)
 *
 * Note:
 * Please connect the GND pin first before supplying power to VCC.
 * Please make sure the surface of object to be detect should have at least 0.5 meter^2 for better performance.
 * Please make sure the distance of object to be detect should be between 2 centimeters and 4 meters.
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */
#include "fsl_interrupt_manager.h"
#include "fsl_gpio_common.h"
#include "board.h"
#include "fsl_device_registers.h"
#include "fsl_ftm_driver.h"
#include "fsl_ftm_hal.h"
#include "fsl_ftm_features.h"
#include "fsl_clock_manager.h"
#include "fsl_ftm_common.h"
#include "stdint.h"     
#include  <string.h>
#include  <math.h>
#include  <lib_math.h>
#include  <cpu_core.h>
#include  <app_cfg.h>
#include  <os.h>
#include  <fsl_os_abstraction.h>
#include  <system_MK64F12.h>
#include  <bsp_ser.h>

#define N 11


/*
 *********************************************************************************************************
 *                                            LOCAL DEFINES
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 */

static  OS_TCB       AppTaskStartTCB;
static  OS_TCB       TrigTaskTCB;
static  OS_TCB       MainTaskTCB;
static  OS_TCB       BlinkingTaskTCB;

static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK      TrigTaskStk[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK      MainTaskStk[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK      BlinkingTaskStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_SEM 		 Sem;
static  OS_MUTEX 	 MyMutex;
static  int 		 TableIndex=-1, period=-1;
static  uint32_t 	 led;
static  CPU_TS64	 temp;

/*
 *********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

static  void  AppTaskStart (void  *p_arg);
static  void  TrigTask (void  *p_arg);
static  void  MainTask (void *p_arg);
static  void  BlinkingTask (void *p_arg);

/*
 *********************************************************************************************************
 *                                      Interrupt Handler Function
 *********************************************************************************************************
 */

void BSP_FTM3_int_hdlr( void )
{
	OS_ERR 	 err;
	uint32_t instance=3;    	          					 /* The FTM instance that performs a positive pulse width measurement is the third one, FTM3 */
	uint8_t  channel=0x01;									 /* The channel that raises the interrupt is CH1 */
	uint16_t init=0x0000, tof=0x0000;						 /* The TOF variable is used in order to handle the counter overflow */
	uint32_t ftmBaseAddr = g_ftmBaseAddr[instance];
	CPU_TS64 before=0, after=0;

	CPU_CRITICAL_ENTER();
	OSIntEnter();                                            /* Tell the OS that we are starting an ISR */

	/* If CH1F = 1 */
	if( FTM_HAL_HasChnEventOccurred(ftmBaseAddr, channel) )
	{
		/* If an overflow has occurred, TOF bit is cleared and C0V and C1V values are incremented by FFFF, that is
		 * the final value of the counter (MOD). Max one overflow can occur between two falling edges of the Echo signal */
		if(FTM_HAL_HasTimerOverflowed(ftmBaseAddr)){
			FTM_HAL_ClearTimerOverflow(ftmBaseAddr);
			tof=0xFFFF;
		}
		before = FTM_HAL_GetChnCountVal(ftmBaseAddr, channel-1, before) + tof;
		after = FTM_HAL_GetChnCountVal(ftmBaseAddr, channel, after) + tof;
		temp=after-before;

		/* CH0F and CH1F bits are cleared and the COUNT register is reset to its initial value */
		FTM_HAL_ClearChnEventStatus(ftmBaseAddr, channel-1);
		FTM_HAL_ClearChnEventStatus(ftmBaseAddr, channel);
		FTM_HAL_SetCounter(ftmBaseAddr,init);
		OSSemPost( &Sem, OS_OPT_POST_1+OS_OPT_POST_NO_SCHED, &err );
	}

	CPU_CRITICAL_EXIT();

	OSIntExit();
}


/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C code.  It is assumed that your code will call
 *               main() once you have performed all necessary initialization.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This the main standard entry point.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

int  main (void)
{
	OS_ERR   err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
	CPU_ERR  cpu_err;
#endif

	hardware_init();

	GPIO_DRV_Init(switchPins, ledPins);

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
	CPU_NameSet((CPU_CHAR *)"MK64FN1M0VMD12",
			(CPU_ERR  *)&cpu_err);
#endif

	OSA_Init();                                                                   /* Init uC/OS-III. */
	INT_SYS_InstallHandler(FTM3_IRQn, BSP_FTM3_int_hdlr);						  /* Installing an handler for FTM3	*/
	OSSemCreate( &Sem, "Main Task Semaphore", 0, &err );		    	 		  /* Creating a semaphore */
	OSMutexCreate(&MyMutex, "My Mutex", &err);
	OSTaskCreate(&AppTaskStartTCB,                                                /* Creating the start task */
			"App Task Start",
			AppTaskStart,
			0u,
			APP_CFG_TASK_START_PRIO,
			&AppTaskStartStk[0u],
			(APP_CFG_TASK_START_STK_SIZE / 10u),
			APP_CFG_TASK_START_STK_SIZE,
			0u,
			0u,
			0u,
			(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
			&err);

	OSA_Start();                                                                  /* Start multitasking (i.e. give control to uC/OS-III) */

	while (DEF_ON) {                                                              /* Should Never Get Here */
		;
	}
}


/*
 *********************************************************************************************************
 *                                          STARTUP TASK
 *
 * Description : This is the startup task.  As mentioned in the book's text, you MUST
 *               initialize the ticker only once multitasking has started.
 *
 * Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
 *                   used.  The compiler should not generate any code for this statement.
 *********************************************************************************************************
 */

static void AppTaskStart (void *p_arg)
{
	OS_ERR    err;

	(void)p_arg;                                                                  /* See Note #1 */

	CPU_Init();                                                                   /* Initialize the uC/CPU Services */
	Mem_Init();                                                                   /* Initialize the Memory Management Module */
	Math_Init();                                                                  /* Initialize the Mathematical Module */

	BSP_Ser_Init(115200u);

	OSTaskCreate(&TrigTaskTCB,                                                    /* Creating the trigger task */
			"Trig Task",
			TrigTask,
			0u,
			APP_CFG_TASK_START_PRIO,
			&TrigTaskStk[0u],
			(APP_CFG_TASK_START_STK_SIZE / 10u),
			APP_CFG_TASK_START_STK_SIZE,
			0u,
			0u,
			0u,
			(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
			&err);

	OSTaskCreate(&MainTaskTCB,                                                    /* Creating the main task */
			"Main Task",
			MainTask,
			0u,
			APP_CFG_TASK_START_PRIO,
			&MainTaskStk[0u],
			(APP_CFG_TASK_START_STK_SIZE / 10u),
			APP_CFG_TASK_START_STK_SIZE,
			0u,
			0u,
			0u,
			(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
			&err);

	OSTaskCreate(&BlinkingTaskTCB,                                                /* Creating the blinking task */
			"Blinking Task",
			BlinkingTask,
			0u,
			APP_CFG_TASK_START_PRIO,
			&BlinkingTaskStk[0u],
			(APP_CFG_TASK_START_STK_SIZE / 10u),
			APP_CFG_TASK_START_STK_SIZE,
			0u,
			0u,
			0u,
			(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
			&err);

	while (DEF_TRUE) {                                                            /* Task body, always written as an infinite loop */
		;
	}

}

static void MainTask (void *p_arg)
{
	OS_ERR err;
	CPU_TS ts;
	char tmp[80];
	int first[N]={2,10,25,50,75,100,120,140,160,180,200};                         /* Vector of measurements' left bounds in cm            */
	int last[N]={10,25,50,75,100,120,140,160,180,200,401};                        /* Vector of measurements' right bounds in cm           */
	int blinktime[N]={0,400,600,800,1000,200,400,600,800,1000,2000};              /* Vector of blinking periods in ms: 0 represents an infinite period */
	int i, flag, d1, d2, MeasurementIndex=0, ps=128;							  /* The prescaler factor is set to 128 */
	uint32_t instance=3, frequency=0;;											  /* The FTM instance that performs a positive pulse width measurement is the third one, FTM3 */
	uint32_t ftmBaseAddr = g_ftmBaseAddr[instance];
	uint8_t channel = 0x00, secondchannel = 0x01;								  /* In the dual edge continuous capture mode, the first channel is CH0, the second one is CH1 */
	float avg, f2, distance, FirstDistance, SecondDistance;

	/* The FTM3 instance of the timer is configured in 'dual edge continuous capture mode' */
	ftm_dual_param_t ftmParam = {
			.mode                   = kFtmDualEdgeCapture,
			.dualEdgeMode           = kFtmContinuous,
	};

	(void)p_arg;

	configure_ftm_pins(instance);										//FTM3 CH0 is assigned to pin D0 Alt4

	ftm_user_config_t ftmInfo;
	memset(&ftmInfo, 0, sizeof(ftmInfo));

	/* The FTM3 is configured in order to perform a positive pulse width measurement */
	CLOCK_SYS_EnableFtmClock(instance);									//The Clk is enabled
	FTM_HAL_Reset(ftmBaseAddr, instance);								//All the registers are reset
	FTM_HAL_Enable(ftmBaseAddr, true); 									//FTMEN=1
	FTM_HAL_SetClockPs(ftmBaseAddr, kFtmDividedBy128); 					//PS=111
	FTM_HAL_SetTofFreq(ftmBaseAddr, 0x00);								//NUMTOF=0
	FTM_HAL_SetWriteProtectionCmd(ftmBaseAddr, false); 					//MODE[WPIS]=1
	FTM_HAL_SetCpwms(ftmBaseAddr, 0);									//CPWMS=0 --> Up-counting configuration
	NVIC_ClearPendingIRQ(g_ftmIrqId[instance]);
	INT_SYS_EnableIRQ(g_ftmIrqId[instance]);
	FTM_HAL_ClearTimerOverflow(g_ftmBaseAddr[instance]);				//TOF=0
	FTM_HAL_SetDualEdgeCaptureCmd(ftmBaseAddr, channel, true);			//DECAPEN=1
	FTM_HAL_SetChnEdgeLevel(ftmBaseAddr, channel, 1);					//FTM3_CH0 detects rising edges (ELS0B:ELS0A = 01)
	FTM_HAL_SetChnEdgeLevel(ftmBaseAddr, secondchannel, 2);				//FTM3_CH1 detects falling edges (ELS1B:ELS1A = 10)
	FTM_HAL_SetChnMSnBAMode(ftmBaseAddr, channel, 1);					//MS0A=1 MS0B=X for CH0
	FTM_HAL_SetCounterInitVal(ftmBaseAddr, 0);							//CNTINIT=0
	FTM_HAL_SetMod(ftmBaseAddr, 0xFFFF);								//MOD=FFFF --> Free running counter
	FTM_HAL_EnableChnInt(ftmBaseAddr, secondchannel);					//CH1IE=1
	FTM_HAL_SetSoftwareTriggerCmd(ftmBaseAddr, true);					//Issue a software trigger to update registers
#if FSL_FEATURE_FTM_BUS_CLOCK
	CLOCK_SYS_GetFreq(kBusClock, &frequency);
#else
	CLOCK_SYS_GetFreq(kSystemClock, &frequency);
#endif
	FTM_HAL_SetClockSource(ftmBaseAddr, kClock_source_FTM_SystemClk);	//Clock=System clock 120MHz
	FTM_HAL_SetDualChnDecapCmd(ftmBaseAddr, channel, true);				//DECAP=1;


	while (DEF_ON) {                                                    /* Task body, always written as an infinite loop.       */
		OSSemPend(&Sem, 0,OS_OPT_PEND_BLOCKING,&ts, &err);

		/* The distance in cm is computed */
		distance=(float)((1000000.0*ps*(temp)/58)/frequency);

		/* The sensor ranging distance spreads from 2cm to 400 cm: all the measurements that exceed these boundaries are rounded */
		if(distance<2)
			distance=2;
		else if(distance>400)
			distance=400;

		/* Two measurements must be computed and, then, their avg is calculated, in order to reach a better accuracy */
		if(MeasurementIndex++==0)
			FirstDistance = distance;
		else{
			SecondDistance = distance;
			MeasurementIndex=0;

			avg=(SecondDistance+FirstDistance)/2;
			d1 = avg;          											// Get the integer part (ex: 678).
			f2 = avg - d1;   											// Get fractional part (ex: 678.01 - 678 = 0.01).
			d2 = (int)(f2 * 100);   									// Turn into integer (ex: 1).
			sprintf( tmp, "Distance [cm] = %d.%02d\n", d1, d2);
			APP_TRACE_DBG((tmp));

			/* The range index the measurement belongs to is computed */
			i=0;
			flag=1;
			while(i<N && flag){
				if(avg>first[i] && avg<last[i])
					flag=0;
				i++;
			}

			/* If the next blinking period is different from the current one, the next LED that has to blink is assigned to the global
			 * variable 'led', all the LEDs are turned off and the blinking task is woken up
			 */
			if(blinktime[i-1] != period){
				OSMutexPend((OS_MUTEX *)&MyMutex, (OS_TICK)0, (OS_OPT)OS_OPT_PEND_BLOCKING, (CPU_TS*)&ts, (OS_ERR*)&err);
				period=blinktime[i-1];                                                  /* The global variable 'period' is properly updated    */
				TableIndex=i-1;                                                         /* The global variable 'TableIndex' is properly updated    */
				if(TableIndex==10)                                                      /* The next LED that has to blink is assigned to the global variable 'led'  */
					led = BOARD_GPIO_LED_GREEN ;
				else if(TableIndex>=5 && TableIndex<=9)
					led = BOARD_GPIO_LED_BLUE ;
				else
					led = BOARD_GPIO_LED_RED ;

				GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_RED );                            /* The LEDs are turned off in order to perform a blinking with a different period */
				GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_GREEN );
				GPIO_DRV_SetPinOutput( BOARD_GPIO_LED_BLUE );

				OSTimeDlyResume(&BlinkingTaskTCB, &err);								/* The blinking task is woken up, in case it was sleeping */
				OSMutexPost((OS_MUTEX*)&MyMutex, (OS_OPT)OS_OPT_POST_NONE, (OS_ERR*)&err);
			}
		}
	}
}

static void TrigTask (void *p_arg)
{
	(void)p_arg;

	/* The FTM0 instance of the timer is configured in 'Edge aligned PWM mode'.
	 * The period of the CH0 output (Trig signal) is equal to 67ms (1/15 Hz), the width of the positive pulse is equal to  670us (1% duty cycle)
	 */
	ftm_pwm_param_t ftmParam = {
			.mode                   = kFtmEdgeAlignedPWM,
			.edgeMode               = kFtmHighTrue,
			.uFrequencyHZ           = 15,
			.uDutyCyclePercent      = 1,
			.uFirstEdgeDelayPercent = 0,
	};

	/* FTM0 CH0 is assigned to pin C1 Alt4 */
	configure_ftm_pins(BOARD_FTM_INSTANCE);

	ftm_user_config_t ftmInfo;
	memset(&ftmInfo, 0, sizeof(ftmInfo));

	/* The DRV_init function performs the initialization of the timer, the DRV_PwmStart function starts the counter */
	FTM_DRV_Init(BOARD_FTM_INSTANCE, &ftmInfo);
	FTM_DRV_PwmStart(BOARD_FTM_INSTANCE, &ftmParam, BOARD_FTM_CHANNEL);

}

static void BlinkingTask (void *p_arg)
{
	OS_ERR err;
	CPU_TS ts;

	(void)p_arg;

	while (DEF_ON) {
		if(period!=-1){                                                         	  /* If at least one measurement has been collected       */
			OSMutexPend((OS_MUTEX *)&MyMutex, (OS_TICK)0, (OS_OPT)OS_OPT_PEND_BLOCKING, (CPU_TS*)&ts, (OS_ERR*)&err);
			if(TableIndex==0)                                                         /* The right LED is toggled                             */
				GPIO_DRV_ClearPinOutput( led );
			else
				GPIO_DRV_TogglePinOutput( led );
			OSMutexPost((OS_MUTEX*)&MyMutex, (OS_OPT)OS_OPT_POST_NONE, (OS_ERR*)&err);
			OSTimeDlyHMSM(0u, 0u, 0u, (CPU_INT32U)period, OS_OPT_TIME_HMSM_STRICT, &err); /* The task waits for an amount of ms equal to the 'period' value */
		}
	}
}
