/* Copyright 2008 SMSC, All rights reserved
FILE: icmp_echo_tester.h
*/

#ifndef ICMP_ECHO_TESTER_H
#define ICMP_ECHO_TESTER_H

#include "smsc_environment.h"

void IcmpEchoTester_Initialize(IPV4_ADDRESS destination, u32_t size, u8_t timeOut);

#endif /*ICMP_ECHO_TESTER_H */
