#include "objrdf.h"
/*

*/

using namespace objrdf;
PROPERTY(p_0,int);
PROPERTY(p_1,int);
PROPERTY(p_2,int);
struct _A;
//typedef resource<_A,duo<p_0,duo<p_1,p_2> > > A;
typedef resource<_A,duo<duo<p_0,p_1>,p_2> > A;

SET(p_0);
SET(p_1);
SET(p_2);
SET(A);

int main(){
	A a;
	a.get<p_0>().t=0;
	a.get<p_1>().t=1;
	a.get<p_2>().t=2;
	a.to_turtle(cout);

}
