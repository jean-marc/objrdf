/*
* basic test
*/
#include "objrdf.h"
#include "rdf_xml_parser.h"
#include "sparql_engine.h"
#include "objrdf_time.h"
#include <set>
using namespace objrdf;
namespace test{	
	RDFS_NAMESPACE("http://www.example.org/#","test");
	typedef property<rdfs_namespace,str<'p','_','0'>,int> p_0;
	typedef property<rdfs_namespace,str<'p','_','1'>,std::chrono::system_clock::time_point> p_1;
	typedef resource<rdfs_namespace,str<'A'>,std::tuple<p_0,p_1>> A;
}
int main(){
	test::A::get_class();
	//RDF parsing
	{
		string s="<rdf:RDF\n"
		"xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'\n"
		"xmlns:test='http://www.example.org/#'\n"
		">\n"
		"<test:A rdf:ID='instance_0'>\n"
		"	<test:p_0>0</test:p_0>\n"
		"	<test:p_1>2016-02-17T22:27:33.508-0800</test:p_1>\n"
		"	<test:p_1>0</test:p_1>\n"
		"</test:A>\n"
		"</rdf:RDF>\n";
		istringstream in(s);
		rdf_xml_parser p(in);
		assert(p.go());		
	}	
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
		"	<instance> :p_0 6789 .\n"
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
