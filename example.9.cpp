#include "objrdf.h"
#include "rdf_xml_parser.h"
#include <iostream>
using namespace objrdf;

PROPERTY(p_0,int);
PROPERTY(p_1,std::string);
CLASS2(C,p_0,p_1);
PROPERTY(p_2,double);
//struct _D;typedef resource<_D,p_2,C> D;
char _D[]="D";
struct D:resource<rdfs_namespace,_D,p_2,D,C>{
	void set(p_2){}
};
int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(D::get_class());
	shared_ptr<C> a_C=new C(std::string("a_C"));
	a_C->get<p_0>()=1;
	a_C->get<p_1>().t="do";
	doc.insert(a_C);
	shared_ptr<D> a_D=new D;//(std::string("a_D"));
	a_D->get<p_0>()=2;
	a_D->get<p_1>().t="re";
	a_D->get<p_2>()=3.14;	
	doc.insert(a_D);
	doc.to_turtle(std::cout);
	cout<<"here"<<endl;
}

