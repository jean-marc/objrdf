#include "sparql_engine.h"
#include "rdf_xml_parser.h"
#include "objrdf.h"
#include <fstream>

RDFS_NAMESPACE("http://www.example.org/#","ex");
struct C;
PROPERTY(p_0,C*);
PROPERTY(p_1,int);
PROPERTY(p_2,int);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,objrdf::tuple<p_array<p_0>,p_1,p_array<p_2> >::SELF>{
	C(uri u):SELF(u){}
};
int main(int argc,char* argv[]){
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(shared_ptr<C>(new C(uri("jm"))));
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		cerr<<"parsing `"<<argv[i]<<"'";
		rdf_xml_parser p(doc,in);
		cerr<<"\t"<<p.go()<<endl;
	}
	doc.to_rdf_xml_pretty(cout);
 	cin.exceptions(iostream::eofbit);
	sparql_parser sp(doc,std::cin);
	bool r=sp.go();
	if(r){
		cerr<<"parsing success"<<endl;
		sp.out(cout);
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
	//doc.to_rdf_xml_pretty(cout);
	return 0;
}
