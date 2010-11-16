#include "objrdf.h"
#include "rdf_xml_parser.h"
#include <iostream>
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,std::string);
CLASS2(C,p_0,p_1);

int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	rdf_xml_parser p(doc,std::cin);
	std::cout<<p.go()<<std::endl;
	doc.to_rdf_xml(std::cout);
}


