/*
 *	main access point to a RDF document
 *	single-threaded for now	
 *
 */
#ifndef HTTPD_H
#define HTTPD_H
#include "http_parser.h"
#include "objrdf.h"
#include "Sockets.h"
#include <pthread.h>
namespace objrdf{
	struct httpd{
		typedef pair<httpd*,TCPSocketWrapper*> request_info;
		int port;
		rdf::RDF& doc;
		pthread_mutex_t mutex;	
		httpd(rdf::RDF& _doc,short _port=1080):doc(_doc){
			socketsInit();
			port=_port;
			pthread_mutex_init(&mutex,NULL);
		}
		static void* server(void* s);
		static void* request(void* s);
		typedef void (*fpt)(http_parser&,iostream&);
		void run();//blocking
		void start();//non-blocking
		void get(http_parser& h,iostream& io);
		void post(http_parser& h,iostream& io);
		void put(http_parser& h,iostream& io);	
		void head(http_parser& h,iostream& io);	
		void delete_(http_parser& h,iostream& io);
	};
}
#endif
