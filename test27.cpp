/*
 *	save document and reload at next run
 */
#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

namespace test27{
	PROPERTY(a,int);
	PROPERTY(b,objrdf::base_resource*);
	CLASS2(C,a,b);
}

int main(){
	for(vector<rdfs::Class*>::iterator i=rdfs::Class::v.begin();i<rdfs::Class::v.end();++i)
		(*i)->to_turtle_pretty(cout);
	rdf::RDF doc;	
	doc.insert(test27::C::get_class());
	rdf_xml_parser p(doc,cin);
	p.go();
	cerr<<p.missing_object.size()<<" object not set "<<endl;//<<p.missing_object.begin()->first<<endl;	
	//do some processing
	doc.to_rdf_xml_pretty(cout);
	cout<<endl;
}	


