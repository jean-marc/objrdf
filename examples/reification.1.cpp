/*
 *	what about a dummy resource between the subject and the object
 *
 *
 */
#include "objrdf.h"
#include "reification.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

RDFS_NAMESPACE("http://test.example.org/#","test")
typedef resource<rdfs_namespace,str<'B'>,std::tuple<>> B;
/*
 *	how do we add information to b?
 */
typedef property<rdfs_namespace,str<'b'>,B::allocator_type::pointer> b;
/*
<rdf:Statement>
	<rdf:subject rdf:resource='#p'/>
	<rdf:predicate rdf:resource='#b'/>
	<rdf:object rdf:resource='#q'/>
</rdf:Statement>

 */
typedef property<rdfs_namespace,str<'m','e','t','a'>,int> meta;
typedef objrdf::reified<b,std::tuple<meta>> rb;
struct A:resource<rdfs_namespace,str<'A'>,
	std::tuple<rb>,
	A,
	base_resource,
	std::tuple<rb>
>{
	A(uri u):SELF(u){}
	//work around to set Statement rdf:subject property
	void set_p(rb r,allocator_type::pointer _this){
		get<rb>()=r;
		get<rb>().meta->get<rdf::subject>()=_this;
	}
};

int main(){
	A::get_class();
	B::get_class();
	rb::R::get_class();
	objrdf::generate_index();
	rdf_xml_parser p(cin);
	cerr<<"parsing: "<<p.go()<<endl;
	/*
	auto p=A::allocator_type().construct_allocate(uri("p"));
	auto q=B::allocator_type().construct_allocate(uri("q"));
	p->get<rb>()=q;
	*/
	to_rdf_xml(cout);

};




