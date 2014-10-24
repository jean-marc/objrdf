#include <objrdf.h>	/* doc/example.3.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<objrdf::array<a>>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	t.get<objrdf::array<a>>().push_back(a(123));
	t.get<objrdf::array<a>>().push_back(a(456));
	to_rdf_xml(cout);
}
