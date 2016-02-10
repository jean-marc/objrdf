/*
* basic test
*/
#include "objrdf.h"
#include "sparql_engine.h"
using namespace objrdf;
namespace test{	
	RDFS_NAMESPACE("http://www.example.org/#","test");
	typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
	typedef resource<rdfs_namespace,str<'A'>,std::tuple<p_0>> A;
}
int main(){
	test::A::get_class();
	test::A::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,uri("instance"));
	test::A::do_index(p);
	p->get<test::p_0>()=1234;
	to_rdf_xml(p,cout);
	cout<<endl;
	//run some SPARQL queries
	vector<string> queries={
		"PREFIX: <http://www.example.org/#>"
		"INSERT DATA{<instance> :p_0 12345 .}",
		"PREFIX: <http://www.example.org/#>\n"
		"INSERT DATA{\n"
		"	<instance> :p_0 12345;\n"
		"	:p_0 6789 .\n"
		"}",
		"DESCRIBE <#instance>"
	};
	for(auto q:queries){
		cerr<<"parsing query `"<<q<<"'..."<<endl;
		istringstream in(q);
		sparql_parser p(in);
		auto r=p.go();
		if(!r) exit(1);
		p.out(cout);
	}
}


