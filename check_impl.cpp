/*
* basic test
*/
#include "objrdf.h"
#include "rdf_xml_parser.h"
#include "sparql_engine.h"
#include "objrdf_time.h"
#include "introspect.h"
#include <set>
using namespace objrdf;
namespace test{	
	RDFS_NAMESPACE("http://www.example.org/#","test");
	typedef resource<rdfs_namespace,str<'B'>,std::tuple<>/*,NIL,base_resource,std::tuple<>,std::allocator<void>,true*/> B;
	typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
	typedef property<rdfs_namespace,str<'p','_','1'>,std::chrono::system_clock::time_point> p_1;
	typedef property<rdfs_namespace,str<'p','_','2'>,B::allocator_type::const_pointer> p_2;
	typedef property<rdfs_namespace,str<'p','_','3'>,int> p_3;
	typedef property<rdfs_namespace,str<'p','_','4'>,B::allocator_type::pointer> p_4;
	typedef resource<rdfs_namespace,str<'A'>,
		std::tuple<
			p_0,
			p_1,
			p_2,
			objrdf::array<p_3,std::allocator<p_3>>,
			objrdf::array<p_4,std::allocator<p_4>>
		>
	> A;
}
/*
namespace objrdf{
	template<> struct introspect<test::A>;
	template<> struct introspect<test::B>;
}
*/
namespace rdfs{
	int verbosity=7;
	const char _context_[]="rdfs";
}
namespace objrdf{
	int verbosity=7;
	const char _context_[]="objrdf";
}
namespace pool_allocator{
	int verbosity=7;
	const char _context_[]="pool";
}
int main(){
	introspect<test::A>::get_vtable();
	//RDF parsing
	{
		string s="<rdf:RDF\n"
		"xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'\n"
		"xmlns:test='http://www.example.org/#'\n"
		">\n"
		"<test:A rdf:ID='instance_0'>\n"
		"	<test:p_0>1234</test:p_0>\n"
		"	<test:p_1>2016-02-17T22:27:33.508-0800</test:p_1>\n"
		"	<test:p_1>0</test:p_1>\n"
		"	<test:p_2><test:B rdf:ID='instance_B'/></test:p_2>\n"
		"	<test:p_3>1</test:p_3>\n"
		"	<test:p_3>2</test:p_3>\n"
		"	<test:p_3>3</test:p_3>\n"
		"	<test:p_4><test:B/></test:p_4>\n"
		"	<test:p_4><test:B/></test:p_4>\n"
		"</test:A>\n"
		"</rdf:RDF>\n";
		istringstream in(s);
		rdf_xml_parser p(in);
		assert(p.go());		
	}	
	to_rdf_xml(introspect<rdfs::Class>::get_class(),cout);
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
		"	:p_0 6789;\n"
		"	:p_1 '0';\n"
		"	:p_1 '2016-02-17T22:27:33.508-0800' .\n"
		"}",
		"PREFIX: <http://www.example.org/#>\n"
		"DELETE DATA{\n"
		"	<instance_0> :p_0 1234 .\n"
		"}",

		"PREFIX: <http://www.example.org/#>\n"
		"DELETE DATA{\n"
		"	<instance_0> :p_2 <instance_B> .\n"
		"}",

		"DESCRIBE <#instance_0>"
	};
	for(auto q:queries){
		cerr<<"parsing query `"<<q<<"'..."<<endl;
		istringstream in(q);
		sparql_parser p(in);
		auto r=p.go();
		if(!r) exit(1);
		p.out(cout);
	}
	{
		//pool iterator
		std::set<uri> s;
		rdfs::Class::allocator_type a;
		find_if(a.cbegin(),a.cend(),[&](const rdfs::Class& c){
			cerr<<"\t"<<c.id<<endl;
			s.insert(c.id);
			return false;
		});
		assert(s.size()==a.size());
	}
	{
		//generic pool iterator
		rdfs::Class::allocator_type a;
		for(auto i=a.cbegin();i!=a.cend();++i){
			pool_allocator::pool::POOL_PTR p(i.get_cell_index(),0); //there is a mapping between Class and pools
			if(p->iterable){
				cerr<<i->id<<endl;
				std::set<uri> s;
				for(
					auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);
					j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);
					++j){
					cerr<<"\t"<<j->id<<endl;
					s.insert(j->id);
				}
				assert(s.size()==p->get_size_generic(*p));//could have nicer syntax
			}else{
				cerr<<"pool `"<<i->id<<"' not iterable "<<p->payload_offset<<endl;
			}
		}
	}
}
