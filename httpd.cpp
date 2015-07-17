#include "httpd.h"
//#include "rdf_xml_parser.h"
#include "sparql_engine.h"
#ifdef NEW_HEADER
#include "sparql_parser.h"
#endif
#include <unistd.h>
#include "url_decoder.h"
#include <fstream>
#include <sys/stat.h>
//#include <iomanip>
#include <time.h>
#include "popen_streambuf.h"
using namespace objrdf;
map<string,string> httpd::mimes={{"html","text/html"},{"xhtml","text/html"},
					{"xml","text/xml"},{"xsd","text/xml"},{"xsl","text/xml"},
					{"kml","application/vnd.google-earth.kml+xml"},
					{"rdf","application/rdf+xml"},
					{"svg","image/svg+xml"},
					{"png","image/png"},
					{"css","text/css"},
					{"js","application/javascript"},
					{"mat","application/octet-stream"}};
string get_mime(string extension){
	auto i=httpd::mimes.find(extension);
	return i==httpd::mimes.end() ? "text/plain" : i->second;
}
string url_decode(string in){
	istringstream is(in);//we need to make the parser more generic
	url_decoder u(is);
	u.go();
	return u.decoded;	
}
string httpd::file_path="www/";
httpd::httpd(short port):port(port){
	socketsInit();
	#ifdef PTHREAD
	pthread_mutex_init(&mutex,NULL);
	#endif
}
#ifdef PTHREAD
void httpd::run(){
	server(this);
}
void httpd::start(){
	pthread_t shell_t;
	pthread_create(&shell_t,NULL,server,this);
}
void* httpd::server(void* s){
	TCPSocketWrapper sockserver;
	int port=static_cast<httpd*>(s)->port;
	//can we make it listen only on 127.0.0.1?
	sockserver.listen(port);
	LOG << "http server is listening on port "<<port<< endl;
	for(;;){
		TCPSocketWrapper* sock=new TCPSocketWrapper(sockserver.accept());
		LOG<<"accepted connection from: "<<sock->address()<<" port:"<<sock->port()<<endl;
		request_info* r=new request_info(static_cast<httpd*>(s),sock);
		pthread_t shell_t;
		pthread_create(&shell_t,NULL,request,r);
	}
}
#else
void httpd::run(){
	TCPSocketWrapper sockserver;
	sockserver.listen(port);
	for(;;){
		TCPSocketWrapper::TCPAcceptedSocket sock(sockserver.accept());
		auto t=std::thread(&httpd::process,this,sock);
		t.detach();
	}
}
void httpd::process(TCPSocketWrapper::TCPAcceptedSocket sock){
	try{
		TCPSocketWrapper _sock(sock);
		LOG<<"HTTPD accepted connection from: "<<_sock.address()<<" port:"<<_sock.port()<<endl;
		TCPStream stream(_sock);
		stream.exceptions(iostream::eofbit|iostream::badbit);
		while(stream.good()){
			LOG<<"peek: "<<stream.peek()<<endl;
			http_parser p(stream);
			if(p.go()){//blocking
				if(p.current_method=="GET") get(p,stream);
				else if(p.current_method=="POST") post(p,stream);
				else{
					stream<<"HTTP/1.1 400 Bad Request\r\n";
					stream.setstate(ios::badbit);//maybe not needed
					stream.flush();
				}
			}else{
				LOG<<"could not parse http header\n";
				stream<<"HTTP/1.1 400 Bad Request\r\n";
				stream.setstate(ios::badbit);
				stream.flush();
			}
			LOG<<"done!"<<endl;
		}
		LOG<<"ending thread"<<endl;
	}catch(SocketRunTimeException& e){
		LOG<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		LOG<<"****** "<<e.what()<<endl;
	}catch(...){
		LOG<<"****** exception!"<<endl;
	}

}
#endif
inline ostream& http_404(ostream& stream){
	stream<<"HTTP/1.1 404 file not found"<<endl;
	ostringstream o;
	char hostname[255];
	gethostname(hostname,255);
	o<<"<html><title>404 Not Found</title><body><h1>Not Found</h1>The requested URL was not found on this server.";
	o<<"<hr/><address>httpd Server at "<<hostname<<"</address></body></html>"<<endl;
	stream<<"Content-Length:"<<o.str().size()<<endl;
	stream<<"Content-Type:"<<get_mime("html")<<"\r\n";
	stream<<"\r\n";
	stream<<o.str();
	stream.flush();
	return stream;
}
inline ostream& http_400(ostream& stream){
	stream<<"HTTP/1.1 400 Bad Request\r\n";
	stream<<"Content-Length: 0\r\n";
	stream<<"\r\n";
	stream.flush();
	return stream;
}
#ifdef PTHREAD
void* httpd::request(void* s){
	LOG<<"new request"<<endl;
	request_info* r=static_cast<request_info*>(s);
	try{
		TCPStream stream(*r->second);
		stream.exceptions(iostream::eofbit|iostream::badbit);
		while(stream.good()){
			LOG<<"peek: "<<stream.peek()<<endl;
			http_parser p(stream);
			if(p.go()){//blocking
				if(p.current_method=="GET") r->first->get(p,stream);
				else if(p.current_method=="POST") r->first->post(p,stream);
				else{
					stream<<"HTTP/1.1 400 Bad Request\r\n";
					stream.setstate(ios::badbit);//maybe not needed
					stream.flush();
				}
			}else{
				LOG<<"could not parse http header\n";
				stream<<"HTTP/1.1 400 Bad Request\r\n";
				stream.setstate(ios::badbit);
				stream.flush();
			}
			LOG<<"done!"<<endl;
		}
		LOG<<"ending thread"<<endl;
		delete r->second;
		delete r;//will attempt a flush here
		return 0;
	}catch(SocketRunTimeException& e){
		LOG<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		LOG<<"****** "<<e.what()<<endl;
	}catch(...){
		LOG<<"****** exception!"<<endl;
	}
	//return 0;
}
#endif
void httpd::get(http_parser& h,iostream& io){
	if(h.current_path.compare(0,6,"/data/")==0){
	/*
 	*	can only query local variables
 	*	problem with hash URI: the browser loads the whole document
 	*/
		string id=h.current_path.substr(6);
		CONST_RESOURCE_PTR r=find(uri(id));
		if(!r){
			io<<http_404;
		}else{
			io<<"HTTP/1.1 200 OK\r\n";
			io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			out<<"<?xml-stylesheet type='text/xsl' href='/describe.xsl'?>";
			//not a valid RDF document, should use DESCRIBE query instead
			to_rdf_xml(r,out);
			io<<"Content-Length:"<<out.str().size()<<"\r\n";
			io<<"\r\n";
			io<<out.str();
			io.flush();
		}
	}else if(h.current_path.compare(0,3,"/r/")==0){
		string id=h.current_path.substr(3);
		CONST_RESOURCE_PTR r=find(uri(id));
		if(!r){
			io<<http_404;
		}else{
			//io<<"HTTP/1.1 200 OK\r\n";
			//a resource can have its own content, could be a static/dynamic document
			get_output(r,io); //not protected by mutex
		}	
	}else if(h.current_path=="/sparql"){//sparql end-point
		/*
 		*	we should test if the query is select or update, only authorized users should modify db,
 		*	in mysql we have: 'grant all privileges on 'table'.'column' to 'user'@'host' with privileges SELECT|INSERT|UPDATE|DELETE
 		*	in sparql an UPDATE is implemented by DELETE then INSERT
 		*	here we have rdfs::Class and rdf::Property
 		*
 		*/ 
		auto i=h.url_arguments.find("query");
		if(i!=h.url_arguments.end()){
			//can we cache queries?
			istringstream is(url_decode(i->second)+char(EOF));//hack to work around parser limitation
			is.exceptions(iostream::eofbit);
			sparql_parser p(is);
			//modify the parser
			//if(p.go()){
			if(p._go<seqw<sparql_parser::document,char_p<EOF>>>()){
				LOG<<"success!"<<endl;
				io<<"HTTP/1.1 200 OK"<<"\r\n";
				ostringstream out;
				auto i=h.url_arguments.find("format");
				if(i->second=="csv"){
					io<<"Content-Type: "<<"text/plain"<<"\r\n";
					p.out_csv(out);
				}else{
					io<<"Content-Type:"<<get_mime("xml")<<"\r\n";
					auto j=h.url_arguments.find("xsl");
					if(j!=h.url_arguments.end()){
						//only one xslt parameter for now, see https://developer.mozilla.org/en/XSLT/PI_Parameters
						//does not work with Chromium
						auto k=h.url_arguments.find("xslt-param-name");
						auto l=h.url_arguments.find("xslt-param-value");
						if((k!=h.url_arguments.end())&&(l!=h.url_arguments.end()))
							out<<"<?xslt-param name='"<<k->second<<"' value='"<<url_decode(l->second)<<"'?>"<<endl;
						out<<"<?xml-stylesheet type='text/xsl' href='"<<j->second<<"'?>"<<endl;
					}
					//add support for css
					{
						auto j=h.url_arguments.find("css");
						if(j!=h.url_arguments.end()){
							out<<"<?xml-stylesheet type='text/css' href='"<<j->second<<"'?>"<<endl;
						}
					}
					p.out(out);//this where the processing takes place
				}
				io<<"Content-Length:"<<out.str().size()<<"\r\n";
				io<<"\r\n";
				io<<out.str();
				io.flush();
			}else{
				io<<"HTTP/1.1 500 Bad Request\r\n";
				io<<"\r\n";
				io<<"could not parse sparql query"<<endl;
				io.setstate(ios::badbit);
				io.flush();
			}
		}else{
			io<<"HTTP/1.1 500 Bad Request\r\n";
			io<<"\r\n";
			io<<"no query argument"<<endl;
			io.setstate(ios::badbit);
			io.flush();
		}	
	}else if(h.current_path=="/add"){
		/*
 		*	create 
 		*/ 
	}else if(h.current_path.compare(0,9,"/cgi-bin/")==0){
		//pass the arguments to the command: http://unix.derkeiler.com/Newsgroups/comp.unix.programmer/2006-12/msg00447.html
		//string command="./"+h.current_path.substr(9);
		string command=file_path+h.current_path.substr(9);
		LOG<<"running command `"<<command<<"'"<<endl;
		popen_streambuf sb;
		istream in(&sb);
		if (NULL == sb.open(command.c_str(), "r")) {
			io<<"HTTP/1.1 500 Bad Request\r\n";
			io<<"\r\n";
			io<<"could not parse sparql query"<<endl;
			io.setstate(ios::badbit);
			io.flush();
		}else{
			io<<in.rdbuf();
			io.flush();
		}
	#ifndef PTHREAD
	}else if(h.current_path.compare(0,7,"/_test_")==0){
		/* 
		* how would we implement a basic comet-like function?
		* let's just sleep for some time
		* we could have some real triggering going on when a client ask to be notified when a property is modified/created/deleted
		*/
		std::this_thread::sleep_for(std::chrono::seconds(5));
		io<<"HTTP/1.1 200 OK\r\n";
		io<<"Content-Length:"<<0<<"\r\n";
		io<<"\r\n"<<flush;
	#endif		
	}else{
		//default to index.html
		string path=h.current_path.substr(1);
		if(path.empty()) path="index.html";
		//need to add code to prevent https://en.wikipedia.org/wiki/Directory_traversal_attack
		//ifstream file(h.current_path.substr(1).c_str(),ios_base::binary);
		path=file_path+path;
		struct stat results;
		if(stat(path.c_str(),&results)==0){
			tm* ptm;
			ptm=gmtime(&results.st_mtime);//last modification time
			bool stale=true;
			auto j=h.headers.find("If-Modified-Since");
			if(j!=h.headers.end()){
				std::tm t={};
				//Last-Modified: Tue, 15 Nov 1994 12:45:26 GMT
				strptime(j->second.c_str(), "%A, %d %B %Y %T %Z", &t);//time zone is parsed but not used	
				//istringstream is(j->second);
				//only implemented in gcc 5.0
				//is >> std::get_time(&t, "%A, %d %B %T:%S");//what about time zone?
				stale=(mktime(&t)<mktime(ptm));//does that modify ptm??
			}
			if(stale){
				LOG<<"opening file `"<<path<<"'"<<endl;
				ifstream file(path.c_str(),ios_base::binary);
				string::size_type i=path.find_last_of('.');
				string extension;
				if(i!=string::npos) extension=path.substr(i+1);
				io<<"HTTP/1.1 200 OK"<<"\r\n";
				io<<"Content-Type:"<<get_mime(extension)<<"\r\n";
				io<<"Content-Length:"<<results.st_size<<"\r\n";
				char buffer[80];
				//just to be sure
				ptm=gmtime(&results.st_mtime);//last modification time
				strftime(buffer,80,"%A, %d %B %Y %T %Z",ptm);
				LOG<<"time stamp:"<<buffer<<endl;
				io<<"Last-Modified:"<<buffer<<"\r\n";
				io<<"\r\n";
				io<<file.rdbuf();
			}else{
				LOG<<"cached!"<<endl;
				io<<"HTTP/1.1 304"<<"\r\n";
				io<<"\r\n";
			}
			io.flush();
		}else{
			io<<http_404;
		}
	}
}	
void httpd::post(http_parser& h,iostream& io){
	/*
 	*	can we chain queries? it would make client code simpler
 	*	because we don't have to wait for reply and it guarantees synchronization
 	*	we know the request length
 	*
 	*/ 
	/*
 	*	might be better to read the whole POST message and parse it
 	*	it would be nice to have a generic parser that can use char*
 	*
 	*/
	try{
		io.exceptions(iostream::eofbit);
		LOG<<"attempting to get mutex...";
		#ifdef PTHREAD
		pthread_mutex_lock(&mutex);
		#else
		std::lock_guard<std::mutex> lock(m);
		#endif
		LOG<<"ok"<<endl;
		//problem: no EOF to tell the parser to stop
		/*
		if(!h.headers["Content-Length"].empty()){
			istringstream in(h.headers["Content-Length"]);
			int l=0;
			in>>l;
			string query;
		*/		
		sparql_parser p(io);
		LOG<<"peek:`"<<io.peek()<<"'"<<endl;
		//a lot of things happening in go, it can be interrupted anytime by socket exception
		if(p.go()){
			io<<"HTTP/1.1 200 OK\r\n";
			//io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			//check if client want csv format
			auto i=h.url_arguments.find("format");
			if(i->second=="csv"){
				p.out_csv(out);
				io<<"Content-Type: "<<"text/plain"<<"\r\n";
			}else{
				p.out(out);
				io<<"Content-Type: "<<(out.str().size() ? "text/xml":"text/plain")<<"\r\n";//an XML document can not be empty
			}
			io<<"Content-Length: "<<out.str().size()<<"\r\n";
			io<<"\r\n";
			io<<out.str();//for some reason throws here and not caught anywhere?
			io.flush();
			//there might be characters left in the pipe
			if(!h.headers["Content-Length"].empty()){
				istringstream in(h.headers["Content-Length"]);
				int l=0;
				in>>l;
				l-=p.n_read;
				LOG<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) LOG<<(char)io.get();
			}
		}else{
			LOG<<"could not parse sparql query\n";
			io<<http_400;
			//there might be characters left in the pipe
			if(!h.headers["Content-Length"].empty()){
				istringstream in(h.headers["Content-Length"]);
				int l=0;
				in>>l;
				l-=p.n_read;
				LOG<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) LOG<<(char)io.get();
			}
		}
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
	}catch(SocketRunTimeException& e){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG<<"http::post "<<e.what()<<endl;
	}catch(std::exception& e){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG<<"exception: "<<e.what()<<endl;
	}catch(...){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG<<"unknown exception!!!!!"<<endl;
	}
}
void httpd::put(http_parser& h,iostream& io){}	
void httpd::head(http_parser& h,iostream& io){}	
void httpd::delete_(http_parser& h,iostream& io){}
