/*
 *	main application to work on database:
 *	dump
 *	dump_schema
 *	parser
 *	query
 *	server
 *	it should be possible to run inside chroot jail for security
 */
#include "objrdf.h"
#include "rdf_xml_parser.h"
#include "sparql_engine.h"
#include "httpd.h"
using namespace objrdf;
using namespace std;
void dump(){
	to_rdf_xml(cout);
}
void dump_schema(){
	cout<<"<"<<rdf::_RDF<<"\n";
	uri::ns_declaration(cout);
	cout<<">";
	for(auto j=::begin<PROPERTY_PTR>();j< ::end<PROPERTY_PTR>();++j) to_rdf_xml(*j,cout);
	for(auto j=::begin<CLASS_PTR>();j< ::end<CLASS_PTR>();++j) to_rdf_xml(*j,cout);
	cout<<"\n</"<<rdf::_RDF<<">\n";
}
void parser(){
	rdf_xml_parser p(cin);
	bool r=p.go();
	if(r){
		cerr<<"parsing success"<<endl;
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
}
void query(){
	cin.exceptions(iostream::eofbit);
	sparql_parser sp(cin);
	bool r=sp.go();
	if(r){
		cerr<<"parsing success"<<endl;
		sp.out(cout);
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
}
void server(){
	objrdf::httpd h;
	h.run();
}
void init();//defined in schema.so
int main(int argc,char* argv[]){
	typedef void (*fpt)();
	typedef std::pair<string,fpt> P;
	map<string,fpt> functions={P("dump",dump),P("dump_schema",dump_schema),P("parser",parser),P("query",query),P("server",server)};
	if(argc==1 && string(argv[0])=="objrdf_ctl"){
		for(auto i=functions.cbegin();i!=functions.cend();++i) cout<<i->first<<endl;
	}else{
		string command=(argc==1)? argv[0] : argv[1];
		auto i=functions.find(command);
		if(i!=functions.end()){
			init();
			i->second();
		}else{
			cerr<<"command `"<<command<<"' not found"<<endl;
			exit(1);
		}	

	}
}

