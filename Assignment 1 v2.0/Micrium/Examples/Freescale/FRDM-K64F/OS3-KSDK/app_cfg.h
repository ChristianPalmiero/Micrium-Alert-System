/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      APPLICATION CONFIGURATION
*
*                                        Freescale Kinetis K64
*                                               on the
*
*                                         Freescale FRDM-K64F
*                                          Evaluation Board
*
* Filename      : app_cfg.h
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/

#ifndef  APP_CFG_MODULE_PRESENT
#define  APP_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*                                       ADDITIONAL uC/MODULE ENABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_PRIO                      2u
#define  APP_CFG_TASK_OBJ_PRIO                        3u
#define  APP_CFG_TASK_EQ_PRIO                         4u


/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_STK_SIZE                512u
#define  APP_CFG_TASK_EQ_STK_SIZE                   512u
#define  APP_CFG_TASK_OBJ_STK_SIZE                  256u

/*
*********************************************************************************************************
*                                          TASK STACK SIZES LIMIT
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_STK_SIZE_PCT_FULL             90u
#define  APP_CFG_TASK_EQ_STK_SIZE_PCT_FULL                90u


#define  APP_CFG_TASK_START_STK_SIZE_LIMIT       (APP_CFG_TASK_START_STK_SIZE * (100u - APP_CFG_TASK_START_STK_SIZE_PCT_FULL)) / 100u
#define  APP_CFG_TASK_EQ_STK_SIZE_LIMIT          (APP_CFG_TASK_EQ_STK_SIZE    * (100u - APP_CFG_TASK_EQ_STK_SIZE_PCT_FULL   )) / 100u
#define  APP_CFG_TASK_OBJ_STK_SIZE_LIMIT         (APP_CFG_TASK_OBJ_STK_SIZE   * (100u - APP_CFG_TASK_EQ_STK_SIZE_PCT_FULL   )) / 100u


/*
*********************************************************************************************************
*                                          SERIAL CONFIGURATION
*********************************************************************************************************
*/

#define  BSP_CFG_SER_COMM_SEL                BSP_SER_COMM_UART_00


/*
*********************************************************************************************************
*                                       TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                            0u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                           1u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                            2u
#endif

#include  <stdio.h>
void  BSP_Ser_Printf (CPU_CHAR *p_fmt,
                      ...);
#define  APP_TRACE_LEVEL                            TRACE_LEVEL_DBG
#define  APP_CFG_TRACE                              BSP_Ser_Printf

#define  APP_TRACE_INFO(x)               ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_CFG_TRACE x) : (void)0u)
#define  APP_TRACE_DBG(x)                ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_CFG_TRACE x) : (void)0u)

#endif
