/*
 *	single threaded httpd to exercise the http parser
 *
 */
#include "http_parser.h"
//#include "Sockets.h"

int main(){
	http_parser c(cin);
	cin.exceptions(iostream::eofbit);
	cout<<c.n_read<<endl;
	cout<<"parsing result: "<<c.go()<<endl;
	cout<<c.n_read<<endl;
	/*TCPSocketWrapper sockserver;
	enum{port=1080};
	sockserver.listen(port);
	cout << "http server is listening on port "<<port<< endl;
	for(;;){
		TCPSocketWrapper sock(sockserver.accept());
		cout<<"accepted connection from: "<<sock.address()<<" port:"<<sock.port()<<endl;
		TCPStream stream(sock);
		generic_http_parser p(stream);
		if(p.go()){
			cout<<"HTTP/1.1 200 OK\n";
			stream<<"HTTP/1.1 200 OK\r\n";
		}else{
			stream<<"HTTP/1.1 400 Bad Request\r\n";
		}

	}
	*/
}
