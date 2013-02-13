#include "objrdf.h"
#include <iostream>
using namespace objrdf;
RDFS_NAMESPACE("http://www.example.org/0#","ex")
PROPERTY(p,int);
CLASS1(C,p);
int main(){
	cout<<base_resource::v.size()<<endl;
	C a_C(uri("a_C"));
	a_C.get<p>()=10;
	functions<C,p>::get(&a_C).in(cin);
	//p::functions::in<C,p>(&a_C,cin,0);
	function_table t=functions<C,p>::get_table();
	//std::get<0>(t)=0;
	//std::get<1>(t)=p::in_g<C,p>;
	//std::get<2>(t)=p::out_g<C,p>;
	std::get<2>(t)(&a_C,cout,0);
	//p::get_object_g<C,p>(&a_C,0);
	a_C.to_turtle(std::cout);
	C::get_class()->to_turtle(cout);
	p::get_property()->to_turtle(cout);
	cout<<base_resource::get_class().get()<<endl;
	cout<<rdfs::Class::get_class().get()<<endl;
	p::get_property()->to_turtle(cout);
};
