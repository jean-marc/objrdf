#include "example.6.h"
#include "objrdf.h"
#include <iostream>
using namespace objrdf;
/*
PROPERTY(p,int);
struct _C;
template<typename T=NIL> struct C:resource<_C,p>{
	T t;
	C(){
		//T::go();
		t.go();
	}
};
*/
struct test{
	//static void go(){};
	void go(){};
	template<typename T> void get(){cout<<"hello\n";};
};
SET(p);
//SET(C);
typedef C<test> C_test;
//template<> objrdf::introspection<C_test>::value_type C_test::SELF::c=objrdf::set_id(objrdf::introspection<C_test>::go(),uri("C"));
SET(C_test);

int main(){
	C_test::c.second->to_turtle(std::cout);	
	C_test c;
	c.to_turtle(std::cout);
};
