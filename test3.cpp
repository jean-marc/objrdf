/*
test scalability of tuple 
SIZE needs to be defined
all the properties have a different type
*/
#include "objrdf.h"
#include <sstream>
#ifndef SIZE
#define SIZE 20
#endif
using namespace objrdf;
template<int N> struct test{typedef duo<property<test,int>,typename test<N-1>::P> P;};
template<> struct test<1>{typedef property<test,int> P;};

template<typename T> struct test_introspect;
template<int N,typename T> struct test_introspect<property<test<N>,T> >{
	static rdf::Property* go(){
		std::ostringstream os;
		os<<"p_"<<N;
		rdf::Property* p=new rdf::Property;
		p->id=os.str();
		return p;
	}

};
template<typename UNIQUE,typename RANGE> rdf::Property* property<UNIQUE,RANGE>::c=test_introspect<property<UNIQUE,RANGE> >::go();
struct _C;
typedef resource<_C,test<SIZE>::P> C;
SET(C);

int main(){
	//C c;
	cout<<property<test<1>,int>::c<<endl;	
	//property<test<1>,int> pp;	
	//cout<<p::c<<endl;
	cout<<SIZE<<"\t"<<C::c.first.size()<<endl;
	//for(int i=0;i<C::c.second.size();++i) C::c.second.v[i].help();
	C c;
	c.to_turtle(cout);
};

