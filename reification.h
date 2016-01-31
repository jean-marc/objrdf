#ifndef REIFICATION_H
#define REIFICATION_H
#include "objrdf.h"
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
//	
	template<typename STR> struct reified_str:STR{
		static const char* name(){
			char prefix[]="reified_";
			auto s=STR::name();
			char* ss=new char[strlen(prefix)+strlen(s)+1];
			strcpy(ss,prefix);			
			strcat(ss,s);
			//delete[] s;
			return ss;
		}
	};
	template<
		typename _PROPERTY_,
		typename STATEMENT_PROPERTY,
		typename ALLOCATOR
	>
	struct reified;
	template<
		typename NAMESPACE,
		typename NAME,
		typename RANGE,
		typename STATEMENT_PROPERTY,
		typename ALLOCATOR
	>
	struct reified<property<NAMESPACE,NAME,RANGE>,STATEMENT_PROPERTY,ALLOCATOR>:property<NAMESPACE,NAME,RANGE>{
		//what if we make just a wrapper and manage the meta member outside?
		/*
		*	only store information if pointer non-null
		*	we need to come up with derived class name, should have something to do with property name 
		*	can we have compile time concatenation?
		*/ 
		typedef resource<
			rdf::rdfs_namespace,
			reified_str<NAME>,
			STATEMENT_PROPERTY,
			NIL,
			rdf::Statement,
			std::tuple<>,
			ALLOCATOR> R;//I guess we could call it statement too
		typename R::allocator_type::pointer meta;//could we allocate on the stack?
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
