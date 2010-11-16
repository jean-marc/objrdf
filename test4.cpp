/*
test string
*/
#include "objrdf.h"
using namespace objrdf;
using namespace std;
struct _p_0;typedef property<_p_0,string> p_0;
struct _C;typedef resource<_C,p_0> C;
SET(p_0);
SET(C);
int main(){
	C c;
	cout<<c.get<p_0>().t<<endl;
	c.get<p_0>().t+="vero";
	cout<<c.get<p_0>().t<<endl;
};

