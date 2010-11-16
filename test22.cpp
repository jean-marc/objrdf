/*
	bug with pointer properties and reference counting

*/
#include "objrdf.h"
using namespace objrdf;

PROPERTY(p,base_resource*);
SET(p);
void test(p _p){
	cout<<_p->n<<endl;
}
int main(){
	base_resource* r=new base_resource();
	rdf::RDF doc;
	doc.insert(p::c);
	doc.insert(r);
	doc.to_turtle(cout);
	cout<<r->n<<endl;
	p _p(r);
	test(p(r));
	cout<<r->n<<endl;
	doc.to_turtle(cout);
}
