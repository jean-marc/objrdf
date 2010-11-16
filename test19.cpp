/*
 *	test new method for callbacks
 *
 */
#include "objrdf.h"
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,int);
struct C:resource<C,duo<p_0,p_1> >{
	CALLBACK;
	void _set(p_0 p){get<p_0>().t=p.t+1;}
};
SET(p_0);
SET(p_1);
SET(C);
int main(){
	C c;
	//c.get<p>()=123;
	c._set(p_0(123));
	c._set(p_1(123));
	c.to_turtle(std::cout);
}
