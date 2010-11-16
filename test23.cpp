#include "objrdf.h"
#include "objrdf.impl.cpp"
using namespace objrdf;
int main(){
	rdfs::type::c->get<rdfs::range>().get();//that function is not defined????
	rdf::RDF doc;
	doc.insert(rdfs::type::c);
	doc.to_rdf_xml(cout);
};
