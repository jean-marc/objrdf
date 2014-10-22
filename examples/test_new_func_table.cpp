/*
 *	investigate how to replace functions in table
 *
 *
 */
#include "objrdf.h"
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
typedef property<rdfs_namespace,str<'p','_','1'>,int> p_1;
typedef resource<
	rdfs_namespace,
	str<'A'>,
	std::tuple<p_0,p_1>
> A;

int main(){
	for(auto i:A::v)
		cout<<i<<endl;
	A a(uri("jm"));
	a.get<p_0>().t=1234;
	a.get<p_1>().t=5678;
	to_rdf_xml(&a,cout);

}
