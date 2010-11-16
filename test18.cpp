//test smart pointer
#include "objrdf.h"
using namespace objrdf;

struct R:resource<R>{
	R(){cout<<"R()"<<"\t"<<this<<endl;}
	~R(){cout<<"~R()"<<"\t"<<this<<endl;}
};
PROPERTY(p,R*)
CLASS1(C,p)

SET(p)
SET(R)
SET(C)

int main(){
	for(;;){
		C c;
		c.get<p>().set(new R);
		cout<<c.get<p>().get()<<endl;
	}

}
