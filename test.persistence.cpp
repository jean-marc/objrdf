/*
 	file manipulation for objrdf data persistence
	file is 256 Bytes
	format bytes:
	1	rdfs:Class id
	m	offset to instance id string
	n	non-string, non-container properties (p_array<>)

*/
#define PERSIST
#include "persistence.h"
#include "objrdf.h"
using namespace objrdf;
using namespace std;
/*
struct C:resource<C>{
	int pos;
	C(){}
	C(persist& p):pos(0){
		//write to file asap
		//p.request(2);	
		//p.f.put(1);
		//p.request(2);	
		//p.f.put(1);
		p.to_binary(*this);
	}
};
*/
PROPERTY(p_0,short);
PROPERTY(p_1,string);
PROPERTY(p_2,base_resource*);
SET(p_0);
SET(p_1);
SET(p_2);
//CLASS3(C,p_array<p_0>,p_1,p_2);
CLASS3(C,p_0,p_1,p_2);
SET(C)
CLASS0(D);
SET(D);

int main () {
	cout<<"..."<<endl;
	rdfs::Class test;
	base_resource* tp=rdfs::Class::c.second->f();
	cout<<"..."<<endl;
	p_0::c->to_turtle(cout);
	//cout<<p_2::c->size<<endl;
	//fstream outfile;
	//outfile.open ("test.txt",ios_base::in|ios_base::out|ios_base::binary);
	//persist pr(outfile);
	C c(uri("a_c"));	
	//c.id="a_c";
	//c.get<p_0>()=0x1234;
	//c.test_get<p_0>()=0x1234;
	c.set<p_0>(0x1234);
	//c.get<p_array<p_0> >().push_back(0x1111);
	//c.get<p_array<p_0> >().push_back(0x2222);
	//c.get<p_array<p_0> >().push_back(0x3333);
	//c.get<p_1>().t="wassup";
	c.set<p_1>("wassup");
	base_resource* r=new base_resource(uri("pointer"));
	//pr.to_binary(*r);
	c.set<p_2>(r);
	c.to_turtle(cout);
	cout<<C::c.second->index<<endl;
/*
	pr.to_binary(c);
	++c.get<p_0>().t;
	pr.update(c,p_0::c);
	//D d;	
	//d.id="a_d";
	//d.to_turtle(cout);
	//pr.to_binary(d);
*/	persist::p.m[base_resource::c.second->index]=base_resource::c.second;
	persist::p.m[rdfs::Class::c.second->index]=rdfs::Class::c.second;
	persist::p.m[rdf::Property::c.second->index]=rdf::Property::c.second;
	persist::p.m[C::c.second->index]=C::c.second;
	persist::p.m[D::c.second->index]=D::c.second;
	persist::p.f.seekg(0);
	cout<<"reading from db.."<<endl;
	persist::p.from_binary();
	return 0;
}
