#include <objrdf.h>	/* doc/example.1.cpp */
#include <rdf_xml_parser.h>
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>> Test;
int main(){
	Test::get_class();
	rdf_xml_parser r(cin);
	r.go();
	to_rdf_xml(cout);
}
