/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : ETHIF_CS8900A.h                                         *
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
*   External interface & Module configuration header for ETHIF (Ethernet  *
*   Interface) module.                                                    *
*                                                                         *
**************************************************************************/


#ifndef __ETHIF_CS8900A_H__
#define __ETHIF_CS8900A_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"


/* ********************************************************************* */
/* Module configuration */

/* interface statistics gathering
 * such as collisions, dropped packets, missed packets
 * 0 = no statistics, minimal memory requirements, no overhead 
 * 1 = statistics on, but some have large granularity (0x200), very low overhead
 * 2 = statistics on, updated on every call to cs8900_service(), low overhead
 */
#define ETHIF_STATS 2


/* ********************************************************************* */
/* Interface macro & data definition */

struct ETHIF
{
  struct eth_addr *ethaddr;
  U8 needs_service;
  U8 use_polling;
#if (ETHIF_STATS > 0)
  U32 interrupts; // number of interrupt requests of cs8900
  U32 missed; // #packets on medium that could not enter cs8900a chip due to buffer shortage
  U32 dropped; // #packets dropped after they have been received in chip buffer
  U32 collisions; // #collisions on medium when transmitting packets 
  U32 sentpackets; // #number of sent packets
  U32 sentbytes; // #number of sent bytes
#endif

  /* Add whatever per-interface state that is needed here. */
};


/* ********************************************************************* */
/* Interface function definition */

err_t
ETHIF_Init(struct netif *netif);

void ETH_vWaitOnRxPacket(U32 wTimeOut);


/* ********************************************************************* */

#ifdef __cplusplus
}
#endif

#endif /*__ETHIF_CS8900A_H__*/
