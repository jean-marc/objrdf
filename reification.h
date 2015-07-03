#ifndef REIFICATION_H
#define REIFICATION_H
#include <objrdf/objrdf.h>
namespace rdf{
	using namespace objrdf;
	typedef property<rdf::rdfs_namespace,str<'s','u','b','j','e','c','t'>,RESOURCE_PTR> subject;
	typedef property<rdf::rdfs_namespace,str<'p','r','e','d','i','c','a','t','e'>,PROPERTY_PTR> predicate;
	typedef property<rdf::rdfs_namespace,str<'o','b','j','e','c','t'>,RESOURCE_PTR> object;
	struct Statement:resource<rdf::rdfs_namespace,str<'S','t','a','t','e','m','e','n','t'>,
		std::tuple<subject,predicate,object>,
		Statement
	>{
		Statement(uri u):SELF(u){}
		//how can we accept any predicate?, make it behave like a proxy
		//what about defining a map of all 
	};
}
namespace objrdf{
//augmented property
	template<
		typename _PROPERTY_,
		typename STATEMENT_PROPERTY=std::tuple<>
	>
	struct reified:_PROPERTY_{
		/*
		*	only store information if pointer non-null
		*	we need to come up with derived class name, should have something to do with property name 
		*/ 
		typedef resource<rdf::rdfs_namespace,str<'R'>,STATEMENT_PROPERTY,NIL,rdf::Statement> R;
		typename R::allocator_type::pointer meta;
		reified(){
			R::get_class();
		}
		reified(const reified& r):_PROPERTY_(r){
			if(r){
				typename R::allocator_type a;
				meta=a.allocate(1);
				a.construct(meta,get_uri(meta));
				meta->get<rdf::predicate>()=_PROPERTY_::get_property();
				meta->get<rdf::object>()=r;//might not be ready!
			}
		}
		reified(typename _PROPERTY_::RANGE r):_PROPERTY_(r){
			if(r){
				typename R::allocator_type a;
				meta=a.allocate(1);
				a.construct(meta,get_uri(meta));
				meta->get<rdf::predicate>()=_PROPERTY_::get_property();
				meta->get<rdf::object>()=r;
			}
		}
		~reified(){
			if(meta){
				typename R::allocator_type a;
				a.destroy(meta);
				a.deallocate(meta,1);
			}
		}
		reified& operator=(const reified& r){
			_PROPERTY_::operator=(r);
			if(!meta&&r){
				typename R::allocator_type a;
				meta=a.allocate(1);
				a.construct(meta,get_uri(meta));
				meta->get<rdf::predicate>()=_PROPERTY_::get_property();
				meta->get<rdf::object>()=r;
			}
			return *this;
		}
		void set_object(RESOURCE_PTR object){
			operator=(static_cast<typename _PROPERTY_::RANGE>(object));
		}
	};
	/*
	template<
		typename SUBJECT,
		typename PROPERTY,
		typename STATEMENT_PROPERTY
	> struct functions<SUBJECT,reified<PROPERTY,STATEMENT_PROPERTY>,0>{

		static function_table get_table(){
			auto tmp=functions<SUBJECT,PROPERTY,0>::get_table();
			tmp.set_object=[](RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){
				
			};
			//patch set_object
			return tmp;
		}

	};
	*/
}
#endif
