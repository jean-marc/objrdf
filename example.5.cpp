/*
 *	modify behaviour:
 *	run-time: patch function table, we need to find the right entry
 *	compile-time: specialize templates
 *
 */
#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;
RDFS_NAMESPACE("http://www.example.org/5#","ex")
PROPERTY(p_0,int);
CLASS1(C,p_0);
void out(base_resource* subject,ostream& os,size_t){os<<"hello";}
int main(){
	std::get<2>(C::v[0].t)=out;
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(p_0::get_property());
	doc.insert(shared_ptr<base_resource>(new C(uri("a_C"))));
	rdf_xml_parser p(doc,cin);
	cout<<p.go()<<endl;
	doc.to_rdf_xml_pretty(cout);
	cout<<endl;
}
