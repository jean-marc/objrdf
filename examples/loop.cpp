/*
 *	how to handle recursive definitions
 *	can we use forward declaration? that seems to work
 *
 */

#include "objrdf.h"
using namespace objrdf;

namespace ex{
	RDFS_NAMESPACE("http://www.example.org/#","ex");
	class A;
	PROPERTY(pp,volatile_allocator_managed<A>::pointer);
	char _A[]="A";
	class A:public resource<
		rdfs_namespace,
		_A,
		std::tuple<pp>,
		A,
		base_resource,
		volatile_allocator_managed<void>
	>{
	public:
		A(uri u):SELF(u){}
	};
}
int main(){
	ex::A::get_class();//very important!!
	auto p=ex::A::allocator_type::construct_allocate(uri("jm"));
	p->get<ex::pp>()=p;
	to_rdf_xml(p,cout);
	cout<<endl;
}
	
	
