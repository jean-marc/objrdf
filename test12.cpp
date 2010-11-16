/*
	multiple instances of properties


*/
#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;
using namespace std;

PROPERTY(p,base_resource*);
CLASS1(C,p_array<p>);
int main(){
	C c;
	c.get<p_array<p> >().push_back(p(new base_resource));
	c.get<p_array<p> >().push_back(p(new base_resource));
	c.get<p_array<p> >().push_back(p(new base_resource));
	rdf::RDF doc;
	doc.insert(C::get_class());
	rdf_xml_parser pr(doc,cin);
	pr.go();
	doc.to_rdf_xml_pretty(cout);
	cout<<endl;

};

