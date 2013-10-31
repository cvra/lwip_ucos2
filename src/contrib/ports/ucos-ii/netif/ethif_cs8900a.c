/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : ETHIF_CS8900A.c                                         *
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
*   Ethernet (CS8900) interface layer for Lwip.                           *
*                                                                         *
**************************************************************************/

#include "frmwrk_cfg.h"
#if (INCLUDE_ETHERNET_CS8900A == 1)

#include "defs.h"
#include "frmwrk.h"
#include "consol.h"
#include "eventlog.h"
#include "eth_cs8900a.h"

#include "lwip/debug.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "netif/etharp.h"

#include "ethif_cs8900a.h"

#if LWIP_SNMP > 0
#  include "snmp.h"
#endif


/* ********************************************************************* */
/* Global definitions */


/* ********************************************************************* */
/* File local definitions */

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

// network interface state
struct netif *ETHIF_netif;

sys_mbox_t mboxRxQ;


/* ********************************************************************* */
/* Local functions */

static err_t ETH_output(struct netif *netif, struct pbuf *p)
{
        // q traverses through linked list of pbuf's
        struct pbuf *q;
        int i;
        
        EVENT_LOG(EV_CODE_TXR_BEG, 0, 0, 0);
        
        if(!ETH_CS8900A_wWriteStart(p->tot_len))
        {
                //printf("CS8900A driver error\n");
                return 0 /* error */;
        }
        
        for(q = p; q != NULL; q = q->next)
        {
#if (DEBUG_ETH == 1)
                CONSOL_Printf("NETIF: Txed\n");
                CONSOL_vPrintData((U8 *)q->payload, q->len, 0);
#endif
                
                ETH_CS8900A_wWriteData((U8 *)q->payload, q->len);
                
#if (ETHIF_STATS > 0)
                ((struct ETHIF *)netif->state)->sentbytes += q->len;
#endif
                
#if LWIP_SNMP > 0
                snmp_add_ifoutoctets(p->tot_len);
#endif
                
#if (ETHIF_STATS > 0)
                ((struct ETHIF *)netif->state)->sentpackets++;
#endif
        }
        
        ETH_CS8900A_wWriteEnd();
        
#if 0 /* mike TBI, not ready to transmit */
        {
                // { not ready to transmit!? }
#if LWIP_SNMP > 0
                snmp_inc_ifoutdiscards();
#endif
        }
#endif
        
        EVENT_LOG(EV_CODE_TXR_END, 0, 0, 0);
        
        return ERR_OK;
}

/**
* Writing an IP packet (to be transmitted) to the CS8900.
*
* Before writing a frame to the CS8900, the ARP module is asked to resolve the
* Ethernet MAC address. The ARP module might undertake actions to resolve the
* address first, and queue this packet for later transmission.
*
* @param netif The lwIP network interface data structure belonging to this device.
* @param p pbuf to be transmitted (or the first pbuf of a chained list of pbufs).
* @param ipaddr destination IP address.
*
* @internal It uses the function cs8900_input() that should handle the actual
* reception of bytes from the network interface.
*
*/
static err_t ETHIF_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
        struct ETHIF *ETHIF = netif->state;
        
        p = etharp_output(netif, ipaddr, p);
        /* network hardware address obtained? */
        if (p != NULL)
        {
                /* send out the packet */
                ETH_output(netif, p);
                p = NULL;
        }
        else
        {
                /* we cannot tell if the packet was sent, the packet could have been queued */
                /* on an ARP entry that was already pending. */
        }
        
        return ERR_OK;
}

/**
* Move a received packet from the cs8900 into a new pbuf.
*
* Must be called after reading an ISQ event containing the
* "Receiver Event" register, before reading new ISQ events.
*
* This function copies a frame from the CS8900A.
* It is designed failsafe:
* - It does not assume a frame is actually present.
* - It checks for non-zero length
* - It does not overflow the frame buffer
*/
static struct pbuf *ETH_Input(U16 len)
{
        struct netif *netif = ETHIF_netif;
        struct pbuf *p = NULL, *q = NULL;
        U16 i;
        U8 *ptr = NULL;
        
        EVENT_LOG(EV_CODE_RXR_BEG, 0, 0, 0);
        
        /* mike TBI */
#if LWIP_SNMP > 0
    // update number of received MAC-unicast and non-MAC-unicast packets
    if (event_type & 0x0400U/*Individual*/)
    {
                snmp_inc_ifinucastpkts();
    }
    else
    {
                snmp_inc_ifinnucastpkts();
    }
#endif
        
        // read RxLength
    LWIP_DEBUGF(NETIF_DEBUG, ("ETHIF_rawInput: packet len %u\n", len));
#if LWIP_SNMP > 0    
    snmp_add_ifinoctets(len);
#endif
        
        // positive length?
        if (len > 0)
        {
                // allocate a pbuf chain with total length 'len' 
                p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
                if (p != 0)
                {
                        ETH_CS8900A_wRxCopyStart();

                        for (q = p; q != 0; q = q->next)
                        {
                                LWIP_DEBUGF(DEBUG_ETH, ("ETHIF_rawInput: pbuf @%p len %u\n", q, q->len));

                ETH_CS8900A_wRxCopy(q->payload, q->len);
                        }

                        ETH_CS8900A_wRxCopyEnd();
                }
                else
                { // could not allocate a pbuf
                        // skip received frame
            ETH_CS8900A_vSkipRxFrame();

                        // TODO: maybe do not skip the frame at this point in time?
#if (ETHIF_STATS > 0)
                        ((struct ETHIF *)netif->state)->dropped++;
#endif
#if LWIP_SNMP > 0    
                        snmp_inc_ifindiscards();
#endif
                        /* --------------------------------------------------------------------------------------- Add stats >>>>>>>>>>>>>>>>>>>>>>>>>>>> no pbuf */
                        return 0;
                }
        }
        // length was zero
        else
        {
                /* --------------------------------------------------------------------------------------- Add stats >>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
        }
        
        EVENT_LOG(EV_CODE_RXR_END, 0, 0, 0);
        
        return p;
}

/**
* Read a received packet from the CS8900.
*
* This function should be called when a packet is received by the CS8900
* and is fully available to read. It moves the received packet to a pbuf
* which is forwarded to the IP network layer or ARP module. It transmits
* a resulting ARP reply or queued packet.
*
* @param netif The lwIP network interface to read from.
*
* @internal Uses cs8900_input() to move the packet from the CS8900 to a
* newly allocated pbuf.
*
*/
static void ETHIF_input(struct pbuf *p)
{
        struct netif *netif = ETHIF_netif;
        struct ETHIF *ETHIF = netif->state;
        struct eth_hdr *ethhdr = NULL;
#ifdef OLD_LWIP
        struct pbuf *q = NULL;
#endif
        
#if (DEBUG_ETH == 1)
        CONSOL_Printf("NETIF: Rxed\n");
        //   CONSOL_vPrintData(pbData, wLen, 0);
#endif
        
        /* no packet could be read */
        if(p == NULL)
        {
                /* silently ignore this */
                SEND_STR("e1\n");
                return;
        }
        
#ifdef _dump1   
        pb = p->payload;
        for(i=0; i<wLen; i++)
        {
                printf(" %x",pb[i]);
        }
        printf("\n");
#endif
        
        /* points to packet payload, which starts with an Ethernet header */
        ethhdr = p->payload;
        
        switch(htons(ethhdr->type))
        {
        case ETHTYPE_IP:        /* IP packet? */
                LWIP_DEBUGF(DEBUG_ETH, ("Rx: IP(%d)\n",wLen));
                
                etharp_ip_input(netif, p); /* update ARP table, obtain first queued packet */
                pbuf_header(p, -14); /* skip Ethernet header */
                netif->input(p, netif); /* pass to network layer */
                break;
                
        case ETHTYPE_ARP:       /* ARP packet? */
                LWIP_DEBUGF(DEBUG_ETH, ("Rx: ARP(%d)\n",wLen));
                
                /* pass p to ARP module, get ARP reply or ARP queued packet */
                etharp_arp_input(netif, ((struct ETHIF *)netif->state)->ethaddr, p);
                break;
                
        default:                        /* unsupported Ethernet packet type */
                pbuf_free(p);
                break;
        }
        
#ifdef OLD_LWIP
        /* send out the ARP reply or ARP queued packet */
        if (q != NULL)
        {
                /* q pbuf has been succesfully sent? */
                if (ETH_output(netif, q) == ERR_OK)
                {
                        pbuf_free(q);
                }
                else
                {
                        /* TODO: re-queue packet in the ARP cache here (?) */
                        pbuf_free(q);
                }
        }
#endif
}


/* ********************************************************************* */
/* Global functions */

/*-----------------------------------------------------------------------------------*/
/*
* ETHIF_vInit():
*
* Should be called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
*/
/*-----------------------------------------------------------------------------------*/

err_t
ETHIF_Init(struct netif *netif)
{
        struct ETHIF *ETHIF;
        
        ETHIF = mem_malloc(sizeof(struct ETHIF));
        if (ETHIF == NULL) return ERR_MEM;
        
        netif->name[0] = IFNAME0;
        netif->name[1] = IFNAME1;
        netif->output = ETHIF_output;
        netif->linkoutput = ETH_output;
        
        // initialize ETH specific interface structure
        netif->state = ETHIF;
        
#if 0
        /* maximum transfer unit */
        netif->mtu = 1500;
        
        /* broadcast capability */
        netif->flags = NETIF_FLAG_BROADCAST;
#endif
        
        /* hardware address length */
        netif->hwaddr_len = 6;
        
        /* mike not needed*/ ETHIF->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
        // initially assume no ISQ event
        ETHIF->needs_service = 0;
        // set to 1 if polling method is used
        ETHIF->use_polling = 0;
        
#if (ETHIF_STATS > 0)
        // number of interrupts (vector calls)
        ETHIF->interrupts = 0;
        ETHIF->missed = 0;
        ETHIF->dropped = 0;
        ETHIF->sentpackets = 0;
        ETHIF->sentbytes = 0;
#endif
        
        /* Copy MAC address */
        ETH_CS8900A_vGetMACAddress((U8 *)ETHIF->ethaddr);

        mboxRxQ = sys_mbox_new();
        
        // TODO: remove this hack
        ETHIF_netif = netif;
        
        etharp_init();

        CONSOL_SendString("ETHIF init\n");

        return ERR_OK;
}

void ETH_vWaitOnRxPacket(U32 wTimeOut)
{
    struct pbuf *pstPBuf;
        U16 hwLen;

        if((hwLen = ETH_CS8900A_hwPoll()))
        {
        pstPBuf = ETH_Input(hwLen);
                ETHIF_input(pstPBuf);
        }
}

#endif /* (INCLUDE_ETHERNET_CS8900A == 1) */


/* ********************************************************************* */
