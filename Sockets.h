//
// Sockets.h
//
// this file contains the wrappers that can be used
// as a iostream-compatible TCP/IP sockets
//
// on Windows the program that uses this utility
// should be linked with Ws2_32.lib
//
// Copyright (C) 2001 Maciej Sobczak
//
// you can use this code for any purpose without limitations
// (and for your own risk) as long as this notice remains
//

#ifndef INCLUDED_SOCKETS_H
#define INCLUDED_SOCKETS_H

#ifdef WIN32
// this is for MS Windows
#include <winsock2.h>
//#include <streambuf>
typedef int socklen_t;
#endif

#ifndef WIN32
// this is for Linux
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <netdb.h>
#include <arpa/inet.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1



#endif

#include <streambuf>
#include <string>
#include <iostream>
#include <stdexcept>
using namespace std;


// exception class which designates errors from socket functions
class SocketRunTimeException : public std::runtime_error
{
public:
	explicit SocketRunTimeException(const std::string &what);
	virtual const char * what() const throw();
	int errornumber() const throw() { return errnum; }
	~SocketRunTimeException() throw () { }
private:
	// this will serve as a message returned from what()
	mutable std::string msg;
	int errnum;
};

// exception class which designates logic (programming) errors with sockets
class SocketLogicException : public std::logic_error
{
public:
	explicit SocketLogicException(const std::string &what)
		: std::logic_error(what)
	{
	}
};

// this class serves as a socket wrapper
class TCPSocketWrapper
{
#ifdef WIN32
	// on Windows, socket is represented by the opaque handler
	typedef SOCKET socket_type;
#else
	// on Linux, socket is just a descriptor number
	typedef int socket_type;
#endif

	// proxy helper for syntax:
	// Sock s2(s1.accept());
public:
	class TCPAcceptedSocket
	{
//	private:
	public:
		TCPAcceptedSocket(socket_type s, sockaddr_in a);

		// copy not provided for the proxy
		//TCPAcceptedSocket(const TCPAcceptedSocket &);
		TCPAcceptedSocket& operator=(const TCPAcceptedSocket &);

		socket_type sock;
		sockaddr_in addr;

		friend class TCPSocketWrapper;
	};

//public:

	enum sockstate_type { CLOSED, LISTENING, ACCEPTED, CONNECTED };

	TCPSocketWrapper();
	~TCPSocketWrapper();

	// this is provided for syntax
	// TCPSocketWrapper s2(s2.accept());
	TCPSocketWrapper(const TCPAcceptedSocket &as);

	// server methods

	// binds and listens on a given port number
	void listen(int port, int backlog = 100);
	
	// accepts the new connection
	// it requires the earlier call to listen
	TCPAcceptedSocket accept();

	// client methods

	// creates the new connection
	void connect(const char *address, int port);

	// general methods

	// get the current state of the socket wrapper
	sockstate_type state() const { return sockstate; }

	// get the network address and port number of the socket
	const char * address() const;
	int port() const;

	// write data to the socket
	void write(const void *buf, int len);

	// read data from the socket
	// returns the number of bytes read
	int read(void *buf, int len);

	void close();
	socket_type& get_sock(){return sock;}
private:
	// not for use
	TCPSocketWrapper(const TCPSocketWrapper&);
	TCPSocketWrapper& operator=(const TCPSocketWrapper&);

	socket_type sock;
	sockaddr_in sockaddress;
	sockstate_type sockstate;
};
// this class is supposed to serve as a stream buffer associated with a socket
//template <class charT, class traits = string_char_traits<charT> >

template< class charT, class traits = std::char_traits<charT> >
//class TCPStreamBuffer : public std::streambuf<charT, traits>
class TCPStreamBuffer : public std::basic_streambuf<charT, traits>
{
	typedef std::basic_streambuf<charT, traits>	sbuftype;
	typedef typename sbuftype::int_type		int_type;
	typedef charT					char_type;

public:

	// the buffer will take ownership of the socket (ie. it will close it
	// in the destructor) if takeowner == true
	explicit TCPStreamBuffer(TCPSocketWrapper &sock, bool takeowner = false, std::streamsize bufsize = 10000): rsocket_(sock){
		ownsocket_=takeowner;
		inbuf_=NULL;
		outbuf_=NULL;
		ownbuffers_=false;
		bufsize_=bufsize;
		remained_=0;
	}

	~TCPStreamBuffer()
	{
		if (rsocket_.state() == TCPSocketWrapper::CONNECTED ||
			rsocket_.state() == TCPSocketWrapper::ACCEPTED)
			_flush();

		if (ownbuffers_)
		{
			delete [] inbuf_;
			delete [] outbuf_;
		}

		if (ownsocket_ == true)
			rsocket_.close();
	}

protected:
	sbuftype * setbuf(char_type *s, std::streamsize n)
	{
		if (basic_streambuf<charT,traits>::gptr() == NULL)
		{
			setg(s, s + n, s + n);
			setp(s, s + n);
			inbuf_ = s;
			outbuf_ = s;
			bufsize_ = n;
			ownbuffers_ = false;
		}

		return this;
	}

	void _flush()
	{
		rsocket_.write(outbuf_, (basic_streambuf<charT,traits>::pptr() - outbuf_) * sizeof(char_type));
	}

	int_type overflow(int_type c = traits::eof())
	{
		// this method is supposed to flush the put area of the buffer
		// to the I/O device

		// if the buffer was not already allocated nor set by user,
		// do it just now
		if (basic_streambuf<charT,traits>::pptr() == NULL)
		{
			outbuf_ = new char_type[bufsize_];
			ownbuffers_ = true;
		}
		else
		{
			_flush();
		}
		setp(outbuf_, outbuf_ + bufsize_);
		if (c != traits::eof())
			sputc(traits::to_char_type(c));
		return 0;
	}

	int sync()
	{
		// just flush the put area
		_flush();
		setp(outbuf_, outbuf_ + bufsize_);
		return 0;
	}

	int_type underflow()
	{
		// this method is supposed to read some bytes from the I/O device

		// if the buffer was not already allocated nor set by user,
		// do it just now
		if (basic_streambuf<charT,traits>::gptr() == NULL)
		{
			inbuf_ = new char_type[bufsize_];
			ownbuffers_ = true;
		}

		if (remained_ != 0)
			inbuf_[0] = remainedchar_;

		int readn = rsocket_.read(static_cast<char*>(inbuf_) + remained_,
			bufsize_ * sizeof(char_type) - remained_);

		// if (readn == 0 && remained_ != 0)
		// error - there is not enough bytes for completing
		// the last character before the end of the stream
		// - this can mean error on the remote end

		if (readn == 0)
			return traits::eof();

		int totalbytes = readn + remained_;
		setg(inbuf_, inbuf_,
			inbuf_ + totalbytes / sizeof(char_type));

		remained_ = totalbytes % sizeof(char_type);
		if (remained_ != 0)
			remainedchar_ = inbuf_[totalbytes / sizeof(char_type)];

		return basic_streambuf<charT,traits>::sgetc();
	}

private:

	// not for use
	TCPStreamBuffer(const TCPStreamBuffer&);
	TCPStreamBuffer& operator=(const TCPStreamBuffer&);

	TCPSocketWrapper &rsocket_;
	bool ownsocket_;
	std::streamsize bufsize_;
	char_type *inbuf_;
	char_type *outbuf_;
	int remained_;
	char_type remainedchar_;
	bool ownbuffers_;
};

// this class is an ultimate stream associated with a socket
template <class charT, class traits = std::char_traits<charT> >
class TCPGenericStream :
	private TCPStreamBuffer<charT, traits>,
	public std::basic_iostream<charT, traits>
{
public:

	// this constructor takes 'ownership' of the socket wrapper if btakeowner == true,
	// so that the socket will be closed in the destructor of the
	// TCPStreamBuffer object
	explicit TCPGenericStream(TCPSocketWrapper &sock, bool takeowner = false)
		: TCPStreamBuffer<charT, traits>(sock, takeowner),
		std::basic_iostream<charT, traits>(this)
	{
	}

private:
	// not for use
	TCPGenericStream(const TCPGenericStream&);
	TCPGenericStream& operator=(const TCPGenericStream&);
};

// this is even more specialized for use as a client
template <class charT, class traits = std::char_traits<charT> >
class TCPGenericClientStream :
	//private TCPSocketWrapper,
	public TCPSocketWrapper,
	public TCPGenericStream<charT, traits>
{
public:
	//time_out in milliseconds
	TCPGenericClientStream(const char *address, int port,int time_out=0): TCPGenericStream<charT, traits>(*this, true)
	{
		TCPSocketWrapper::connect(address, port);
#ifdef WIN32
		int  nRet = setsockopt(get_sock(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(int));
		
#else
		timeval t;
		t.tv_sec=time_out/1000;
		t.tv_usec=1000*(time_out%1000);
		int  nRet = setsockopt(get_sock(), SOL_SOCKET, SO_RCVTIMEO, (char*)&t, sizeof(t));
#endif	
		if(nRet==-1){
			cerr<<"Socket setsockopt: "<<nRet<<" "<<errno<<endl;
		}
	}

private:
	// not for use
	TCPGenericClientStream(const TCPGenericClientStream&);
	TCPGenericClientStream& operator=(const TCPGenericClientStream&);
};

// helper declarations for narrow and wide streams
typedef TCPGenericStream<char> TCPStream;
typedef TCPGenericStream<wchar_t> TCPWStream;
typedef TCPGenericClientStream<char> TCPClientStream;
typedef TCPGenericClientStream<wchar_t> TCPWClientStream;

// 'portable' code should call those on the beginning and end of the program
// (Linux/Unix code does not require any initialization and cleanup)
bool socketsInit();	// returns true in success
void socketsEnd();

#endif
