/*
 *	reification is a nice concept to add information about triples,
 *		.provenance, ownership,....
 *		.domain specific information
 *	let us see how we could make a statement about a property with the currently
 *	available tools
 *	note: we already have pointers to triple: base_resource::instance_iterator
 *	what about sparql: how do we query a statement?	http://www.w3.org/2001/sw/DataAccess/rq23/#queryReification
 *
 */
#include "objrdf.h"
using namespace objrdf;

RDFS_NAMESPACE("http://test.example.org/#","test")
char _p[]="p";
PROPERTY(g,int);
PROPERTY(version,int);
#if 0
CLASS(Statement,std::tuple<version>);
struct p:property<rdfs_namespace,_p,int>{
	/*
 	*	we want to be able to make a statement about this triple,eg: date the statement was made
 	*	could we make it inherit from resource?
 	*	what would be nice is to have statements totaly separated from triples
 	*	so we wouldn't have to modify properties but maybe inefficient
 	*	what about pointer to resource
 	*/ 
	Statement::allocator_type::pointer _p;
	p(){
		Statement::get_class();//make sure everything is ready
		Statement::allocator_type a;
		_p=a.allocate(1);
	}		

};
#endif
namespace rdf{
	char _Statement[]="Statement";
	PROPERTY(subject,base_resource::allocator_type::pointer);
	PROPERTY(predicate,rdf::Property::allocator_type::pointer);
	PROPERTY(object,base_resource::allocator_type::pointer);//no literal for now
	//char _Statement[]="Statement";
	struct Statement:resource<rdfs_namespace,str<'S','t','a','t','e','m','e','n','t'>,
		std::tuple<>,//if same size as base class could it use same allocator???
		Statement
	>{
		base_resource::const_instance_iterator p;
		//instead we need a combination of base_resource::type_iterator and RESOURCE_PTR to the built-in statement object
		Statement(uri u):SELF(u){}	
		Statement(uri u,objrdf::base_resource::const_instance_iterator p):SELF(u),p(p){}
		static void patch(V& _v){
			//let's first read subject/predicate/object to get a reference to the property of interest
			{
				function_table t;	
				t.cget_object=[](CONST_RESOURCE_PTR _subject,size_t){return static_cast<allocator_type::const_pointer>(_subject)->p.subject;};
				t.get_size=[](CONST_RESOURCE_PTR){return size_t(1);};
				_v.push_back(property_info(subject::get_property(),t));
			}
			{
				function_table t;	
				t.cget_object=[](CONST_RESOURCE_PTR _subject,size_t){return CONST_RESOURCE_PTR(static_cast<allocator_type::const_pointer>(_subject)->p.get_Property());};
				t.get_size=[](CONST_RESOURCE_PTR){return size_t(1);};
				_v.push_back(property_info(predicate::get_property(),t));
			}
			{
				function_table t;	
				t.cget_object=[](CONST_RESOURCE_PTR _subject,size_t){return CONST_RESOURCE_PTR(base_resource::nil);};
				t.get_size=[](CONST_RESOURCE_PTR){return size_t(1);};
				_v.push_back(property_info(object::get_property(),t));
			}
		}

	};	

	template<
		typename PROPERTY,
		typename STATEMENT_PROPERTY,
		typename STATEMENT_ALLOCATOR
	>struct reification:PROPERTY{
		//create class to store info
		typedef objrdf::resource<rdfs_namespace,str<'S','t','a','t','e','m','e','n','t'>,STATEMENT_PROPERTY> Statement;
		typename reification::Statement::allocator_type::pointer s;//better with ref counting		
		reification(){
			Statement::get_class();
			//should only construct if needed, what happens when copying?
			typename reification::Statement::allocator_type a;
			s=a.allocate(1);
			a.construct(s,uri());
			//also creates a rdf:Statement that will act as a proxy
			{
				rdf::Statement::allocator_type a;
				//auto tmp=a.allocate(1);
				/*
 				* we need an instance iterator to this property!, actually instance iterator gets invalidated 
 				* if inside container and size changes....the safest would be to look up per resource/property type/property instance
 				* we still need a reference to the subject!
 				* ok let's keep it simple and go the other way
				*/
				//a.construct(tmp,uri());
			}

		}
		~reification(){
			/*
			typename Statement::allocator_type a;
			a.destroy(s);
			a.deallocate(s,1);
			*/
		}
	};

}
typedef rdf::reification<g,std::tuple<version>,base_resource::allocator_type> rg;
CLASS(A,std::tuple<rg>);
int main(){
	A::get_class();
	rdf::Statement::get_class();
	A::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,uri("jm"));
	to_rdf_xml(p,cout);
	p->get<rg>().s->get<version>().t=1234;
	to_rdf_xml(p->get<rg>().s,cout);
	//rg a;//,b;//that should create new class
	/*
	rg::Statement::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,uri("111"));
	*/
	//get statement
	/*
	for(auto i=cbegin(p);i<cend(p);++i){
		for(auto j=i->cbegin();j<i->cend();++j){
			rdf::Statement::allocator_type a;
			auto s=a.construct_allocate(uri(),j);
			to_rdf_xml(s,cout);
			//wish we could still have that simple syntax instead of allocators...
			//rdf::Statement s(uri(),j);
			//s.to_rdf_xml(cout);
		}
	}	
	*/
	//to_rdf_xml(rg::Statement::get_class(),cout);
	/*
	{
		rdf::Statement::allocator_type a;
		for(auto i=a.cbegin();i<a.cend();++i) to_rdf_xml(i,cout);
	}
	{
		rg::Statement::allocator_type a;
		for(auto i=a.cbegin();i<a.cend();++i) to_rdf_xml(i,cout);
	}
	*/
	{	
		rdfs::Class::allocator_type a;
		for(auto i=a.cbegin();i<a.cend();++i){
			cerr<<(int)i.get_cell_index()<<"\t"<<i->id<<endl;	
		}
	}
	//crashes when iterating through classes in generic fashion
	/*
	{
	rdfs::Class::allocator_type a;
	//for(auto i=a.cbegin();i!=a.cend();++i){
	auto i=a.cbegin();{
		cerr<<i->id<<endl;
		pool_allocator::pool::POOL_PTR p(i.get_cell_index()); //there is a mapping between Class and pools
		ofstream of("dump");
		of.write(p->buffer,p->buffer_size);
		cerr<<"pool size generic:"<<p->get_size_generic(*p)<<"\t"<<p->cell_size<<"\t"<<p->payload_offset<<endl;
		for(auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);++j){
			cerr<<j.get_cell_index()<<"\t"<<j->id<<endl;	
		}
	}
	}
	*/
	//cerr<<"#######"<<endl;
	to_rdf_xml(cout);

}
