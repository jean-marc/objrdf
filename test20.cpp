/*
 *	test new method for callbacks with p_array
 *
 */
#include "objrdf.h"
namespace objrdf{
	//template<typename T> struct help<p_array<T>,T>{enum{VALUE=1};};
}
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,int);
struct C:resource<C,duo<p_0,p_array<p_1> > >{
	//DEFAULT_SET;
	//void _set(p_0 p){get<p_0>().t=p.t+1;}
};
SET(p_0);
SET(p_1);
SET(C);
int main(){
	C c;
	c.get<p_0>()=123;
	c.get<p_array<p_1> >().push_back(123);
	c.get<p_array<p_1> >().push_back(456);
	c.get<p_array<p_1> >().push_back(789);
	c._set(p_0(123));
	c._set(p_1(123));
	c.to_turtle(std::cout);
}
