// This file is copy from projb_current.pdf 
#ifndef OCTANEMESSAGE
#define OCTANEMESSAGE
#include <stdint.h>

#define ANY_PORT 0xFFFF
#define ANY_IP 0xFFFFFFFF

struct octane_control {
    uint8_t octane_action;
    uint8_t octane_flags;
    uint16_t octane_seqno;
    uint32_t octane_source_ip;
    uint32_t octane_dest_ip;
    uint16_t octane_source_port;
    uint16_t octane_dest_port;
    uint16_t octane_protocol;
    uint16_t octane_port;
};
#endif