/*
 *	add new function to the function table
 *
 *
 */
#include "objrdf.h"
using namespace objrdf;

RDFS_NAMESPACE("http://test.example.org/#","test")

PROPERTY(a,int);
PROPERTY(b,double);
CLASS(Test,std::tuple<a,b>);
int main(){
	Test::allocator al;
	auto ptr=al.allocate(1);
	al.construct(ptr,uri("test"));
	ptr->get<a>().t=123;
	ptr->get<b>().t=.123;
	to_rdf_xml(ptr,cout);
	//Test::v.push_back(Test::v[0]);
	Test::v.pop_back();
	to_rdf_xml(ptr,cout);
	//al.destroy(ptr);
	//al.deallocate(ptr,1);
	//access function table
	/*
	for(auto i=Test::v.cbegin();i<Test::v.cend();++i){
		cout<<i->p->id<<endl;
	}
	//create new function table
	objrdf::V v(Test::v.size());
	copy(Test::v.cbegin(),Test::v.cend(),v.begin());	
	*/	
}
