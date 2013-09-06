/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/*****************************************************************************
* Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
*   All Rights Reserved.
*   The modifications to this program code are proprietary to SMSC and may not be copied,
*   distributed, or used without a license to do so.  Such license may have
*   Limited or Restricted Rights. Please refer to the license for further
*   clarification.
******************************************************************************
* FILE: ip.h
* DESCRIPTION:
*    Creates an abstraction around the IP Layer so that 
*       protocols such as TCP and UDP don't need to know
*       if they are using IPv4, IPv6 or both.
*    Designed to allow both IPv4 and IPv6 to co exist. 
*       When they are both used at the same time an
*       additional byte is used in the IP_ADDRESS for 
*       an address type indicator.
*****************************************************************************/

#ifndef IP_H
#define IP_H

#include "smsc_environment.h"

/*****************************************************************************
* The functions/macros described below may be implemented differently
* depending on which IP protocols are implemented. When both IPv4 and IPv6
* are enabled then these functions/macros should be implemented in a way that
* allows the coexistance of both. This is possible when you consider joining
* the IPv4, and IPv6 address space into a larger address space. Users of the
* IP layer could then use either address just as easily.
*****************************************************************************/

/*****************************************************************************
* TYPE: IP_ADDRESS, PIP_ADDRESS
* DESCRIPTION:
*    IP_ADDRESS represents a literal address, IPV4, or IPV6
*    PIP_ADDRESS represents a pointer to an IP_ADDRESS
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: Ip_GetSourceAddress
* DESCRIPTION:
*    This function is used to get the address of the interface with which
*    a packet using a particular destination address will depart from.
* PARAMETERS:
*    PIP_ADDRESS pIpSourceAddress, This is a pointer to the IP_ADDRESS where
*        the source address will be stored.
*    PIP_ADDRESS pIpDestinationAddress, This is a pointer to the IP_ADDRESS
*        where the destination address is stored.
* RETURN VALUE:
*    None
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: Ip_GetMaximumTransferUnit
* DESCRIPTION:
*    This function is used to get the maximum transfer unit(MTU) of the
*    interface with which a packet using a particular destination address
*    will depart from.
* PARAMETERS:
*    PIP_ADDRESS pDestinationAddress, This is a pointer to the IP_ADDRESS
*        where the destination address is stored.
* RETURN VALUE:
*    returns the MTU as a u16_t
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: Ip_AllocatePacket
* DESCRIPTION:
*    This function is used to allocate a series of packet buffers whos
*    active regions are ready for IP payload, and their sizes add up to
*    the size requested. If the user would like to make sure this function
*    does not return a series of packet buffers then the user must use
*    Ip_GetMaximumTransferUnit and make sure that the size of the requested
*    packet is less than or equal to the MTU returned by 
*    Ip_GetMaximumTransferUnit.
* PARAMETERS:
*    PIP_ADDRESS pDestinationAddress, this is a pointer to the IP_ADDRESS
*        where the destination address is stored.
*    u32_t size, This is the requested size of payload space to be provided
*        in the returned packet buffers.
* RETURN VALUE:
*    returns a PPACKET_BUFFER. If NULL then the packet buffer could not be
*    allocated. If not NULL, then the returned pointer points to one or more
*    packet buffers, whos active region sizes add up to the size requested.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: Ip_PseudoCheckSum
* DESCRIPTION:
*    This function is used to calculate pseudo check sum, as specified by
*    RFC 768, RFC 793 for UDP and TCP over IPv4
*    RFC 2460 for UDP and TCP over IPv6
*    It calculates the check some over the IP payload plus the pseudo header
*    defined in those RFCs
* PARAMETERS:
*    PPACKET_BUFFER packet, This is the packet buffer list for which the
*        check sum is calculated.
*    PIP_ADDRESS pSourceAddress, A pointer to the IP_ADDRESS that contains
*        the source address used for pseudo header check sum calculation.
*    PIP_ADDRESS pDestinationAddress, A pointer to the IP_ADDRESS that contains
*        the destination address used for pseudo header check sum calculation.
*    u8_t protocol, this is the protocol identifier used for pseudo header
*        check sum calculation.
*    u16_t length, this is the length field used for pseudo header check
*        sum calculation.
* RETURN VALUE:
*    the return value is a u16_t that represents the pseudo check sum.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: Ip_Output
* DESCRIPTION:
*    This function is used by the UDP, and TCP layers to send packets over
*    the IP network. If you are implementing another protocol you may also
*    need to use this function. Ownership of the packet buffer is assumed to
*    be transfered to the IP layer by calling this function. If the caller
*    needs to make sure that the packet buffer is not freed then the caller
*    should call PacketBuffer_IncreaseReference on this packet before sending
*    it to Ip_Output.
* PARAMETERS:
*    PPACKET_BUFFER packet, This is the packet buffer list to send over
*        the IP network.
*    PIP_ADDRESS pSourceAddress, This is a pointer to the IP_ADDRESS which
*        holds the source address to use for the the IP header.
*    PIP_ADDRESS pDestinationAddress, This is a pointer to the IP_ADDRESS
*        which holds the destination address to use for the IP header.
*    u8_t protocol, This is the protocol code to use for the IP header.
* RETURN VALUE
*    None.
*****************************************************************************/

/*****************************************************************************
* CONSTANT: IP_ADDRESS_ANY
* DESCRIPTION: This is intended to represent any IP address
*****************************************************************************/

/*****************************************************************************
* CONSTANT: IP_ADDRESS_STRING_SIZE
* DESCRIPTION:
*    This is intended to represent the maximum number of characters
*    needed for representing an IP_ADDRESS in string form. It includes the
*    zero terminating character. This is intended to work with
*    IP_ADDRESS_TO_STRING as its caller is required to provided the data
*    space where the string will be stored.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_TO_STRING
* DESCRIPTION:
*    This function converts an IP_ADDRESS into its string representation
* PARAMETERS:
*    char * stringDataSpace, This is a pointer to the memory space where
*        the string will be stored. The memory space must be at least as
*        large as IP_ADDRESS_STRING_SIZE
*    PIP_ADDRESS pIpAddress, This is a pointer to the IP_ADDRESS that is
*        to be converted to string form.
* RETURN VALUE:
*    Returns stringDataSpace.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_IS_ANY
* DESCRIPTION: 
*    This function determines if a particular address is the ANY address.
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to be tested
* RETURN VALUE:
*    returns zero if the IP_ADDRESS is not the ANY address
*    returns non zero if the IP_ADDRESS is the ANY address
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_IS_BROADCAST
* DESCRIPTION:
*    This function determines if a particular address is a broadcast address.
* PARAMETERS: 
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to be tested
*    struct NETWORK_INTERFACE * pNetworkInterface, this is a pointer to
*        the network interface, of which the provided IP_ADDRESS is tested
*        as a broadcast address. This is necessary because there is a 
*        notion of local broadcast, which means it is only a broadcast
*        with in a local subnet.
* RETURN VALUE:
*    returns zero if the IP_ADDRESS is not a broadcast address.
*    returns non zero if the IP_ADDRESS is a broadcast address.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_COMPARE
* DESCRIPTION:
*    Tests if two addresses are identical
* PARAMETERS:
*    PIP_ADDRESS pIpAddress1, This is a pointer to the first IP_ADDRESS
*    PIP_ADDRESS pIpAddress2, This is a pointer to the second IP_ADDRESS
* RETURN VALUE:
*    returns zero if the first and second addresses do not match
*    returns non zero if the first and second addresses do match
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_NETWORK_COMPARE
* DESCRIPTION:
*    Tests if two addresses are on the same subnet
* PARAMETERS:
*    IP_ADDRESS ipAddress1, the first address to compare
*    IP_ADDRESS ipAddress2, the second address to compare
*    IP_ADDRESS netMask, the subnet mask to qualify against
* RETURN VALUE:
*    returns zero if the first and second addresses are not on the same subnet
*    returns non zero if the first and second addresses are on the same subnet
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_IS_MULTICAST
* DESCRIPTION:
*    Tests if an address is a multicast address
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to test
* RETURN VALUE:
*    returns zero if the address is not a multicast address.
*    returns non zero if the address is a multicast address.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_COPY
* DESCRIPTION:
*    Copies an IP_ADDRESS into the space of another IP_ADDRESS
* PARAMETERS:
*    PIP_ADDRESS pIpAddressDestination, this is a pointer to the IP_ADDRESS 
*        where to source address should be copied into.
*    PIP_ADDRESS pIpAddressSource, this is a pointer to the IP_ADDRESS
*        which is copied into the destination address.
* RETURN VALUE:
*    None.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_GET_TYPE
* DESCRIPTION:
*    Gets the type of the provided IP_ADDRESS. The type could be IPv4 or IPv6
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to be tested
* RETURN VALUE:
*    returns IP_ADDRESS_TYPE_IPV4 if the type of the address is IPv4
*    returns IP_ADDRESS_TYPE_IPV6 if the type of the address is IPv6
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_IS_IPV4
* DESCRIPTION:
*    tests if an IP_ADDRESS is an IPv4 address
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to be tested
* RETURN VALUE:
*    returns zero if the IP_ADDRESS is not an IPv4 address.
*    returns non zero if the IP_ADDRESS is an IPv4 address.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_IS_IPV6
* DESCRIPTION:
*    tests if an IP_ADDRESS is an IPv6 address
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS to be tested
* RETURN VALUE:
*    returns zero if the IP_ADDRESS is not an IPv6 address.
*    returns non zero if the IP_ADDRESS is an IPv6 address.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_SET_IPV4
* DESCRIPTION:
*    Sets an IPv4 address by providing the 4 data bytes of an IPv4 address
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS where
*        to set the address.
*    u8_t byte1, this is the byte to set to the most significant byte of the
*        IP_ADDRESS. 
*    u8_t byte2, this is the byte to set to the second most significant byte
*        of the IP_ADDRESS.
*    u8_t byte3, this is the byte to set to the second least significant byte
*        of the IP_ADDRESS.
*    u8_t byte4, this is the byte to set to the least significant byte of the
*        IP_ADDRESS.
* RETURN VALUE:
*    None.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IP_ADDRESS_COPY_FROM_IPV4_ADDRESS
* DESCRIPTION:
*    Copies an IPV4_ADDRESS to an IP_ADDRESS
* PARAMETERS:
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS where 
*        the IPV4_ADDRESS is copied to.
*    PIPV4_ADDRESS pIpv4Address, this is a pointer to the IPV4_ADDRESS where
*        the data is copied from.
* RETURN VALUE:
*    None.
*****************************************************************************/

/*****************************************************************************
* FUNCTION/MACRO: IPV4_ADDRESS_COPY_FROM_IP_ADDRESS
* DESCRIPTION:
*    Copies an IP_ADDRESS to an IPV4_ADDRESS. If the IP_ADDRESS is not
*    an IPV4_ADDRESS then an error is generated.
* PARAMETERS:
*    PIPV4_ADDRESS pIpv4Address, this is a pointer to the IPV4_ADDRESS where
*        the IP_ADDRESS is copied to.
*    PIP_ADDRESS pIpAddress, this is a pointer to the IP_ADDRESS where
*        the data is copied from.
* RETURN VALUE:
*    None.
*****************************************************************************/

#define IP_ADDRESS_TYPE_IPV4	(1)
#define IP_ADDRESS_TYPE_IPV6	(2)

#if (IPV4_ENABLED && IPV6_ENABLED)
#error IPv4 and IPv6 co existence is not implemented
#elif IPV4_ENABLED

#include "ipv4.h"

/* only IPv4 is enabled 
Just remap IP neutral functions to IPv4 versions*/

#define IP_ADDRESS IPV4_ADDRESS
#define PIP_ADDRESS PIPV4_ADDRESS

#define IP_ADDRESS_STRING_SIZE	IPV4_ADDRESS_STRING_SIZE

#define Ip_GetSourceAddress(pIpSourceAddress,pIpDestinationAddress)	\
	(*(pIpSourceAddress))=Ipv4_GetSourceAddress(*(pIpDestinationAddress))
#define Ip_GetMaximumTransferUnit(pDestinationAddress)	\
	Ipv4_GetMaximumTransferUnit(*(pDestinationAddress))
#define Ip_AllocatePacket(pDestinationAddress,size)			\
		Ipv4_AllocatePacket(pDestinationAddress,size)
#define Ip_PseudoCheckSum(packet,pSourceAddress,pDestinationAddress,protocol,length)	\
	Ipv4_PseudoCheckSum(packet,pSourceAddress,pDestinationAddress,protocol,length)
#define Ip_Output(packet,pSourceAddress,pDestinationAddress,protocol)	\
	Ipv4_Output(packet,*(pSourceAddress),*(pDestinationAddress),protocol)
#define IP_ADDRESS_ANY											IPV4_ADDRESS_ANY
#define IP_ADDRESS_TO_STRING(stringDataSpace,pIpAddress)		IPV4_ADDRESS_TO_STRING(stringDataSpace,*(pIpAddress))
#define IP_ADDRESS_IS_ANY(pIpAddress)							IPV4_ADDRESS_IS_ANY(pIpAddress)
#define IP_ADDRESS_IS_BROADCAST(pIpAddress,pNetworkInterface)	IPV4_ADDRESS_IS_BROADCAST(*(pIpAddress),pNetworkInterface)
#define IP_ADDRESS_COMPARE(pIpAddress1,pIpAddress2)				IPV4_ADDRESS_COMPARE(*(pIpAddress1),*(pIpAddress2))
#define IP_ADDRESS_NETWORK_COMPARE								IPV4_ADDRESS_NETWORK_COMPARE
#define IP_ADDRESS_IS_MULTICAST(pIpAddress)						IPV4_ADDRESS_IS_MULTICAST(*(pIpAddress))
#ifndef M_WUFEI_080825
#define IP_ADDRESS_COPY(pIpAddressDestination,pIpAddressSource)	\
	(*(pIpAddressDestination))=(*(pIpAddressSource))
#else
#define IP_ADDRESS_COPY(pIpAddressDestination,pIpAddressSource)	\
	do{	\
	(((u8_t *)(pIpAddressDestination))[0])=(((u8_t *)(pIpAddressSource))[0]);	\
	(((u8_t *)(pIpAddressDestination))[1])=(((u8_t *)(pIpAddressSource))[1]);	\
	(((u8_t *)(pIpAddressDestination))[2])=(((u8_t *)(pIpAddressSource))[2]);	\
	(((u8_t *)(pIpAddressDestination))[3])=(((u8_t *)(pIpAddressSource))[3]);	\
	}while(0);
#endif

#define IP_ADDRESS_GET_TYPE(pIpAddress)	IP_ADDRESS_TYPE_IPV4
#define IP_ADDRESS_IS_IPV4(pIpAddress)	(1)
#define IP_ADDRESS_IS_IPV6(pIpAddress)	(0)
#define IP_ADDRESS_SET_IPV4(pIpAddress,byte1,byte2,byte3,byte4)	\
	IPV4_ADDRESS_SET(*(pIpAddress),byte1,byte2,byte3,byte4)
#define IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(pIpAddress,pIpv4Address)	\
	(*(pIpAddress))=(*(pIpv4Address))
#define IPV4_ADDRESS_COPY_FROM_IP_ADDRESS(pIpv4Address,pIpAddress)	\
	(*(pIpv4Address))=(*(pIpAddress))

/* NOTE: IP_ADDRESS_SET_IPV6: this is an incomplete place holder.
	When IPv6 is implemented then appropriate data bytes will be added
	to this macro. Still, the mapping to SMSC_ERROR is appropriate in this case. */
#define IP_ADDRESS_SET_IPV6(pIpAddress,pIpv6Address) SMSC_ERROR(("IPV6 is not supported"))

/* end of IPV4_ENABLED only */
#elif IPV6_ENABLED
#error IPv6 is not implemented
#else
#error neither IPv4 nor IPv6 is enabled
#endif

#endif
