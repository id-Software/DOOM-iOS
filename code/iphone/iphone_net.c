/*
 *  iphone_net.c
 *  doom
 *
 *  Created by John Carmack on 7/8/09.
 *  Copyright 2009 id Software. All rights reserved.
 *
 */
/*
 
 Copyright (C) 2009 Id Software, Inc.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */


/*

 Deal with all the DNS / bonjour service discovery and resolution
 
 */

#include "doomiphone.h"

#include <dns_sd.h>
#include <netdb.h>		// for gethostbyname
#include <net/if.h>		// for if_nameindex()

DNSServiceRef	browseRef;
DNSServiceRef	clientServiceRef;
DNSServiceRef	serviceRef;
boolean			serviceRefValid;

typedef struct {
	int				interfaceIndex;
	char			browseName[1024];
	char			browseRegtype[1024];
	char			browseDomain[1024];
} service_t;

boolean			localServer;

// we can find services on both WiFi and Bluetooth interfaces
#define MAX_SERVICE_INTEFACES 4
service_t		serviceInterfaces[MAX_SERVICE_INTEFACES];

boolean			gotServerAddress;
struct sockaddr resolvedServerAddress;

static const char *serviceName = "_DoomServer._udp.";

int InterfaceIndexForName( const char *ifname );

void DNSServiceRegisterReplyCallback ( 
										  DNSServiceRef sdRef, 
										  DNSServiceFlags flags, 
										  DNSServiceErrorType errorCode, 
										  const char *name, 
										  const char *regtype, 
										  const char *domain, 
										  void *context ) {
	(void)sdRef;
	(void)flags;
	(void)name;
	(void)regtype;
	(void)domain;
	(void)context;
	
	if ( errorCode == kDNSServiceErr_NoError ) {
		localServer = true;
	} else {
		localServer = false;
	}
}

boolean RegisterGameService() {
	DNSServiceErrorType	err = DNSServiceRegister( 
					   &serviceRef, 
					   kDNSServiceFlagsNoAutoRename,		// we want a conflict error
					   InterfaceIndexForName( "en0" ),		// pass 0 for all interfaces
					   "iPhone Doom Classic",
					   serviceName,
					   NULL,	// domain
					   NULL,	// host
					   htons( DOOM_PORT ),
					   0,		// txtLen
					   NULL,	// txtRecord
					   DNSServiceRegisterReplyCallback,
					   NULL		// context
					   );
	
	if ( err != kDNSServiceErr_NoError ) {
		printf( "DNSServiceRegister error\n" );
	} else {
		// block until we get a response, process it, and run the callback
		err = DNSServiceProcessResult( serviceRef );
		if ( err != kDNSServiceErr_NoError ) {
			printf( "DNSServiceProcessResult error\n" );
		}
	}
	return localServer;
}

void TerminateGameService() {
	if ( localServer ) {
		localServer = false;
	}
	DNSServiceRefDeallocate( serviceRef );
	memset( serviceInterfaces, 0, sizeof( serviceInterfaces ) );
}

void DNSServiceQueryRecordReplyCallback ( 
											 DNSServiceRef DNSServiceRef, 
											 DNSServiceFlags flags, 
											 uint32_t interfaceIndex, 
											 DNSServiceErrorType errorCode, 
											 const char *fullname, 
											 uint16_t rrtype, 
											 uint16_t rrclass, 
											 uint16_t rdlen, 
											 const void *rdata, 
											 uint32_t ttl, 
											 void *context ) {
	(void)DNSServiceRef;
	(void)flags;
	(void)errorCode;
	(void)rrtype;
	(void)rrclass;
	(void)ttl;
	(void)context;
											 
	assert( rdlen == 4 );
	const byte *ip = (const byte *)rdata;
	char	interfaceName[IF_NAMESIZE];
	if_indextoname( interfaceIndex, interfaceName );
	printf( "DNSServiceQueryRecordReplyCallback: %s, interface[%i] = %s, [%i] = %i.%i.%i.%i\n", 
		   fullname, interfaceIndex, interfaceName, rdlen, ip[0], ip[1], ip[2], ip[3] );

	ReportNetworkInterfaces();
	
	memset( &resolvedServerAddress, 0, sizeof( resolvedServerAddress ) );
	struct sockaddr_in *sin = (struct sockaddr_in *)&resolvedServerAddress;
	sin->sin_len = sizeof( resolvedServerAddress );
	sin->sin_family = AF_INET;
	sin->sin_port = htons( DOOM_PORT );
	memcpy( &sin->sin_addr, ip, 4 );
	
	gotServerAddress = true;
}
	

DNSServiceFlags callbackFlags;

void DNSServiceResolveReplyCallback ( 
										 DNSServiceRef sdRef, 
										 DNSServiceFlags flags, 
										 uint32_t interfaceIndex, 
										 DNSServiceErrorType errorCode, 
										 const char *fullname, 
										 const char *hosttarget, 
										 uint16_t port, 
										 uint16_t txtLen, 
										 const unsigned char *txtRecord, 
										 void *context ) {
	(void)sdRef;
	(void)errorCode;
	(void)port;
	(void)txtLen;
	(void)txtRecord;
	(void)context;
	
	char	interfaceName[IF_NAMESIZE];
	if_indextoname( interfaceIndex, interfaceName );
	printf( "Resolve: interfaceIndex [%i]=%s : %s @ %s\n", interfaceIndex, interfaceName, fullname, hosttarget );
	callbackFlags = flags;
	
#if 0	
	struct hostent * host = gethostbyname( hosttarget );
	if ( host ) {
		printf( "h_name: %s\n", host->h_name );
		if ( host->h_aliases ) {	// this can be NULL
			for ( char **list = host->h_aliases ; *list ; list++ ) {
				printf( "h_alias: %s\n", *list );
			}
		}
		printf( "h_addrtype: %i\n", host->h_addrtype );
		printf( "h_length: %i\n", host->h_length );
		if ( !host->h_addr_list ) {	// I doubt this would ever be NULL...
			return;
		}
		for ( char **list = host->h_addr_list ; *list ; list++ ) {
			printf( "addr: %i.%i.%i.%i\n", ((byte *)*list)[0], ((byte *)*list)[1], ((byte *)*list)[2], ((byte *)*list)[3] );
		}
		
		memset( &resolvedServerAddress, 0, sizeof( resolvedServerAddress ) );
		resolvedServerAddress.sin_len = sizeof( resolvedServerAddress );
		resolvedServerAddress.sin_family = host->h_addrtype;
		resolvedServerAddress.sin_port = htons( DOOM_PORT );
		assert( host->h_length == 4 );
		memcpy( &resolvedServerAddress.sin_addr, *host->h_addr_list, host->h_length );

		gotServerAddress = true;
	}	
#else
	DNSServiceRef	queryRef;
	
	// look up the name for this host
	DNSServiceErrorType err = DNSServiceQueryRecord ( 
													 &queryRef, 
													 kDNSServiceFlagsForceMulticast, 
													 interfaceIndex, 
													 hosttarget, 
													 kDNSServiceType_A,		// we want the host address
													 kDNSServiceClass_IN, 
													 DNSServiceQueryRecordReplyCallback, 
													 NULL /* may be NULL */
													 );  	
	if ( err != kDNSServiceErr_NoError ) {
		printf( "DNSServiceQueryRecord error\n" );
	} else {
		// block until we get a response, process it, and run the callback
		err = DNSServiceProcessResult( queryRef );
		if ( err != kDNSServiceErr_NoError ) {
			printf( "DNSServiceProcessResult error\n" );
		}
		DNSServiceRefDeallocate( queryRef );
	}
#endif	
}

boolean NetworkServerAvailable() {
	for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
		if ( serviceInterfaces[i].interfaceIndex != 0 ) {
			return true;
		}
	}
	return false;
}

// returns "WiFi", "BlueTooth", or "" for display on the
// main menu multiplayer icon
const char *NetworkServerTransport() {
	int	count = 0;
	for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
		if ( serviceInterfaces[i].interfaceIndex != 0 ) {
			count++;
		}		
	}
	
	static char	str[1024];
	
	str[0] = 0;
	for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
		int	index = serviceInterfaces[i].interfaceIndex;
		if ( index == 0 ) {
			continue;
		}		
		if ( str[0] ) {
			strcat( str, "+" );
		}
		if ( index == -1 ) {
			strcat( str, "BT-NEW" );
		} else if ( index == 1 ) {
			strcat( str, "LOOP" );	// we should never see this!
		} else if ( index == 2 ) {
			strcat( str, "WiFi" );
		} else {
			strcat( str, "BT-EST" );
		}
	}
	return str;
}



boolean ResolveNetworkServer( struct sockaddr *addr ) {
	if ( !NetworkServerAvailable() ) {
		return false;
	}
	
	gotServerAddress = false;
	
	DNSServiceRef	resolveRef;
	
	// An unconnected bluetooth service will report an interfaceIndex of -1, so if
	// we have a wifi link with an interfaceIndex > 0, use that
	// explicitly.
	service_t	*service = NULL;
	for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
		if ( serviceInterfaces[i].interfaceIndex > 0 ) {
			service = &serviceInterfaces[i];
			char	interfaceName[IF_NAMESIZE];
			if_indextoname( service->interfaceIndex, interfaceName );
			printf( "explicitly resolving server on interface %i = %s\n", service->interfaceIndex, interfaceName );
			break;
		}
	}
	if ( !service ) {
#if 0			// don't support bluetooth now
		// settle for the unconnected bluetooth service
		for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
			if ( serviceInterfaces[i].interfaceIndex != 0 ) {
				service = &serviceInterfaces[i];
				break;
			}
		}
#endif		
		if ( !service ) {
			printf( "No serviceInterface current.\n" );
			return false;
		}
	}
	
	// look up the name for this service
	
	DNSServiceErrorType err = DNSServiceResolve ( 
										 &resolveRef, 
										 kDNSServiceFlagsForceMulticast,	// always on local link
										 service->interfaceIndex > 0 ? service->interfaceIndex : 0,		// don't use -1 for bluetooth
										 service->browseName, 
										 service->browseRegtype, 
										 service->browseDomain, 
										 DNSServiceResolveReplyCallback, 
										 NULL			/* context */
										 );  

	if ( err != kDNSServiceErr_NoError ) {
		printf( "DNSServiceResolve error\n" );
	} else {
		// We can get two callbacks when both wifi and bluetooth are enabled
		callbackFlags = 0;
		do {
			err = DNSServiceProcessResult( resolveRef );
			if ( err != kDNSServiceErr_NoError ) {
				printf( "DNSServiceProcessResult error\n" );
			}
		} while ( callbackFlags & kDNSServiceFlagsMoreComing );
		DNSServiceRefDeallocate( resolveRef );
	}
	
	if ( gotServerAddress ) {
		*addr = resolvedServerAddress;
		return true;
	}
	
	
	return false;
}


void DNSServiceBrowseReplyCallback(
								   DNSServiceRef sdRef, 
								   DNSServiceFlags flags, 
								   uint32_t interfaceIndex, 
								   DNSServiceErrorType errorCode, 
								   const char *theServiceName, 
								   const char *regtype, 
								   const char *replyDomain, 
								   void *context ) {
	(void)sdRef;
	(void)errorCode;
	(void)context;
	
	printf( "DNSServiceBrowseReplyCallback %s: interface:%i name:%s regtype:%s domain:%s\n", 
		   (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE",
		   interfaceIndex, theServiceName, regtype, replyDomain );
	if ( flags & kDNSServiceFlagsAdd ) {
		// add it to the list
		if ( interfaceIndex == 1 ) {
			printf( "Not adding service on loopback interface.\n" );
		} else {
			for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
				service_t *service = &serviceInterfaces[i];
				if ( service->interfaceIndex == 0 ) {
					strncpy( service->browseName, theServiceName, sizeof( service->browseName ) -1 );
					strncpy( service->browseRegtype, regtype, sizeof( service->browseRegtype ) -1 );
					strncpy( service->browseDomain, replyDomain, sizeof( service->browseDomain ) -1 );
					service->interfaceIndex = interfaceIndex;
					break;
				}
			}
		}
	} else {
		// remove it from the list
		for ( int i = 0 ; i < MAX_SERVICE_INTEFACES ; i++ ) {
			if ( serviceInterfaces[i].interfaceIndex == interfaceIndex ) {
				serviceInterfaces[i].interfaceIndex = 0;
			}
		}
	}
}

void ProcessDNSMessages() {
	static boolean initialized;
	
	if ( !initialized ) {
		initialized = true;
		DNSServiceErrorType err = DNSServiceBrowse ( 
													&browseRef, 
													0,					/* flags */
													0,					/* interface */
													serviceName, 
													NULL,				/* domain */
													DNSServiceBrowseReplyCallback, 
													NULL				/* context */
													);  
		if ( err != kDNSServiceErr_NoError ) {
			printf( "DNSServiceBrowse error\n" );
			return;
		}		
	}
	
	// poll the socket for updates
	int	socket = DNSServiceRefSockFD( browseRef );
	if ( socket <= 0 ) {
		return;
	}
	fd_set	set;
	FD_ZERO( &set );
	FD_SET( socket, &set );
	
	struct timeval tv;
	memset( &tv, 0, sizeof( tv ) );
	if ( select( socket+1, &set, NULL, NULL, &tv ) > 0 ) {
		DNSServiceProcessResult( browseRef );
	}	
}

void ReportNetworkInterfaces() {
	struct ifaddrs *ifap;
	printf( "getifaddrs():\n" );
	if ( getifaddrs( &ifap ) == -1 ) {
		perror( "getifaddrs(): " );
	} else {
		for ( struct ifaddrs *ifa = ifap ; ifa ; ifa = ifa->ifa_next ) {
			struct sockaddr_in *ina = (struct sockaddr_in *)ifa->ifa_addr;
			if ( ina->sin_family == AF_INET ) {
				byte *ip = (byte *)&ina->sin_addr;
				printf( "ifa_name: %s ifa_flags: %i sa_family: %i=AF_INET ip: %i.%i.%i.%i\n", ifa->ifa_name, ifa->ifa_flags,
					   ina->sin_family, ip[0], ip[1], ip[2], ip[3]  );
			} else if ( ina->sin_family == AF_LINK ) {
				struct if_data *data = (struct if_data *)ifa->ifa_data;
				printf( "ifa_name: %s ifa_flags: %i sa_family: %i=AF_LINK ifi_ipackets: %i\n", ifa->ifa_name, ifa->ifa_flags,
					   ina->sin_family, data->ifi_ipackets );
			} else {
				printf( "ifa_name: %s ifa_flags: %i sa_family: %i=???\n", ifa->ifa_name, ifa->ifa_flags,
					   ina->sin_family );
			}
		}
		freeifaddrs( ifap );
	}
	
	printf( "if_nameindex():\n" );
	struct if_nameindex *ifnames = if_nameindex();
	if ( !ifnames ) {
		perror( "if_ameindex():" );
	} else {
		for ( int i = 0 ; ifnames[i].if_index != 0 ; i++ ) {
			printf( "%i : %s\n", ifnames[i].if_index, ifnames[i].if_name );
		}
		if_freenameindex( ifnames );
	}
	
}

boolean NetworkAvailable() {
	struct ifaddrs *ifap;
	if ( getifaddrs( &ifap ) == -1 ) {
		return false;
	}
	
	// We can't tell if bluetooth is available from here, because
	// the interface doesn't appear until after the service is found,
	// but I decided not to support bluetooth for now due to the poor performance.
	boolean	goodInterface = false;
	
	for ( struct ifaddrs *ifa = ifap ; ifa ; ifa = ifa->ifa_next ) {
		struct sockaddr_in *ina = (struct sockaddr_in *)ifa->ifa_addr;
		if ( ina->sin_family == AF_INET ) {
			if ( !strcmp( ifa->ifa_name, "en0" ) ) {
				goodInterface = true;
			}
		}
	}
	freeifaddrs( ifap );
	
	return goodInterface;
}

int InterfaceIndexForAddress( struct sockaddr_in *adr ) {
	(void)adr;
	// FIXME: compare against getifaddrs 
	return 0;
}

struct sockaddr_in AddressForInterfaceName( const char *ifname ) {	
	struct sockaddr_in s;
	memset( &s, 0, sizeof( s ) );
	
	struct ifaddrs *ifap;
	if ( getifaddrs( &ifap ) == -1 ) {
		perror( "getifaddrs()" );
		return s;
	}
	
	struct ifaddrs *ifa;
	for ( ifa = ifap ; ifa ; ifa = ifa->ifa_next ) {
		struct sockaddr_in *ina = (struct sockaddr_in *)ifa->ifa_addr;
		if ( ina->sin_family == AF_INET && !strcmp( ifa->ifa_name, ifname ) ) {
			byte *ip = (byte *)&ina->sin_addr;
			printf( "AddressForInterfaceName( %s ) = ifa_name: %s ifa_flags: %i sa_family: %i=AF_INET ip: %i.%i.%i.%i\n", 
				   ifname, ifa->ifa_name, ifa->ifa_flags,
				   ina->sin_family, ip[0], ip[1], ip[2], ip[3]  );
			freeifaddrs( ifap );
			return *ina;
		}
	}
	freeifaddrs( ifap );
	printf( "AddressForInterfaceName( %s ): Couldn't find IP address\n", ifname );
	return s;
}

int InterfaceIndexForName( const char *ifname ) {
	struct if_nameindex *ifnames = if_nameindex();
	if ( !ifnames ) {
		perror( "if_nameindex()" );
		return 0;
	}
	for ( int i = 0 ; ifnames[i].if_index != 0 ; i++ ) {
		if ( !strcmp( ifname, ifnames[i].if_name ) ) {
			int	index = ifnames[i].if_index;
			if_freenameindex( ifnames );
			return index;
		}
	}	
	printf( "InterfaceIndexForName( %s ): Couldn't find interface\n", ifname );
	if_freenameindex( ifnames );
	return 0;
}

struct sockaddr_in AddressForInterfaceIndex( int interfaceIndex ) {
	struct sockaddr_in addr;

	memset( &addr, 0, sizeof( addr ) );
	addr.sin_len = sizeof( addr );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;

	if ( interfaceIndex == 0 ) {
		return addr;
	}
		
	struct if_nameindex *ifnames = if_nameindex();
	if ( !ifnames ) {
		perror( "if_ameindex()" );
		return addr;
	}
	for ( int i = 0 ; ifnames[i].if_index != 0 ; i++ ) {
		if ( ifnames[i].if_index == interfaceIndex ) {
			addr = AddressForInterfaceName( ifnames[i].if_name );
			if_freenameindex( ifnames );
			return addr;
		}
	}	
	printf( "AddressForInterfaceIndex( %i ): Couldn't find interface\n", interfaceIndex );
	if_freenameindex( ifnames );
	return addr;
}

struct sockaddr_in gameSocketAddress;

int	UDPSocket( const char *interfaceName, int portnum ) {
	int udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( udpSocket == -1 ) {
		Com_Printf( "UDP socket failed: %s\n", strerror( errno ) );
		return -1;
	}
	struct sockaddr_in addr = AddressForInterfaceName( interfaceName );
	addr.sin_port = htons( portnum );
	byte *ip = (byte *)&addr.sin_addr;
	gameSocketAddress = addr;
	Com_Printf( "UDPSocket( %s, %i ) = %i.%i.%i.%i\n", interfaceName, portnum,
			   ip[0], ip[1], ip[2], ip[3] );	
	if ( bind( udpSocket, (struct sockaddr *)&addr, sizeof( addr ) ) == -1 ) {
		Com_Printf( "UDP bind failed: %s\n", strerror( errno ) );
		close( udpSocket );
		return -1;
	}
	
#if 0	
	// enable broadcast
	int	on = 1;
	if ( setsockopt( udpSocket, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on) ) == -1 ) {
		Com_Printf( "UDP setsockopt failed: %s\n", strerror( errno ) );
		close( udpSocket );
		return -1;
	}
#endif

#if 0	
	// set the type-of-service, in hopes that the link level drivers use it
	// to stop buffering huge amounts of data when there are line errors
	int tos = 0x10; /* IPTOS_LOWDELAY; */       /* see <netinet/in.h> */
	if ( setsockopt( udpSocket, IPPROTO_IP, IP_TOS, &tos, sizeof(tos) ) == -1 ) {
		Com_Printf( "setsockopt IP_TOS failed: %s\n", strerror( errno ) );
	}
#endif
	
	// enable non-blocking IO
	if ( fcntl( udpSocket, F_SETFL, O_NONBLOCK ) == -1 ) {
		Com_Printf( "UDP fcntl failed: %s\n", strerror( errno ) );
		close( udpSocket );
		return -1;
	}
	
	return udpSocket;
}

