//
// Sockets.cpp
//
// this file contains the wrappers (implementation)
// that can be used as a iostream-compatible TCP/IP sockets
//
// on Windows the program that uses this library
// should be linked with Ws2_32.lib
//
// Copyright (C) 2001 Maciej Sobczak
//
// you can use this code for any purpose without limitations
// (and for your own risk) as long as this notice remains
//


#include <sstream>


#include "Sockets.h"
#include <string.h> //for memset

using namespace std;

// class SocketException

SocketRunTimeException::SocketRunTimeException(const string &what)
: runtime_error(what)
{
#ifdef WIN32
	errnum = ::WSAGetLastError();
#else
	errnum = errno;
#endif
}

const char * SocketRunTimeException::what() const throw()
{
	ostringstream ss;
	ss << runtime_error::what();
	ss << " error number: " << errnum;
	msg = ss.str();
	return msg.c_str();
}

// class TCPSocketWrapper::TCPAcceptedSocket

TCPSocketWrapper::TCPAcceptedSocket::TCPAcceptedSocket
	(socket_type s, sockaddr_in a)
	: sock(s), addr(a)
{
}

// class TCPSocketWrapper

TCPSocketWrapper::TCPSocketWrapper()
	: sockstate(CLOSED)
{
}

TCPSocketWrapper::TCPSocketWrapper
	(const TCPSocketWrapper::TCPAcceptedSocket &as)
	: sock(as.sock), sockaddress(as.addr), sockstate(ACCEPTED)
{
}

TCPSocketWrapper::~TCPSocketWrapper()
{
	if (sockstate != CLOSED)
	{
#ifdef WIN32
		closesocket(sock);
#else
		::close(sock);
#endif
	}
}

void TCPSocketWrapper::listen(int port, int backlog)
{
	if (sockstate != CLOSED)
		throw SocketLogicException("socket not in CLOSED state");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		throw SocketRunTimeException("socket failed");
	}

	sockaddr_in local;

	memset(&local, 0, sizeof(local));

	local.sin_family = AF_INET;
	local.sin_port = htons((u_short)port);
	//local.sin_addr.s_addr=htonl(0x7f000001);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	int val=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&val,sizeof(val));
	if (::bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
		throw SocketRunTimeException("bind failed");	

	if (::listen(sock, backlog) == SOCKET_ERROR)
		throw SocketRunTimeException("listen failed");

	memset(&sockaddress, 0, sizeof(sockaddress));
	sockstate = LISTENING;
}

TCPSocketWrapper::TCPAcceptedSocket TCPSocketWrapper::accept()
{
	if (sockstate != LISTENING)
		throw SocketLogicException("socket not listening");

	sockaddr_in from;
	socklen_t len = sizeof(from);

	memset(&from, 0, len);

	socket_type newsocket = ::accept(sock, (sockaddr*)&from, &len);
	if (newsocket == INVALID_SOCKET)
		throw SocketRunTimeException("accept failed");

	return TCPAcceptedSocket(newsocket, from);
}

void TCPSocketWrapper::connect(const char *address, int port)
{
	if (sockstate != CLOSED)
		throw SocketLogicException("socket not in CLOSED state");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		throw SocketRunTimeException("socket failed");
	}
	int buffSize = 100000;
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char *)&buffSize,sizeof(buffSize));
	setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char *)&buffSize,sizeof(buffSize));
	hostent *hp;
//	mike sept 11 
//	unsigned long addr = inet_addr(address);
//	if (addr != INADDR_NONE)
//		hp = gethostbyaddr((const char*)&addr, 4, AF_INET);
//	else
		hp = gethostbyname(address);

	if (hp == NULL)
		throw SocketRunTimeException("cannot resolve address");

	if (hp->h_addrtype != AF_INET)
		throw SocketRunTimeException
			("address resolved with TCP incompatible type");

	memset(&sockaddress, 0, sizeof(sockaddress));
	memcpy(&(sockaddress.sin_addr), hp->h_addr_list[0], hp->h_length);
	sockaddress.sin_family = AF_INET;
	sockaddress.sin_port = htons((u_short)port);

	if (::connect(sock, (sockaddr*)&sockaddress, sizeof(sockaddress)) == SOCKET_ERROR){
			//closesocket(sock);
			throw SocketRunTimeException("connect failed");
	}

	sockstate = CONNECTED;
}

const char * TCPSocketWrapper::address() const
{
	if (sockstate != CONNECTED && sockstate != ACCEPTED)
		throw SocketLogicException("socket not connected");

	return inet_ntoa(sockaddress.sin_addr);
}

int TCPSocketWrapper::port() const
{
	if (sockstate != CONNECTED && sockstate != ACCEPTED)
		throw SocketLogicException("socket not connected");

	return ntohs(sockaddress.sin_port);
}

void TCPSocketWrapper::write(const void *buf, int len)
{
	if (sockstate != CONNECTED && sockstate != ACCEPTED)
		throw SocketLogicException("socket not connected");

	int written;
	while (len > 0)
	{
		if ((written = send(sock, (const char*)buf, len, 0)) == SOCKET_ERROR)
			throw SocketRunTimeException("write failed");
		//cerr<<written<<" written\n";
		len -= written;
		buf = (const char*)buf + written;
	}
}

int TCPSocketWrapper::read(void *buf, int len) 
{
	if (sockstate != CONNECTED && sockstate != ACCEPTED)
		throw SocketLogicException("socket not connected");

	int readn = recv(sock, (char*)buf, len, 0);
	if (readn == SOCKET_ERROR)
		throw SocketRunTimeException("read failed");

	return readn;
}

void TCPSocketWrapper::close()
{
	if (sockstate != CLOSED)
	{
#ifdef WIN32
		if (closesocket(sock) == SOCKET_ERROR)
			throw SocketRunTimeException("close failed");
#else
		if (::close(sock))
			throw SocketRunTimeException("close failed");
#endif
		sockstate = CLOSED;
	}
}

bool socketsInit()
{
#ifdef WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 0), &wsadata) == 0)
		return true;
	else
		return false;
#else
	// Linux/Unix do not require any initialization
	return true;
#endif
}

void socketsEnd()
{
#ifdef WIN32
	// we do not care about the error codes
	// anyway, we end the program
	WSACleanup();
#endif
}
