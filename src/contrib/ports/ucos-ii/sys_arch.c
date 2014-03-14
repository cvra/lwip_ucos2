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



OS_STK sys_stack[LWIP_MAX_TASKS][LWIP_STACK_SIZE];
static INT8U sys_thread_no;

/*-----------------------------------------------------------------------------------*/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    mbox->pQ = OSQCreate(mbox->pvQEntries, LWIP_Q_SIZE);
    LWIP_ASSERT("OSQCreate", mbox->pQ != NULL );
    mbox->Q_full = OSSemCreate(LWIP_Q_SIZE);
    LWIP_ASSERT("OSSemCreate", mbox->Q_full != NULL );

    mbox->is_valid = 1;
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
void sys_mbox_free(sys_mbox_t *mbox)
{
    INT8U     ucErr;
    
    LWIP_ASSERT("sys_mbox_free", mbox != NULL);   
    OSQFlush(mbox->pQ);

    OSQDel(mbox->pQ, OS_DEL_NO_PEND, &ucErr);
    LWIP_ASSERT("OSQDel", ucErr == OS_NO_ERR);

    OSSemDel(mbox->Q_full, OS_DEL_NO_PEND, &ucErr);
    LWIP_ASSERT("OSSemDel", ucErr == OS_NO_ERR);
}

/*-----------------------------------------------------------------------------------*/
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    INT8U status;

    /* Wait for an available slot in the queue. */
    OSSemPend(mbox->Q_full, 0, &status);

    /* Posts the message to the queue. */
    status = OSQPost(mbox->pQ, msg);
    LWIP_ASSERT("OSQPost", status == OS_NO_ERR);
}


err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    INT8U status;

    if(OSSemAccept(mbox->Q_full)) {
        status = OSQPost(mbox->pQ, msg);
        LWIP_ASSERT("OSQPost", status == OS_NO_ERR);
    } else {
        return ERR_MEM;
    }

    return ERR_OK; 
}

/*-----------------------------------------------------------------------------------*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    INT8U ucErr;
    INT32U ucos_timeout;
    void *temp;

    /* convert LwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout) {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;
        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    } else {
        ucos_timeout = 0;
    }

    temp = OSQPend(mbox->pQ, ucos_timeout, &ucErr);

    
    if(ucErr == OS_TIMEOUT) {
        timeout = SYS_ARCH_TIMEOUT;
    } else {
        LWIP_ASSERT("OSQPend ", ucErr == OS_NO_ERR);

        /* Tells tasks waiting because of a full buffer that the buffer is not full
         * anymore. */
        OSSemPost(mbox->Q_full);

        /* If there is a destination pointer, store the message in it. */
        if(msg) {
            *msg = temp;
        }

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

    temp = OSQAccept(mbox->pQ, &ucErr);

    if(temp == NULL && ucErr == OS_Q_EMPTY)
        return SYS_MBOX_EMPTY;
    
    /* Tells tasks waiting because of a full buffer that the buffer is not full
     * anymore. */
    OSSemPost(mbox->Q_full);

    *msg = temp;
    
    return 0;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    LWIP_ASSERT("sys_mbox_valid", mbox != NULL);
    return mbox->is_valid;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    LWIP_ASSERT("sys_mbox_valid", mbox != NULL);
    mbox->is_valid = 0;
}

/*-----------------------------------------------------------------------------------*/
err_t sys_sem_new(sys_sem_t *pSem, u8_t count)
{
    pSem->sem = OSSemCreate( (INT16U)count );
    LWIP_ASSERT( "OSSemCreate ", pSem->sem != NULL );
    pSem->is_valid = 1;
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    INT8U  ucErr;
    INT32U ucos_timeout;

    /* Convert lwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout) {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;
        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    } else {
        ucos_timeout = 0;
    }

    OSSemPend(sem->sem, ucos_timeout, &ucErr );        
    if(ucErr == OS_TIMEOUT) {
        timeout = SYS_ARCH_TIMEOUT;
    } else {
        /* Calculate time we waited for the message to arrive. */      
        /* TODO: we cheat and just pretend that we waited for long! */
        timeout = 1;
    }
    return timeout;
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_signal(sys_sem_t *sem) 
{
    OSSemPost(sem->sem); 
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_free (sys_sem_t *sem)
{
    INT8U     ucErr;
    
    OSSemDel(sem->sem, OS_DEL_NO_PEND, &ucErr); 
    LWIP_ASSERT("OSSemDel ", ucErr == OS_NO_ERR);
}

int sys_sem_valid(sys_sem_t *sem)
{
    return sem->is_valid;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    sem->is_valid = 0;
}


/*-----------------------------------------------------------------------------------*/
void sys_init(void)
{
    sys_thread_no = 0;
}

/*-----------------------------------------------------------------------------------*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio) 
{
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

u32_t sys_jiffies(void) { 
    return OSTimeGet();
}
