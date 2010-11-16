#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,std::string);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,tuple<p_0,p_1>::SELF,C>{
	int p;
	C(){std::cout<<"####C"<<std::endl;}
	~C(){std::cout<<"~C"<<std::endl;}
};
int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	rdf_xml_parser p(doc,std::cin);
	std::cout<<p.go()<<std::endl;
	doc.to_rdf_xml(std::cout);
}
