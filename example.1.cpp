#include "objrdf.h"
#include <iostream>
using namespace objrdf;

namespace example_1{
	RDFS_NAMESPACE("http://www.example.org/1#","ex")
	PROPERTY(p_0,int);
	PROPERTY(p_1,string);
	CLASS2(C,p_0,p_1);
}
using namespace example_1;
int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(p_0::get_property());
	doc.insert(p_1::get_property());
	shared_ptr<C> a_C(new C(uri("a_C")));
	a_C->get<p_0>()=1;
	a_C->get<p_1>().t="test";
	doc.insert(a_C);//better
	doc.to_rdf_xml_pretty(std::cout);
}

