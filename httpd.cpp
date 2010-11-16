#include "httpd.h"
#include "rdf_xml_parser.h"
#include "sparql_parser.h"
#include <fstream>
#include <sys/stat.h>
using namespace objrdf;

string get_mime(string extension){
	if((extension=="html")||(extension=="htm")) return "text/html";
	if((extension=="xml")||(extension=="xsd")||(extension=="xsl")||(extension=="rdf")) return "text/xml";
	if(extension=="svg") return "image/svg+xml";
	if(extension=="png") return "image/png";
	if(extension=="css") return "text/css";
	if(extension=="mat") return "application/octet-stream";
	return "text/plain";
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
	cout << "http server is listening on port "<<port<< endl;
	for(;;){
		TCPSocketWrapper* sock=new TCPSocketWrapper(sockserver.accept());
		cout<<"accepted connection from: "<<sock->address()<<" port:"<<sock->port()<<endl;
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
		cout<<"ending thread"<<endl;
		delete r->second;
		delete r;//will attempt a flush here
		return 0;
	}catch(SocketRunTimeException& e){
		cout<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		cout<<"****** "<<e.what()<<endl;
	}catch(...){
		cout<<"****** exception!"<<endl;
	}
	//return 0;
}
void httpd::get(http_parser& h,iostream& io){
	if(h.current_path.compare(0,4,"/id/")==0){
		string id=h.current_path.substr(4);
		shared_ptr<base_resource> r=doc.query(id);
		if(!r){
			io<<http_404;
		}else{
			io<<"HTTP/1.1 200 OK\r\n";
			io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			out<<"<?xml-stylesheet type='text/xsl' href='/describe.xsl'?>";
			r->to_rdf_xml(out);
			io<<"Content-Length:"<<out.str().size()<<"\r\n";
			io<<"\r\n";
			io<<out.str();
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
	sparql_parser p(doc,io);
	if(p.go()){
		io<<"HTTP/1.1 200 OK\r\n";
		io<<"Content-Type: text/xml\r\n";
		ostringstream out;
		p.out(out);
		io<<"Content-Length: "<<out.str().size()<<"\r\n";
		io<<"\r\n";
		io<<out.str();
		io.flush();
		if(!h.headers["Content-Length"].empty()){
			istringstream in(h.headers["Content-Length"]);
			int l=0;
			in>>l;
			io.ignore(l-p.n_read);
		}
	}else{
		cerr<<"could not parse sparql query\n";
		io<<"HTTP/1.1 400 Bad Request\r\n";
		//could send more information
		io.flush();
		io.setstate(ios::badbit);
	}
}
void httpd::put(http_parser& h,iostream& io){}	
void httpd::head(http_parser& h,iostream& io){}	
void httpd::delete_(http_parser& h,iostream& io){}
