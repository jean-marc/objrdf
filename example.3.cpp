#include "objrdf.h"
#include "rdf_xml_parser.h"
#include <iostream>
using namespace objrdf;

RDFS_NAMESPACE("http://www.example.org/3#","ex")
PROPERTY(p_0,int);
PROPERTY(p_1,std::string);
CLASS2(C,p_0,p_1);
PROPERTY(p_2,double);
PROPERTY(p_3,double);
char _D[]="D";typedef resource<rdfs_namespace,_D,std::tuple<p_2>,NIL,C> D;
char _E[]="E";typedef resource<rdfs_namespace,_E,std::tuple<p_3>,NIL,D> E;

int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(D::get_class());
	C a_C(uri("a_C"));
	a_C.get<p_0>()=1;
	a_C.get<p_1>().t="do";
	D a_D(uri("a_D"));
	a_D.get<p_0>()=2;
	//cerr<<std::tuple_index<p_0,std::tuple<p_2>>::value<<endl;
	a_D.get<p_1>().t="re";
	a_D.get<p_2>().t=3.14;	
	E a_E(uri("a_E"));
	//a_E.E::_help_<p_0>::VALUE::get<p_0>()=3;
	a_E.get<p_0>()=3;
	a_E.get<p_1>().t="re";
	a_E.get<p_2>()=3.14;	
	doc.to_rdf_xml_pretty(std::cout);
}

