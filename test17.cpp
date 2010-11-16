#include "rdf.h"
#include "turtle_parser.h"
#include "sparql.h"

using namespace objrdf;


struct _C_A;typedef resource<_C_A> C_A;
struct _p;typedef property_array<_p,C_A*> p;
struct _C_B;typedef resource<_C_B,p> C_B;

SET(C_A);
SET(p);
SET(C_B);

int main(){
	C_B::offsets[1].help();
	cout<<*C_B::offsets[1].c<<endl;
	RDF doc;
	doc.insert(C_A::c);
	doc.insert(C_B::c);
	C_A a;
	a.id="0";
	C_B b;
	b.id="1";
	doc.insert(&a);
	doc.insert(&b);
	b.get<p>().t.push_back(p::P(&a));
	doc.to_turtle();
	sparql s(doc);
	cin>>s;
	s.run();
	s.to_xml();
	s.to_string();
	//we want to remove the last value
	//base_resource::iterator i=
	//base_resource::iterator i=++b.begin();	
	//base_resource::property_type::iterator j=i->begin();//should be i->begin() careful: the first property is the type!
	//i->erase(i->begin());
	doc.to_turtle();
}

