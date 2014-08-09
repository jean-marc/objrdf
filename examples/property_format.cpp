/*
 *	modify the behavior of a property without patching
 *	the function table, it is a bit complicated as it involves template specialization
 *	but the code is very short
 */
#include "objrdf.h"
using namespace objrdf;

RDFS_NAMESPACE("http://test.example.org/#","test")
template<typename T> struct adapter{};//could be any name
namespace objrdf{
	//specialize template
	template<typename T> struct base_property<adapter<T>>:base_property<T>{
		void out(ostream& os)const{os<<hex<<this->t;}
	};
}
//we want a to be serialized as hex
PROPERTY(a,adapter<int>);
CLASS(Test,std::tuple<a>);
int main(){
	Test::get_class();
	auto t=Test::allocator_type::construct_allocate(uri("jm"));
	t->get<a>().t=16;
	to_rdf_xml(t,cout);
}
