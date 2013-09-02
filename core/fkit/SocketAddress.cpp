/*
 * Copyright (C) 2007-2013 Frank Mertens.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // gethostname
#include <netdb.h> // getaddrinfo, freeaddrinfo, getnameinfo
#include <errno.h>
#include "strings.h"
#include "Format.h"
#include "SystemStream.h"
#include "SocketAddress.h"

namespace fkit
{

Ref<SocketAddress> SocketAddress::getLocalAddress(SystemStream *socket)
{
	Ref<SocketAddress> address = SocketAddress::create();
	socklen_t len = address->addrLen();
	if (::getsockname(socket->fd(), address->addr(), &len) == -1)
		FKIT_SYSTEM_EXCEPTION;
	return address;
}

Ref<SocketAddress> SocketAddress::getRemoteAddress(SystemStream *socket)
{
	Ref<SocketAddress> address = SocketAddress::create();
	socklen_t len = address->addrLen();
	if (::getpeername(socket->fd(), address->addr(), &len) == -1)
		FKIT_SYSTEM_EXCEPTION;
	return address;
}

SocketAddress::SocketAddress()
	: socketType_(0),
	  protocol_(0)
{}

SocketAddress::SocketAddress(int family, String address, int port)
	: socketType_(0),
	  protocol_(0)
{
	void *addr = 0;

	if (family == AF_INET) {
		// inet4Address_.sin_len = sizeof(addr);
		*(uint8_t *)&inet4Address_ = sizeof(inet4Address_); // uggly, but safe HACK, for BSD4.4
		inet4Address_.sin_family = AF_INET;
		inet4Address_.sin_port = htons(port);
		inet4Address_.sin_addr.s_addr = htonl(INADDR_ANY);
		addr = &inet4Address_.sin_addr;
	}
	else if (family == AF_INET6) {
		#ifdef SIN6_LEN
		inet6Address_.sin6_len = sizeof(inet6Address_);
		#endif
		inet6Address_.sin6_family = AF_INET6;
		inet6Address_.sin6_port = htons(port);
		inet6Address_.sin6_addr = in6addr_any;
		addr = &inet6Address_.sin6_addr;
	}
	else if (family == AF_LOCAL) {
		localAddress_.sun_family = AF_LOCAL;
		if (unsigned(address->size()) + 1 > sizeof(localAddress_.sun_path))
			FKIT_THROW(NetworkingException, "Socket path exceeds maximum length");
		if ((address == "") || (address == "*"))
			localAddress_.sun_path[0] = 0;
		else
			memcpy(localAddress_.sun_path, address->data(), address->size() + 1);
	}
	else
		FKIT_THROW(NetworkingException, "Unsupported address family");

	if (family != AF_LOCAL)
		if ((address != "") && ((address != "*")))
			if (inet_pton(family, address, addr) != 1)
				FKIT_THROW(NetworkingException, "Broken address String");
}

SocketAddress::SocketAddress(struct sockaddr_in *addr)
	: inet4Address_(*addr)
{}

SocketAddress::SocketAddress(struct sockaddr_in6 *addr)
	: inet6Address_(*addr)
{}

SocketAddress::SocketAddress(addrinfo *info)
	: socketType_(info->ai_socktype),
	  protocol_(info->ai_protocol)
{
	if (info->ai_family == AF_INET)
		inet4Address_ = *(sockaddr_in *)info->ai_addr;
	else if (info->ai_family == AF_INET6)
		inet6Address_ = *(sockaddr_in6 *)info->ai_addr;
	else
		FKIT_THROW(NetworkingException, "Unsupported address family");
}

int SocketAddress::family() const { return addr_.sa_family; }
int SocketAddress::socketType() const { return socketType_; }
int SocketAddress::protocol() const { return protocol_; }

int SocketAddress::port() const
{
	int port = 0;

	if (addr_.sa_family == AF_INET)
		port = inet4Address_.sin_port;
	else if (addr_.sa_family == AF_INET6)
		port = inet6Address_.sin6_port;
	else
		FKIT_THROW(NetworkingException, "Unsupported address family");

	return ntohs(port);
}

void SocketAddress::setPort(int port)
{
	port = htons(port);
	if (addr_.sa_family == AF_INET)
		inet4Address_.sin_port = port;
	else if (addr_.sa_family == AF_INET6)
		inet6Address_.sin6_port = port;
	else
		FKIT_THROW(NetworkingException, "Unsupported address family");
}

String SocketAddress::networkAddress() const
{
	String s;
	if (addr_.sa_family == AF_LOCAL) {
		s = localAddress_.sun_path;
	}
	else {
		const int bufSize = INET_ADDRSTRLEN + INET6_ADDRSTRLEN;
		char buf[bufSize];

		const void *addr = 0;
		const char *sz = 0;

		if (addr_.sa_family == AF_INET)
			addr = &inet4Address_.sin_addr;
		else if (addr_.sa_family == AF_INET6)
			addr = &inet6Address_.sin6_addr;
		else
			FKIT_THROW(NetworkingException, "Unsupported address family");

		sz = inet_ntop(addr_.sa_family, const_cast<void*>(addr), buf, bufSize);
		if (!sz)
			FKIT_THROW(NetworkingException, "Illegal binary address format");

		s = sz;
	}
	return s;
}

String SocketAddress::toString() const
{
	Format s(networkAddress());
	if (addr_.sa_family != AF_LOCAL) {
		if (port() != 0)
			s << ":" << port();
	}
	return s;
}

int SocketAddress::scope() const {
	return (addr_.sa_family == AF_INET6) ? inet6Address_.sin6_scope_id : 0;
}
void SocketAddress::setScope(int scope) {
	if (addr_.sa_family == AF_INET6) inet6Address_.sin6_scope_id = scope;
}

Ref<SocketAddressList> SocketAddress::resolve(String hostName, String serviceName, int family, int socketType, String *canonicalName)
{
	addrinfo hint;
	addrinfo *head = 0;

	memclr(&hint, sizeof(hint));
	hint.ai_flags = (canonicalName) ? AI_CANONNAME : 0;
	if ((hostName == "*") || (hostName == ""))
		hint.ai_flags |= AI_PASSIVE;
	hint.ai_family = family;
	hint.ai_socktype = socketType;

	int ret;
	{
		char *h = 0;
		char *s = 0;
		if ((hint.ai_flags & AI_PASSIVE) == 0) h = hostName;
		if (serviceName != "") s = serviceName;
		ret = getaddrinfo(h, s, &hint, &head);
	}

	if (ret != 0)
		if (ret != EAI_NONAME)
			FKIT_THROW(NetworkingException, gai_strerror(ret));

	Ref<SocketAddressList> list = SocketAddressList::create();

	if (canonicalName) {
		if (head) {
			if (head->ai_canonname)
				*canonicalName = head->ai_canonname;
		}
	}

	addrinfo *next = head;

	while (next) {
		if ((next->ai_family == AF_INET) || (next->ai_family == AF_INET6))
			list->append(new SocketAddress(next));
		next = next->ai_next;
	}

	if (head)
		freeaddrinfo(head);

	if (list->size() == 0)
		FKIT_THROW(NetworkingException, "Failed to resolve host name");

	return list;
}

String SocketAddress::lookupHostName(bool *failed) const
{
	const int hostNameSize = NI_MAXHOST;
	const int serviceNameSize = NI_MAXSERV;
	char hostName[hostNameSize];
	char serviceName[serviceNameSize];
	int flags = NI_NAMEREQD;
	if (socketType_ == SOCK_DGRAM) flags |= NI_DGRAM;

	int ret = getnameinfo(addr(), addrLen(), hostName, hostNameSize, serviceName, serviceNameSize, flags);

	if (ret != 0) {
		if (!failed)
			FKIT_THROW(NetworkingException, gai_strerror(ret));
		*failed = true;
		hostName[0] = 0;
	}
	else {
		if (failed)
			*failed = false;
	}

	return String(hostName);
}

String SocketAddress::lookupServiceName() const
{

	const int hostNameSize = NI_MAXHOST;
	const int serviceNameSize = NI_MAXSERV;
	char hostName[hostNameSize];
	char serviceName[serviceNameSize];
	int flags = (socketType_ == SOCK_DGRAM) ? NI_DGRAM : 0;

	hostName[0] = 0;
	serviceName[0] = 0;
	int ret = getnameinfo(addr(), addrLen(), hostName, hostNameSize, serviceName, serviceNameSize, flags);

	if (ret != 0) {
	#ifdef __MACH__
		if (port()) // OSX 10.4 HACK
	#endif
		FKIT_THROW(NetworkingException, gai_strerror(ret));
	}

	return String(serviceName);
}

String SocketAddress::hostName()
{
	const int bufSize = 1024;
	char buf[bufSize + 1];
	String name;
	if (gethostname(buf, bufSize) != -1) {
		buf[bufSize] = 0;
		name = buf;
	}
	return name;
}

struct sockaddr *SocketAddress::addr() { return &addr_; }
const struct sockaddr *SocketAddress::addr() const { return &addr_; }

int SocketAddress::addrLen() const
{
	int len = 0;
	if (family() == AF_INET)
		len = sizeof(sockaddr_in);
	else if (family() == AF_INET6)
		len = sizeof(sockaddr_in6);
	else if (family() == AF_LOCAL)
		len = sizeof(sockaddr_un);
	else {
		len = sizeof(sockaddr_in);
		if (len < int(sizeof(sockaddr_in6))) len = sizeof(sockaddr_in6);
		if (len < int(sizeof(sockaddr_un))) len = sizeof(sockaddr_un);
	}
	return len;
}

} // namespace fkit