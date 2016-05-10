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
httpd::httpd(){
	auto s=getenv("HTTPD_PORT");
	port=s ? atoi(s) : 80;
	socketsInit();
}
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
	LOG_NOTICE << "http server is listening on port "<<port<< endl;
	for(;;){
		TCPSocketWrapper* sock=new TCPSocketWrapper(sockserver.accept());
		LOG_NOTICE<<"accepted connection from: "<<sock->address()<<" port:"<<sock->port()<<endl;
		request_info* r=new request_info(static_cast<httpd*>(s),sock);
		pthread_t shell_t;
		pthread_create(&shell_t,NULL,request,r);
	}
}
#else
void httpd::run(){
	TCPSocketWrapper sockserver;
	sockserver.listen(port);
	LOG_NOTICE<<"HTTPD server listening on port:"<<port<<endl;
	for(;;){
		TCPSocketWrapper::TCPAcceptedSocket sock(sockserver.accept());
		auto t=std::thread(&httpd::process,this,sock);
		t.detach();
	}
}
void httpd::process(TCPSocketWrapper::TCPAcceptedSocket sock){
	try{
		TCPSocketWrapper _sock(sock);
		LOG_NOTICE<<"HTTPD accepted connection from: "<<_sock.address()<<" port:"<<_sock.port()<<endl;
		TCPStream stream(_sock);
		stream.exceptions(iostream::eofbit|iostream::badbit);
		while(stream.good()){
			LOG_DEBUG<<"peek: "<<stream.peek()<<endl;
			http_parser p(stream);
			//would be nice to display URL
			if(p.go()){//blocking
				LOG_NOTICE<<_sock.address()<<" \""<<p.current_method<<" "<<p.current_path<<" "<<p.version<<"\""<<endl;
				/*
				*	we need to add modules here instead of this
				*/ 
				if(p.current_path.find("/rest")==0){
					rest(p,stream);
				/*}else if(p.current_path.find("/sparql")==0){
					sparql(p,stream);*/
				}else if(p.current_method=="GET") get(p,stream);
				else if(p.current_method=="POST"){
					post(p,stream);
				}
				else{
					stream<<"HTTP/1.1 400 Bad Request\r\n";
					stream.setstate(ios::badbit);//maybe not needed
					stream.flush();
				}
			}else{
				LOG_ERROR<<_sock.address()<<" \""<<p.current_method<<" "<<p.current_path<<"\" 400"<<endl;
				stream<<"HTTP/1.1 400 Bad Request\r\n";
				stream.setstate(ios::badbit);
				stream.flush();
			}
			LOG_DEBUG<<"done!"<<endl;
			//HTTP/1.0 does not support keep-alive
			if(p.version=="HTTP/1.0") break;
		}
		LOG_DEBUG<<"ending thread"<<endl;
	}catch(SocketRunTimeException& e){
		LOG_INFO<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		LOG_INFO<<"****** "<<e.what()<<endl;
	}catch(...){
		LOG_INFO<<"****** exception!"<<endl;
	}

}
#endif
inline ostream& httpd::http_404(ostream& stream){
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
	LOG_NOTICE<<"new request"<<endl;
	request_info* r=static_cast<request_info*>(s);
	try{
		TCPStream stream(*r->second);
		stream.exceptions(iostream::eofbit|iostream::badbit);
		while(stream.good()){
			LOG_DEBUG<<"peek: "<<stream.peek()<<endl;
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
				LOG_ERROR<<"could not parse http header\n";
				stream<<"HTTP/1.1 400 Bad Request\r\n";
				stream.setstate(ios::badbit);
				stream.flush();
			}
			LOG_DEBUG<<"done!"<<endl;
		}
		LOG_DEBUG<<"ending thread"<<endl;
		delete r->second;
		delete r;//will attempt a flush here
		return 0;
	}catch(SocketRunTimeException& e){
		LOG_INFO<<"****** "<<e.what()<<endl;
	}catch(SocketLogicException& e){
		LOG_INFO<<"****** "<<e.what()<<endl;
	}catch(...){
		LOG_INFO<<"****** exception!"<<endl;
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
		/*
 		*	let's pass the header to the function
 		*/ 
		string id=h.current_path.substr(3);
		CONST_RESOURCE_PTR r=find(uri(id));
		if(!r){
			io<<http_404;
		}else{
			//io<<"HTTP/1.1 200 OK\r\n";
			//a resource can have its own content, could be a static/dynamic document
			get_output(r,h.url_arguments,io); //not protected by mutex
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
			std::lock_guard<std::mutex> lock(m);
			if(p._go<seqw<sparql_parser::document,char_p<EOF>>>()){
				LOG_DEBUG<<"success!"<<endl;
				io<<"HTTP/1.1 200 OK"<<"\r\n";
				ostringstream out;
				auto i=h.url_arguments.find("format");
				if(i!=h.url_arguments.end() && i->second=="csv"){
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
		LOG_NOTICE<<"running command `"<<command<<"'"<<endl;
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
		//should be able to handle directory!
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
				LOG_DEBUG<<"time stamp:"<<buffer<<endl;
				io<<"Last-Modified:"<<buffer<<"\r\n";
				io<<"\r\n";
				io<<file.rdbuf();
			}else{
				LOG_NOTICE<<"cached!"<<endl;
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
		LOG_DEBUG<<"attempting to get mutex...";
		#ifdef PTHREAD
		pthread_mutex_lock(&mutex);
		#else
		std::lock_guard<std::mutex> lock(m);
		#endif
		LOG_DEBUG<<"ok"<<endl;
		//problem: no EOF to tell the parser to stop
		/*
		if(!h.headers["Content-Length"].empty()){
			istringstream in(h.headers["Content-Length"]);
			int l=0;
			in>>l;
			string query;
		*/		
		//need to modify so we can only post to /sparql
		sparql_parser p(io);
		LOG_DEBUG<<"peek:`"<<io.peek()<<"'"<<endl;
		//a lot of things happening in go, it can be interrupted anytime by socket exception
		if(p.go()){
			io<<"HTTP/1.1 200 OK\r\n";
			//io<<"Content-Type: text/xml\r\n";
			ostringstream out;
			//check if client want csv format
			auto i=h.url_arguments.find("format");
			if(i!=h.url_arguments.cend()&&i->second=="csv"){
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
				LOG_INFO<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) LOG_DEBUG<<(char)io.get();
			}
		}else{
			LOG_ERROR<<"could not parse sparql query\n";
			io<<http_400;
			//there might be characters left in the pipe
			if(!h.headers["Content-Length"].empty()){
				istringstream in(h.headers["Content-Length"]);
				int l=0;
				in>>l;
				l-=p.n_read;
				LOG_DEBUG<<"ignoring "<<l<<" extra characters:";
				for(int i=0;i<l;++i) LOG_DEBUG<<(char)io.get();
			}
		}
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
	}catch(SocketRunTimeException& e){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG_INFO<<"http::post "<<e.what()<<endl;
	}catch(std::exception& e){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG_INFO<<"exception: "<<e.what()<<endl;
	}catch(...){
		#ifdef PTHREAD
		pthread_mutex_unlock(&mutex);
		#endif
		LOG_INFO<<"unknown exception!!!!!"<<endl;
	}
}
void httpd::sparql(http_parser& h,iostream& io){

}
void httpd::rest(http_parser& h,iostream& io){
	if(h.current_method!="GET"){
		io<<"HTTP/1.1 405 Method Not Allowed"<<endl;
		io<<"\r\n";
		io.flush();
		return;
	}
	//let's do some crude parsing of the path
	vector<string> tokens;
	istringstream in(h.current_path);
	for(std::string each;std::getline(in,each,'/'); tokens.push_back(each));
	for(auto t:tokens) cerr<<t<<"\n";
	//we need to be able to detect array properties so we can serialize as [a,b,c,...]
	//let's start with JSON serialization, let's select all rdfs::Classes
	/*
 	* 	/rest	all the classes
 	*	/rest/rdfs:Class
 	*	/rest/some_class/id
 	*/ 	
	//always start with a rdfs:Class
	vector<CONST_CLASS_PTR> _classes;
	vector<CONST_RESOURCE_PTR> _instances;
	if(tokens.size()>2){
		auto c=find_t<rdfs::Class>(uri::qname_uri(tokens[2]));
		if(c){
			 _classes={c};
		}else{
			io<<http_404;
			return;
		}
		if(tokens.size()>3){
			//maybe unique id present, we might get inconsistent state where instance does not belong to class
			auto i=objrdf::find(uri::qname_uri(tokens[3]));
			if(i){
				_instances={i};
			}else{
				io<<http_404;
				return;
			}
		}
	}else{//copy all classes
		rdfs::Class::allocator_type a;
		for(auto i=a.cbegin();i!=a.cend();++i) _classes.push_back(CONST_CLASS_PTR(i));
	}
	bool html=true;
	ostringstream out;
	if(html){
		out<<"<html xmlns='http://www.w3.org/1999/xhtml'><head><title></title>";
		auto j=h.url_arguments.find("css");
		if(j!=h.url_arguments.end())
			out<<"<link rel='stylesheet' type='text/css' href='"<<j->second<<"' media='all'/>";
		out<<"</head><body><pre>";//so we can render svg
	}
	out<<"{\""<<rdfs::Class::get_class()->id<<"\" :{\n";
	for(auto i=_classes.cbegin();i!=_classes.cend();++i){
		if(i!=_classes.cbegin()) out<<",\n";
		out<<"\t\""<<(*i)->id<<"\":{\n";//which attributes do we serialize?
		if(_instances.empty()){//iterate through all instances
			pool_allocator::pool::POOL_PTR p((*i).index,0); //there is a mapping between Class and pools
			if(p->iterable){
				for(
					auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);
					j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);
					++j){
						if(j!=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p)) out<<",\n";
						out<<"\t\t\""<<j->id<<"\":{\n";
						for(auto k=cbegin(j);k!=cend(j);++k){
							auto size=k.get_size();//iterator should support that
							if(size==1){
								if(k!=cbegin(j)) out<<",\n";	
								out<<"\t\t\t\""<<k->get_Property()->id<<"\":";
								if(k->get_Property()->cget<rdfs::range>()==rdfs::XML_Literal::get_class()){
									if(html)
										out<<*k->cbegin();
									else
										out<<"\"XML content...\"";
								}else if(k->literalp())
									out<<"\""<<*k->cbegin()<<"\"";
								else{
									auto target=k->cbegin()->get_const_object();
									if(html)
										out<<"\"<a href='/rest/"<<objrdf::get_class(target)->id<<"/"<<target->id<<"'>"<<target->id<<"</a>\"";
									else
										out<<"\""<<target->id<<"\"";
								}
							}else if(size>1){
								if(k!=cbegin(j)) out<<",\n";	
								out<<"\t\t\t\""<<k->get_Property()->id<<"\":[";
								if(k->literalp()){
									for(base_resource::const_instance_iterator l=k->cbegin();l!=k->cend();++l){
										if(l!=k->cbegin()) out<<",";
										out<<"\""<<*l<<"\"";
									}
								}else{
									for(base_resource::const_instance_iterator l=k->cbegin();l!=k->cend();++l){
										if(l!=k->cbegin()) out<<",";
										auto target=l->get_const_object();
										if(html)
											out<<"\"<a href='/rest/"<<objrdf::get_class(target)->id<<"/"<<target->id<<"'>"<<target->id<<"</a>\"";
										else
											out<<"\""<<target->id<<"\"";
									}
								}
								out<<"]";
							}
						}
						out<<"\n\t\t}";
				}	
			}
		}else{//serialize single instance
			//	/rest/some_class/some_instance
			for(const auto& j:_instances){
				for(auto k=cbegin(j);k!=cend(j);++k){
					auto size=k.get_size();//iterator should support that
					if(size==1){
						if(k!=cbegin(j)) out<<",\n";	
						out<<"\t\t\""<<k->get_Property()->id<<"\":";
						if(k->get_Property()->cget<rdfs::range>()==rdfs::XML_Literal::get_class()){
							if(html)
								out<<*k->cbegin();
							else
								out<<"\"XML content...\"";
						}else if(k->literalp())
							out<<"\""<<*k->cbegin()<<"\"";
						else{
							auto target=k->cbegin()->get_const_object();
							if(html)
								out<<"\"<a href='/rest/"<<objrdf::get_class(target)->id<<"/"<<target->id<<"'>"<<target->id<<"</a>\"";
							else
								out<<"\""<<target->id<<"\"";
						}
					}else if(size>1){
						if(k!=cbegin(j)) out<<",\n";	
						out<<"\t\t\""<<k->get_Property()->id<<"\":[";
						if(k->literalp()){
							for(base_resource::const_instance_iterator l=k->cbegin();l!=k->cend();++l){
								if(l!=k->cbegin()) out<<",";
								out<<"\""<<*l<<"\"";
							}
						}else{
							for(base_resource::const_instance_iterator l=k->cbegin();l!=k->cend();++l){
								if(l!=k->cbegin()) out<<",";
								auto target=l->get_const_object();
								if(html)
									out<<"\"<a href='/rest/"<<objrdf::get_class(target)->id<<"/"<<target->id<<"'>"<<target->id<<"</a>\"";
								else
									out<<"\""<<target->id<<"\"";
							}
						}
						out<<"]";
					}
				}

			}
		}
		out<<"\n\t}";
	}
	out<<"\n}}\n";
	if(html){
		out<<"</pre></body></html>";
	}
	io<<"HTTP/1.1 200 OK\r\n";
	if(html)
		io<<"Content-Type: "<<"text/html"<<"\r\n";
	else
		io<<"Content-Type: "<<"application/json"<<"\r\n";
	io<<"Content-Length: "<<out.str().size()<<"\r\n";
	io<<"\r\n";
	io<<out.str();//for some reason throws here and not caught anywhere?
	io.flush();
}	
void httpd::put(http_parser& h,iostream& io){}	
void httpd::head(http_parser& h,iostream& io){}	
void httpd::delete_(http_parser& h,iostream& io){}
