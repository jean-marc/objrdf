#define OBJRDF_H_IMPL_IN_HEADER
#include "objrdf.h"
#include <iostream>
using namespace objrdf;
RDFS_NAMESPACE("http://www.example.org/0#","ex")
PROPERTY(p,int);
CLASS1(CC,p);
int main(){
	cout<<base_resource::v.size()<<endl;
	CC a_C(std::string("a_C"));
	a_C.get<p>()=10;
	a_C.to_turtle(std::cout);
	CC::get_class()->to_turtle(cout);
	p::get_property()->to_turtle(cout);
	cout<<base_resource::get_class().get()<<endl;
	cout<<rdfs::Class::get_class().get()<<endl;
	p::get_property()->to_turtle(cout);
};
