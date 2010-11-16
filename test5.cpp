#include "objrdf.h"
#include "obj_xml_parser.h"
#include <stack>
/*
	parse an element
	<C_0>
	 <p_0>1212</p_0>
	 <p_1>1.333</p_1>
	 <p_2>
	  <Resource/>
	 </p_2>
	</C_0>
*/
using namespace objrdf;
struct _p_0;typedef property<_p_0,int> p_0;
//template<> Property* p_0::c=introspection<p_0>::go()->set_id("p_0");
SET(p_0);
struct _p_1;typedef property<_p_1,string> p_1;
//template<> Property* p_1::c=introspection<p_1>::go()->set_id("p_1");
SET(p_1);
struct _p_2;typedef property<_p_2,base_resource*> p_2;
//template<> Property* p_2::c=introspection<p_2>::go()->set_id("p_2");
SET(p_2);
struct _C_0;typedef resource<_C_0,duo<p_0,duo<p_1,p_2> > > C_0;
//template<> Class* C_0::c=introspection<C_0>::go()->set_id("C_0");
SET(C_0)
int main(){
	C_0 _c;
	obj_xml_parser p(&_c);
	char_iterator c(cin);
	p.go(c);
	_c.to_turtle(cout);
	cout<<endl;
	_c.to_rdf_xml(cout);
	cout<<endl;
	_c.to_xml(cout);
	cout<<endl;

}

