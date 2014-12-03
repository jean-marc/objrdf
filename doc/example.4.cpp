#include <objrdf.h>	/* doc/example.4.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef resource<rdfs_namespace,str<'A'>,std::tuple<a>> A;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'B'>,std::tuple<b>,objrdf::NIL,A> B;
int main(){
	B t(uri("test"));
	base_resource::do_index(&t);
	t.get<a>()=123;
	t.get<b>()=.123;
	to_rdf_xml(cout);
}
