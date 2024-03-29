#ifndef SHP_ROUTER_CAN_CORE_H
#define SHP_ROUTER_CAN_CORE_H

#define canMsgId_GET_DEVICE_ID  0b00000000001
#define canMsgId_GET_DEVICE_CFG 0b00000000010
#define canMsgId_SET_DEVICE_ID  0b00000000100
#define canMsgId_ROUTER_DEVICE  0b00000001000


#define CAN_DEVICE_HW_ID_LEN 6

#define ROUTER_CAN_STD_PACKET_LEN 8
#define CAN_SEND_STRING_PACKET_DATA_LEN (ROUTER_CAN_STD_PACKET_LEN - 3)

#define ROUTER_CAN_CMD_NONE								 	    0
#define ROUTER_CAN_CMD_DEVICE_CFG						    1
#define ROUTER_CAN_CMD_PUBLISH_FROM_DEVICE	    2
#define ROUTER_CAN_CMD_ROUTE_TO_DEVICE	        3
#define ROUTER_CAN_CMD_DEVICE_FW_UPG_REQUEST	  4
#define ROUTER_CAN_CMD_DEVICE_FW_BLOCK_REQUEST	5

#define ROUTER_CAN_SPT_NONE								 	0
#define ROUTER_CAN_SPT_STRING2					    1
#define ROUTER_CAN_SPT_FW					          2


#endif

