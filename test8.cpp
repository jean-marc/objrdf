/*
test inheritance using xml parser
should accept documents like:
	<C>
	 <p_c><A/></p_c>
	</C>
or
	<C>
	 <p_c><B/></p_c>
	</C>

bug in parser with one-letter name
*/
#include "objrdf.h"
#include "obj_xml_parser.h"

using namespace objrdf;
struct _A;typedef resource<_A> AA;
SET(AA);
struct _p_b;typedef property<_p_b,int> p_b;
SET(p_b);
struct _B;typedef resource<_B,p_b,AA> BB;//B sub-class of A
SET(BB);
struct _p_c;typedef property<_p_c,AA*> p_c;
SET(p_c);
struct _C;typedef resource<_C,p_c> CC;
SET(CC);

int main(){
	CC r;	
	obj_xml_parser p(&r,cin);
	//p.m["BB"]=BB::c;//register a Class
	p.register_class<BB>();
	p.go();
	//down-casting
	AA* aa=r.get<p_c>().get();
	if(aa->get_Class()==BB::c.second){
		static_cast<BB*>(aa)->get<p_b>().t++;
	}
	r.to_xml(cout);
};
