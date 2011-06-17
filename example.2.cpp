#include "objrdf.h"
#include "rdf_xml_parser.h"
#include <iostream>
using namespace objrdf;

namespace example_2{
	RDFS_NAMESPACE("http://www.example.org/2#","ex")
	PROPERTY(p_0,int);
	PROPERTY(p_1,std::string);
	CLASS2(C,p_0,p_1);
}
int main(){
	rdf::RDF doc;
	doc.insert(example_2::C::get_class());
	rdf_xml_parser p(doc,std::cin);
	std::cerr<<p.go()<<std::endl;
	//doc.to_rdf_xml_pretty(std::cout);
	doc.to_rdf_xml_pretty(std::cout);
}


