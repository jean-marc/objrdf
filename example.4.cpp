#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

RDFS_NAMESPACE("http://www.example.org/4#","ex")
PROPERTY(p_0,int);
PROPERTY(p_1,std::string);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,std::tuple<p_0,p_1>,C>{
	int p;
	C(uri u):SELF(u){
		std::cout<<"C"<<std::endl;
	}
	~C(){
		std::cout<<"~C"<<std::endl;
	}
};
int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	rdf_xml_parser p(doc,std::cin);
	std::cout<<p.go()<<std::endl;
	doc.to_rdf_xml_pretty(std::cout);
}
