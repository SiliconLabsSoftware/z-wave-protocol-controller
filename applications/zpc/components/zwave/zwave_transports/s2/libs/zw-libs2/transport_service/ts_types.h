/**
 * @file ts_types.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Type definitions for transport service.
 * Generic defines for the protocol types uint8_t, uint16_t etc.
 */
/****************************************************************************/
/*                      PUBLIC TYPES and DEFINITIONS                        */
/****************************************************************************/

#ifndef _TRANSPORT_SERVICE_TYPES_H_
#define _TRANSPORT_SERVICE_TYPES_H_
#ifdef _WIN32
#include <unistd.h>
#endif /* _WIN32 */
#include <stdint.h>
#include <ZW_typedefs.h>
#if !defined(ZIPGW)
#include <s2_protocol.h>
#endif
#include <ZW_classcmd.h>
#if defined(EFR32ZG) || defined(ZWAVE_ON_LINUX)
#define ZW_SLAVE_ENHANCED_232  // Defined to pull in ZW_SendDataEx from ZW_transport_api.h
#include <ZW_transport_api.h>
#endif

typedef unsigned long u32;
typedef unsigned short u16;

/* Transmit frame option flags */
#define TRANSMIT_OPTION_ACK 0x01 /* request acknowledge from destination node */
#define TRANSMIT_OPTION_LOW_POWER \
  0x02 /* transmit at low output power level (1/3 of normal RF range)*/
#define TRANSMIT_OPTION_AUTO_ROUTE \
  0x04 /* request retransmission via repeater nodes */
/* do not use response route - Even if available */
#define TRANSMIT_OPTION_NO_ROUTE 0x10
/* Use explore frame if needed */
#define TRANSMIT_OPTION_EXPLORE 0x20

/**
 * A response route is locked by the application
 */
#define RECEIVE_STATUS_ROUTED_BUSY 0x01
/**
 * Received at low output power level, this must
 * have the same value as TRANSMIT_OPTION_LOW_POWER
 */
#define RECEIVE_STATUS_LOW_POWER 0x02
/**
 * Mask for masking out the received frametype bits
 */
#define RECEIVE_STATUS_TYPE_MASK 0x0C
/**
 * Received frame is singlecast frame (rxOptions == xxxx00xx)
 */
#define RECEIVE_STATUS_TYPE_SINGLE 0x00
/**
 * Received frame is broadcast frame  (rxOptions == xxxx01xx)
 */
#define RECEIVE_STATUS_TYPE_BROAD 0x04
/**
 * Received frame is multicast frame (rxOptions == xxxx10xx)
 */
#define RECEIVE_STATUS_TYPE_MULTI 0x08
/**
 * Received frame is an explore frame (rxOptions == xxx1xxxx)
 * Only TYPE_BROAD can be active at the same time as TYPE_EXPLORE
 */
#define RECEIVE_STATUS_TYPE_EXPLORE 0x10
/**
 * Received frame is not send to me (rxOptions == x1xxxxxx)
 * - useful only in promiscuous mode
 */
#define RECEIVE_STATUS_FOREIGN_FRAME 0x40

#if !defined(ZIPGW)
/**
 * Structure describing how a package was received / should be transmitted
 */
typedef struct ts_param {
  /**
   * Source node
   */
  node_t snode;
  /**
   * Destination node
   */
  node_t dnode;

  /**
   * Source endpoint
   */
  uint8_t sendpoint;

  /**
   * Destination endpoint
   */
  uint8_t dendpoint;

  /**
   * Transmit flags
   * see txOptions in \ref ZW_SendData
   */
  uint8_t tx_flags;

  /**
   * Receive flags
   * see rxOptions in \ref ApplicationCommandHandler
   */
  uint8_t rx_flags;

  /**
   * Security scheme used for this package
   */
  uint8_t scheme;  // Security scheme
} ts_param_t;

#include <transport_service2_external.h>
#endif

/* Typedefs shared between ZIPGW and EFR32ZG*/
typedef void (*ZW_CommandHandler_Callback_t)(ts_param_t *p,
                                             ZW_APPLICATION_TX_BUFFER *pCmd,
                                             uint16_t cmdLength);
typedef void (*ZW_TransportService_SendData_Callback_t)(uint8_t txStatus,
                                                        void *t);

#if defined(EFR32ZG) || defined(ZWAVE_ON_LINUX)

extern uint16_t g_nodeID;

#define MyNodeID g_nodeID

bool TS_SendRaw(node_t dst,
                uint8_t *buf,
                uint8_t buflen,
                uint8_t txopt,
                VOID_CALLBACKFUNC(cb)(uint8_t status, TX_STATUS_TYPE *user));
#define TS_SEND_RAW(src, dst, buf, buflen, txopt, cb) \
  TS_SendRaw(dst, buf, buflen, txopt, cb)

#else
#define CALLBACKFUNC(func) func

#define ZIPCommandHandler(srcNode, count)                                     \
  if (TSApplicationCommandHandler) {                                          \
    ts_param_t p;                                                             \
    p.dendpoint = 0;                                                          \
    p.sendpoint = 0;                                                          \
    p.snode     = srcNode;                                                    \
    p.dnode     = rcb.cmn.p.dnode;                                            \
    p.rx_flags  = 0;                                                          \
    p.tx_flags  = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE            \
                 | TRANSMIT_OPTION_EXPLORE;                                   \
    p.scheme = 0xff;                                                          \
    TSApplicationCommandHandler(&p,                                           \
                                (ZW_APPLICATION_TX_BUFFER *)rcb.datagramData, \
                                count);                                       \
  }

extern uint8_t MyNodeID;
#if !defined(NEW_TEST_T2)
#define TS_SEND_RAW(src, dst, buf, buflen, txopt, cb) \
  ZW_SendData_Bridge(src, dst, buf, buflen, txopt, cb)
#endif
#endif

#if (defined(__i386__) || defined(__amd64__) || defined(__arm__) \
     || defined(__aarch64__))                                    \
  && !defined(EFR32ZG) && !defined(ZWAVE_ON_LINUX)
typedef void *TX_STATUS_TYPE;
bool TS_SEND_RAW(node_t snode,
                 node_t dnode,
                 uint8_t *cmd,
                 uint8_t len,
                 uint8_t flags,
                 void (*completedFunc)(uint8_t, TX_STATUS_TYPE *));
#endif

#endif /* _TRANSPORT_SERVICE_TYPES_H_ */
