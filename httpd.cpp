#include "httpd.h"
#include "rdf_xml_parser.h"
#include "sparql_engine.h"
#include "url_decoder.h"
#include <fstream>
#include <sys/stat.h>
using namespace objrdf;

string get_mime(string extension){
	typedef std::pair<string,string> P;
	static map<string,string> m={	P("html","text/html"),P("xhtml","text/html"),
					P("xml","text/xml"),P("xsd","text/xml"),P("xsl","text/xml"),
					P("rdf","application/rdf+xml"),
					P("svg","image/svg+xml"),
					P("png","image/png"),
					P("css","text/css"),
					P("js","application/javascript"),
					P("mat","application/octet-stream")};
	/*
	if((extension=="html")||(extension=="htm")) return "text/html";
	if((extension=="xml")||(extension=="xsd")||(extension=="xsl")||(extension=="rdf")) return "text/xml";
	if(extension=="svg") return "image/svg+xml";
	if(extension=="png") return "image/png";
	if(extension=="css") return "text/css";
	if(extension=="mat") return "application/octet-stream";
	*/
	auto i=m.find(extension);
	return i==m.end() ? "text/plain" : i->second;
}
string url_decode(string in){
	istringstream is(in);//we need to make the parser more generic
	url_decoder u(is);
	u.go();
	return u.decoded;	
}
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
	sockserver.listen(port);
	cerr << "http server is listening on port "<<port<< endl;
	for(;;){
		TCPSocketWrapper* sock=new TCPSocketWrapper(sockserver.accept());
		cerr<<"accepted connection from: "<<sock->address()<<" port:"<<sock->port()<<endl;
		request_info* r=new request_info(static_cast<httpd*>(s),sock);
		pthread_t shell_t;
		pthread_create(&shell_t,NULL,request,r);
	}
}
inline ostream& http_404(ostream& stream){
	stream<<"HTTP/1.1 404 file not found"<<endl;
	ostringstream o;
	char hostname[255];
	gethostname(hostname,255);
	o<<"<html><title>404 Not Found</title><body><h1>Not Found</h1>The requested URL was not found on this server.";
	o<<"<hr/><address>httpd Server at "<<hostname<<"</address></body></html>"<<endl;
	stream<<"Content-Length:"<<o.str().size()<<endl;
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
void* httpd::request(void* s){
	cerr<<"new request"<<endl;
	request_info* r=static_cast<request_info*>(s);
	try{
		TCPStream stream(*r->second);
		stream.exceptions(iostream::eofbit|iostream::badbit);
		while(stream.good()){
			cerr<<"peek: "<<stream.peek()<<endl;
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
				cerr<<"could not parse http header\n";
				stream<<"HTTP/1.1 400 Bad Request\r\n";
				stream.setstate(ios::badbit);
				stream.flush();
			}
			cerr<<"done!"<<endl;
		}
		cerr<<"ending thread"<<endl;
		delete r->second;
		delete r;//will attempt a flush here
		return 0;
	}catch(SocketRunTimeException& e){
		cerr<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		cerr<<"****** "<<e.what()<<endl;
	}catch(...){
		cerr<<"****** exception!"<<endl;
	}
	//return 0;
}
void httpd::get(http_parser& h,iostream& io){
	if(h.current_path.compare(0,6,"/data/")==0){
	/*
 	*	can only query local variables
 	*	problem with hash URI: the browser loads the whole document
 	*/
		string id=h.current_path.substr(4);
		shared_ptr<base_resource> r=doc.query(uri(id));
		if(!r){
			io<<http_404;
		}else{
			io<<"HTTP/1.1 200 OK\r\n";
			io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			out<<"<?xml-stylesheet type='text/xsl' href='/describe.xsl'?>";
			//not a valid RDF document, should use DESCRIBE query instead
			r->to_rdf_xml(out);
			io<<"Content-Length:"<<out.str().size()<<"\r\n";
			io<<"\r\n";
			io<<out.str();
			io.flush();
		}
	}else if(h.current_path.compare(0,3,"/r/")==0){
		string id=h.current_path.substr(3);
		shared_ptr<base_resource> r=doc.query(uri(id));
		if(!r){
			io<<http_404;
		}else{
			//io<<"HTTP/1.1 200 OK\r\n";
			//a resource can have its own content, could be a static/dynamic document
			r->get_output(io);
		}	
	}else if(h.current_path=="/sparql"){//sparql end-point
		auto i=h.url_arguments.find("query");
		if(i!=h.url_arguments.end()){
			istringstream is(url_decode(i->second));
			is.exceptions(iostream::eofbit);
			sparql_parser p(doc,is);
			if(p.go()){
				cerr<<"success!"<<endl;
				io<<"HTTP/1.1 200 OK"<<"\r\n";
				io<<"Content-Type:"<<get_mime("xml")<<"\r\n";
				ostringstream out;
				auto j=h.url_arguments.find("xsl");
				if(j!=h.url_arguments.end()) out<<"<?xml-stylesheet type='text/xsl' href='"<<j->second<<"'?>";
				p.out(out);
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
	}else{
		ifstream file(h.current_path.substr(1).c_str(),ios_base::binary);
		if(file){
			struct stat results;
			stat(h.current_path.substr(1).c_str(),&results);
			http_parser::M::iterator j=h.headers.find("If-Modified-Since");
			time_t t=0;
			if(j!=h.headers.end()){
				istringstream is(j->second);
				is>>t;
			}
			if(t<results.st_mtime){				
				string::size_type i=h.current_path.find_last_of('.');
				string extension;
				if(i!=string::npos) extension=h.current_path.substr(i+1);
				io<<"HTTP/1.1 200 OK"<<"\r\n";
				io<<"Content-Type:"<<get_mime(extension)<<"\r\n";
				io<<"Content-Length:"<<results.st_size<<"\r\n";
				//cache control
				//convert to gmt
				/*
				tm* ptm;
				ptm=gmtime(&results.st_mtime);
				char buffer[80];
				strftime(buffer,80,"%a, %d %b %Y %X %Z",ptm);
				io<<"Last-Modified:"<<buffer<<"\r\n";
				*/
				io<<"Last-Modified:"<<results.st_mtime<<"\r\n";
				io<<"\r\n";
				io<<file.rdbuf();
			}else{
				cerr<<"cached!"<<endl;
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
	/*
	if(!h.headers["Content-Length"].empty()){
		istringstream in(h.headers["Content-Length"]);
		int l=0;
		in>>l;
		string message=io.read(l);
	}else{
		//error
	}
	*/
	try{
		//throw SocketRunTimeException("test");
		pthread_mutex_lock(&mutex);
		sparql_parser p(doc,io);
		cerr<<"peek:`"<<io.peek()<<"'"<<endl;
		if(p.go()){
			io<<"HTTP/1.1 200 OK\r\n";
			//io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			p.out(out);
			//an XML document can not be empty
			io<<"Content-Type: "<<(out.str().size() ? "text/xml":"text/plain")<<"\r\n";
			io<<"Content-Length: "<<out.str().size()/*/2*/<<"\r\n";//to force close socket early
			io<<"\r\n";
			io<<out.str();//for some reason throws here and not caught anywhere?
			io.flush();
			//there might be characters left in the pipe
			if(!h.headers["Content-Length"].empty()){
				istringstream in(h.headers["Content-Length"]);
				int l=0;
				in>>l;
				l-=p.n_read;
				cerr<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) cerr<<(char)io.get();
			}
		}else{
			cerr<<"could not parse sparql query\n";
			io<<http_400;
			//there might be characters left in the pipe
			if(!h.headers["Content-Length"].empty()){
				istringstream in(h.headers["Content-Length"]);
				int l=0;
				in>>l;
				l-=p.n_read;
				cerr<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) cerr<<(char)io.get();
			}
		}
		pthread_mutex_unlock(&mutex);
	}catch(SocketRunTimeException& e){
		pthread_mutex_unlock(&mutex);
		cerr<<"http::post "<<e.what()<<endl;
	}
}
void httpd::put(http_parser& h,iostream& io){}	
void httpd::head(http_parser& h,iostream& io){}	
void httpd::delete_(http_parser& h,iostream& io){}
