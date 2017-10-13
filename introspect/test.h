#ifndef INTROSPECT_TEST_H
#define INTROSPECT_TEST_H
#include "objrdf.h"
namespace test{
using namespace objrdf;
	RDFS_NAMESPACE("http://test.example.org/#","test")
	typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
	typedef property<rdfs_namespace,str<'p','_','1'>,double> p_1;
	struct Test:resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<p_0,p_1>,Test>{
		Test(uri u):SELF(u){}
		void go(){
			cerr<<introspect<Test>::get_class()->id<<endl;
			cerr<<introspect<p_0>::get_property()->id<<endl;
			cerr<<introspect<p_1>::get_property()->id<<endl;
		}
	};
	typedef resource<rdfs_namespace,str<'T','e','s','t','d'>,std::tuple<>,NIL,Test> Testd;
	void go();
}
#endif
