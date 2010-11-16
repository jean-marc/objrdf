/*
	sparql query
*/
#include "objrdf.h"
#include "sparql.h"
#include "turtle_parser.h"
#include <iostream>
#include <fstream>
using namespace std;
using namespace objrdf;
struct _C_A;
struct _C_B;
struct _C_C;
struct _p_0;
struct _p_1;
struct _p_2;
typedef resource<_C_B> C_B;
typedef property<_p_0,C_B*> p_0;
typedef property<_p_1,double> p_1;
typedef property<_p_2,double> p_2;
typedef resource<_C_A,duo<p_0,p_1> > C_A;
typedef resource<_C_C,p_2,C_A> C_C;
SET(C_A);
SET(C_B);
SET(C_C);
SET(p_0);
SET(p_1);
SET(p_2);
int main(){
	RDF r;
	r.insert(C_A::c);
	r.insert(C_B::c);
	r.insert(C_C::c);
	r.introspect();
	turtle_parser p(r);
	cin>>p;
	r.to_turtle();
	sparql query(r);
	ifstream in("test11.rq");
	in>>query;
	cout<<query;
	query.root->serialize(cout);
	cout<<"\n";
	query.run();	
	query.to_xml();
} 
