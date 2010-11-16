/*

*/
#include "objrdf.h"
#include <iostream>
#include <vector>
using namespace std;
using namespace objrdf;
typedef duo<char,duo<int,duo<double,long> > > D;
struct _F_;typedef resource<_F_,D> F;
struct _p_0;typedef property<_p_0,int> p_0;
SET(F)
SET(p_0)
int main(){
	p_0 p;
	cout<<sizeof(fpt)<<endl;
	cout<<get_size<D>::VALUE<<endl;	
	cout<<get_n<D>::VALUE<<endl;	
	F f;
	cout<<sizeof(f)<<endl;
	cout<<get_offset<D,char>::VALUE<<endl;
	cout<<get_offset<D,int>::VALUE<<endl;
	cout<<get_offset<D,double>::VALUE<<endl;
	f.get<char>()='a';
	f.get<int>()=10;
	f.get<double>()=1.;
	f.get<long>()=123;
	cout<<f.get<char>()<<"\t"<<f.get<int>()<<"\t"<<f.get<double>()<<"\n";
	//for(base_resource::type_iterator i=f.begin();i<f.end();++i) i->out();
}
