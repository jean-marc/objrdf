#include <objrdf.h>	/* doc/example.6.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef property<rdfs_namespace,str<'c'>,std::string> c;//pseudo-property
struct Test:resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>,Test>{
	Test(uri id):SELF(id){
		get<a>().t=1;
		get<b>().t=0.1;
		base_resource::do_index(this);
	}
	static void patch(V& _v){
		function_table t;
		t.get_size=[](CONST_RESOURCE_PTR){return size_t(1);};
		t.out=[](CONST_RESOURCE_PTR subject,ostream& os,size_t index){os<<"Hello RDF!";};	
		_v.push_back(property_info(c::get_property(),t));
	}
};
int main(){
	Test t(uri("test"));
	to_rdf_xml(cout);
}
