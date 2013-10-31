/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : SYS_ARCH.c                                              *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : Any                                                     *
*                                                                         *
*   TOOL-CHAIN  : Any                                                     *
*                                                                         *
*   DESCRIPTION :                                                         *
*   sys_arch code file.                                                   *
*                                                                         *
**************************************************************************/

#include <stdlib.h>

#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

#include "arch/sys_arch.h"


const void * const pvNullPointer;

static OS_MEM *pQueueMem;
static char pcQueueMemoryPool[LWIP_MAX_QS * sizeof(TQ_DESCR) +3];

OS_STK sys_stack[LWIP_MAX_TASKS][LWIP_STACK_SIZE];
static INT8U sys_thread_no;

/*-----------------------------------------------------------------------------------*/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{

    /* XXX We ignore the size parameter. */
    INT8U       ucErr;
    PQ_DESCR    pQDesc;

    pQDesc = OSMemGet( pQueueMem, &ucErr );
    LWIP_ASSERT( "OSMemGet ", ucErr == OS_NO_ERR );
    if( ucErr == OS_NO_ERR )
    {
        pQDesc->pQ = OSQCreate(pQDesc->pvQEntries, LWIP_Q_SIZE);
        LWIP_ASSERT( "OSQCreate ", pQDesc->pQ != NULL );
        if( pQDesc->pQ != NULL )
        {
            *mbox = pQDesc;
            return ERR_OK;
        }
    }
    return ERR_MEM;
}

/*-----------------------------------------------------------------------------------*/
void sys_mbox_free(sys_mbox_t *mbox)
{
    INT8U     ucErr;
    
    LWIP_ASSERT( "sys_mbox_free ", mbox != NULL);   
    LWIP_ASSERT( "sys_mbox_free ", *mbox != NULL);   
    OSQFlush( (*mbox)->pQ );

    OSQDel( (*mbox)->pQ, OS_DEL_NO_PEND, &ucErr);
    LWIP_ASSERT( "OSQDel ", ucErr == OS_NO_ERR);         

    ucErr = OSMemPut( pQueueMem, *mbox);
    LWIP_ASSERT( "OSMemPut ", ucErr == OS_NO_ERR );
}

/*-----------------------------------------------------------------------------------*/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{

    INT8U status;

    if( !msg )
        msg = (void*)&pvNullPointer;


    status = OSQPost((*mbox)->pQ, msg);

    /* TODO: what if it is full ? */
    LWIP_ASSERT("OSQPost", status == OS_NO_ERR );
}


err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{

    INT8U status;

    if( !msg )
        msg = (void*)&pvNullPointer;


    status = OSQPost((*mbox)->pQ, msg);

    if(status == OS_Q_FULL)
        return ERR_MEM;

    LWIP_ASSERT("OSQPost", status == OS_NO_ERR );
    return ERR_OK; 
}

/*-----------------------------------------------------------------------------------*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    INT8U ucErr;
    INT32U ucos_timeout;
    void *temp;

    /* convert LwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout)
    {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;

        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    }
    else
    {
        ucos_timeout = 0;
    }

    temp = OSQPend( (*mbox)->pQ, ucos_timeout, &ucErr );

    if(msg)
    {
        if( temp == (void*)&pvNullPointer )
        {
            *msg = NULL;
        }
        else
        {
            *msg = temp;
        }
    }
    
    if( ucErr == OS_TIMEOUT )
    {
        timeout = SYS_ARCH_TIMEOUT;
    }
    else
    {
        LWIP_ASSERT( "OSQPend ", ucErr == OS_NO_ERR );
        /* Calculate time we waited for the message to arrive. */      
        /* XXX: we cheat and just pretend that we waited for long! */
        timeout = 1;
    }

    return timeout;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    INT8U ucErr;
    void *temp;

    temp = OSQAccept((*mbox)->pQ, &ucErr);

    if(temp == NULL || ucErr == OS_Q_EMPTY)
        return SYS_MBOX_EMPTY;

    if(msg)
    {
        if( temp == (void*)&pvNullPointer )
        {
            *msg = NULL;
        }
        else
        {
            *msg = temp;
        }
    }
    
    return 0;
}

/*-----------------------------------------------------------------------------------*/
err_t sys_sem_new(sys_sem_t *pSem, u8_t count)
{
    *pSem = OSSemCreate( (INT16U)count );
    LWIP_ASSERT( "OSSemCreate ", *pSem != NULL );
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    INT8U  ucErr;
    INT32U ucos_timeout;

    /* convert LwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout)
    {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;

        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    }
    else
    {
        ucos_timeout = 0;
    }

    OSSemPend( *sem, ucos_timeout, &ucErr );        
    if( ucErr == OS_TIMEOUT )
    {
        timeout = SYS_ARCH_TIMEOUT;
    }
    else
    {
        //Semaphore is used for pbuf_free, which could get called from an ISR
        //LWIP_ASSERT( "OSSemPend ", ucErr == OS_NO_ERR );

        /* Calculate time we waited for the message to arrive. */      
        /* XXX: we cheat and just pretend that we waited for long! */
        timeout = 1;
    }
    return timeout;
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_signal(sys_sem_t *sem)
{
    INT8U     ucErr;
    ucErr = OSSemPost( *sem );
 
    /* It may happen that a connection is already reset and the semaphore is deleted
       if this function is called. Therefore ASSERTION should not be called */   
    //LWIP_ASSERT( "OSSemPost ", ucErr == OS_NO_ERR );
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_free(sys_sem_t *sem)
{
    INT8U     ucErr;
    
    (void)OSSemDel( *sem, OS_DEL_NO_PEND, &ucErr );
    
    LWIP_ASSERT( "OSSemDel ", ucErr == OS_NO_ERR );
}
/*-----------------------------------------------------------------------------------*/
void sys_init(void)
{
    INT8U   ucErr;

    pQueueMem = OSMemCreate( (void*)((u32_t)((u32_t)pcQueueMemoryPool+3) & ~3), LWIP_MAX_QS, sizeof(TQ_DESCR), &ucErr );
    LWIP_ASSERT( "OSMemCreate ", ucErr == OS_NO_ERR );

    sys_thread_no = 0;
}

/*-----------------------------------------------------------------------------------*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio) 
{

    /* XXX Handle same task priorities. */
    INT8U bTemp;

    LWIP_ASSERT("sys_thread_new: Max Sys. Tasks reached.", sys_thread_no < LWIP_MAX_TASKS);

    ++sys_thread_no; /* next task created will be one lower to this one */

#if (STACK_PROFILE_EN == 1)
    if(bTemp = OSTaskCreateExt( function, arg, &sys_stack[sys_thread_no - 1][LWIP_STACK_SIZE - 1], prio, sys_thread_no - 1,
       &sys_stack[sys_thread_no - 1][0], LWIP_STACK_SIZE, (void *)0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR))
#else
    if((bTemp = OSTaskCreate( function, arg, &sys_stack[sys_thread_no - 1][LWIP_STACK_SIZE - 1], prio )))
#endif
    {

        printf("sys_thread_new: Task creation error (prio=%d) [%d]\n",prio,bTemp);
        --sys_thread_no;
        panic("lwip thread creation failed.");
    }
    return sys_thread_no;
}

u32_t sys_now(void)
{
    return (1000/OS_TICKS_PER_SEC)*OSTimeGet();
}
