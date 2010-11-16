/*
 *	introduces the THIN <> template
 *	it returns a type that is exempt of all user-contributed code.
 *	typedef THIN<A>::ResultT B;
 *	Even though this new type B is distinct from A, from the RDF point of view
 *	it should be identical and share the same static A::c member
 *
 */
#include "objrdf.h"
using namespace objrdf;

PROPERTY(p,int);
struct A:resource<A,p>{
	double a;
};
struct B:resource<B,NIL,A>{};
SET(A);
SET(B);
SET(p);
typedef THIN<A>::ResultT TA;
//SET(TA);
typedef THIN<B>::ResultT TB;
SET(TB);
int main(){
	A a;
	a.to_rdf_xml_pretty(cout);
	cout<<endl;
	TA ta;
	ta.get<p>().t=3;
	ta.to_rdf_xml_pretty(cout);
	cout<<endl;
	TB tb;
	tb.to_rdf_xml_pretty(cout);
	cout<<endl;
	cout<<sizeof(a)<<"\t"<<sizeof(ta)<<"\t"<<sizeof(tb)<<endl;
};
