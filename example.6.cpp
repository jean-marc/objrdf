#include "objrdf.h"
#include <stdio.h>
#include <iostream>
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,int);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,duo<p_0,p_1>,C>{
	void set(p_0 p){get<p_0>()=p;}
	void set(p_1 p){}
};
int main(){
	C c;
	c.begin()->begin()->in(cin);
	//c.get<p_0>().t=0;
	c.get<p_1>().t=1;
	//printf("%d\n",help<cb<p_0>,p_0>::VALUE);
	c.to_turtle(std::cout);
}
