#include <objrdf.h>	/* doc/example.0.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	t.get<a>().t=123;
	t.get<b>().t=.123;
	to_rdf_xml(cout);
}
