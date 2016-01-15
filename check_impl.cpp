/*
* basic test
*/
#include "objrdf.h"
using namespace objrdf;
namespace test{	
	RDFS_NAMESPACE("http://www.example.org/#","test");
	typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
	typedef resource<rdfs_namespace,str<'A'>,std::tuple<p_0>> A;
}
int main(){
	test::A::get_class();
	test::A::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,uri("instance"));
	p->get<test::p_0>()=1234;
	to_rdf_xml(p,cout);
}


