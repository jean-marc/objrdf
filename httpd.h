/*
 *	main access point to a RDF document
 *	creates new thread for each connection
 *	how can we make it secure: usually the database is not accessible to the web client,
 *	there is a server-side layer (PHP,...) that handles the client's request and queries
 *	the database. How could we bypass that and create our queries in Javascript?
 *	Obviously username/password can not be handled on the client side so there should be a minimum handling done on the server 
 *	An alternative is to compile-in the server side handler, that would make it extra-secure
 *	but would require more programming.
 *
 *	still need user name/password management grant ...privileges on .... class/property
 *
 * 	make sure server is only listening on 127.0.0.1 netstat -an
 * 	tcp        0      0 127.0.0.1:1080              0.0.0.0:*                   LISTEN
 * 	
 *	ssh tunnel:
 *		ssh -f user@server -L 8080:localhost:1080 -N
 *
 *	run chrooted, need all the libraries
 *
 *	we could also generate static forms using xsltproc and caching the files
 *
 */
#ifndef HTTPD_H
#define HTTPD_H
#include "http_parser.h"
#include "objrdf.h"
#include "Sockets.h"
#ifdef PTHREAD
#include <pthread.h>
#else
#include <thread>
#include <mutex>
#endif
namespace objrdf{
	struct httpd{
		httpd();
		httpd(short port);
		void run();//blocking
		short port;
		#ifdef PTHREAD
		typedef pair<httpd*,TCPSocketWrapper*> request_info;
		pthread_mutex_t mutex;	
		static void* server(void* s);
		static void* request(void* s);
		void start();//non-blocking
		#else
		std::mutex m;
		void process(TCPSocketWrapper::TCPAcceptedSocket sock);
		#endif
		static string file_path;
		static map<string,string> mimes;
		typedef void (*fpt)(http_parser&,iostream&);
		void get(http_parser& h,iostream& io);
		void post(http_parser& h,iostream& io);
		void put(http_parser& h,iostream& io);	
		void head(http_parser& h,iostream& io);	
		void delete_(http_parser& h,iostream& io);
		void sparql(http_parser& h,iostream& io);
		void rest(http_parser& h,iostream& io);
		static ostream& http_404(ostream& stream);
	};
}
#endif
