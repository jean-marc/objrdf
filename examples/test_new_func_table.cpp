/*
 *	investigate how to replace functions in table
 *
 *
 */
#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
typedef property<rdfs_namespace,str<'p','_','1'>,int> p_1;/* to make sure p_1 and p_0 use same functions */
typedef property<rdfs_namespace,str<'p','_','2'>,base_resource*> p_2;
typedef property<rdfs_namespace,str<'p','_','3'>,base_resource*> p_3;/* to make sure p_3 and p_2 use same functions */
typedef resource<
	rdfs_namespace,
	str<'A'>,
	std::tuple<
		//p_0,
		p_1,
		//p_2,
		p_3,
		objrdf::array<p_0>,
		objrdf::array<p_2>
	>
> A;

int main(){
	for(auto i:A::v)
		cout<<i<<endl;
	A a(uri("jm"));
	base_resource::do_index(base_resource::nil);
	A::do_index(&a);	
	//a.get<p_0>().t=1234;
	a.get<p_1>().t=5678;
	a.get<objrdf::array<p_0>>().push_back(p_0(1111));
	a.get<objrdf::array<p_0>>().push_back(p_0(1111));
	a.get<objrdf::array<p_0>>().push_back(p_0(1111));
	a.get<objrdf::array<p_2>>().push_back(p_2(new base_resource(uri("hi"))));
	a.get<objrdf::array<p_2>>().push_back(p_2(new base_resource(uri("bye"))));
	to_rdf_xml(&a,cout);
	rdf_xml_parser r(cin);
	r.go();
	to_rdf_xml(&a,cout);
	//to_rdf_xml(base_resource::nil,cout);	

}
