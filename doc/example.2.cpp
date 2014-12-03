#include <objrdf.h>	/* doc/example.2.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef resource<rdfs_namespace,str<'T','a','r','g','e','t'>,std::tuple<>> Target;
typedef property<rdfs_namespace,str<'a'>,Target*> a;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	Target d(uri("dest"));
	base_resource::do_index(&d);
	t.get<a>()=&d;
	to_rdf_xml(cout);
}
