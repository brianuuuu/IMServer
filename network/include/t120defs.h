/*------------------------------------------------------------------------*/
/*                                                                        */
/*  T122/125 implementation                                               */
/*  T120DEFS.H                                                            */
/*                                                                        */
/*  t120 common definitions                                               */
/*                                                                        */
/*  All rights reserved                                                   */
/*                                                                        */
/*------------------------------------------------------------------------*/

#ifndef __T120DEFS_H__
#define __T120DEFS_H__

#include "platform.h"

#define INVALID_CONNECTIONID        0
#define INVALID_USERID              0
#define INVALID_USER_HANDLE         0
#define INVALID_CHANNELID           0
#define INVALID_TOKENID				0

#define MASK_MCU_NODE_ID           0xff000000
#define MASK_TERMINAL_NODE_ID      0xfffff000

#define MAX_MCS_USER_INDEX         0xfff
#define MAX_MCS_MCU_INDEX          0xff
#define MAX_MCS_TERMINAL_INDEX     0xfff

#define IS_CHANNEL_ID(x)      ((x&0xffff0000)==0x0)

#define IS_LOCAL_MCS_USER(node_id, user_id) \
	((user_id&0xfffff000)==node_id)

#define IS_LOCAL_MCS_TERMINAL(mcu_id, node_id) \
	((node_id&0xfff000)!=0 && (node_id&0xff000000)==mcu_id)

#define IS_MCS_MCU_NODE(x) \
	((x&0xffffff)==0x0 && (x&0xff000000)!=0x0)

#define IS_MCS_TERMINAL_NODE(x) \
	((x&0xfff)==0x0 && (x&0xff000000)!=0x0 && (x&0xfff000)!=0x0)


typedef char* Transport_Address;

#endif // __T120DEFS_H__

