#include <objrdf.h>	/* doc/example.5.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
struct Test:resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>,Test>{
	Test(uri id):SELF(id){
		get<a>().t=1;
		get<b>().t=0.1;
		base_resource::do_index(this);
	}
};
int main(){
	Test t(uri("test"));
	to_rdf_xml(cout);
}
