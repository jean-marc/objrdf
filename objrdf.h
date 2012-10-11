#ifndef OBJRDF_H
#define OBJRDF_H
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <map>
#include <tuple>
#include "tuple_helper.h"
#include <assert.h>
#include <cstddef>
#include "shared_ptr.h"
#include "ifthenelse.hpp"
#include <algorithm>
#include <stdexcept>
#include "uri.h"
using namespace std;
using namespace special;//for shared_ptr
#define LOG std::cerr
#include "custom_allocator.h"
template<typename T> vector<T> concat(/*const*/ vector<T>& a,const vector<T>& b){
	a.insert(a.end(),b.begin(),b.end());
	return a;
}
#define PROPERTY(n,...) char _##n[]=#n;typedef objrdf::property<rdfs_namespace,_##n,__VA_ARGS__> n
//property that exists in the objrdf namespace
#define OBJRDF_PROPERTY(n,...) char _##n[]=#n;typedef objrdf::property<_rdfs_namespace,_##n,__VA_ARGS__> n
#define CLASS(n,...) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,__VA_ARGS__> n
#define DERIVED_CLASS(n,BASE,...) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,__VA_ARGS__,objrdf::NIL,BASE> n
/*
 *	could we define lightweight classes? to reuse code?, that would all use the same pool because they are identical
 *	they only differ by their rdfs::type
 */
//
//#define DEFAULT_SET template<typename U> void set(U u){help_set<U>::go(this,u);}//need to copy this line in any struct that specializes the function
/*
 * 	must be a public member of the Class!!!!!
 * 	multiple symbols will be defined
 * 	as an alternative could use a C++ comment and parse this file to generate RDF, preprocessor
 */
#define COMMENT(str) static string get_comment(){return str;}
#define HTML_COMMENT(str) static string get_comment(){return string("<p xmlns='http://www.w3.org/1999/xhtml'>")+str+"</p>";}
namespace objrdf{
	template<const char* _A,const char* _B> struct tpair{
		//enum{N=(int)_A}; would be nice to have enums for Class/Property so we can use switch statements
	};
	template<typename T> struct get_uri_help;
	template<const char* A,const char* B> struct get_uri_help<tpair<A,B> >{static uri go(string name){return uri(A,B,name);}};
	template<typename T> uri get_uri(string name){return get_uri_help<T>::go(name);}	
	template<typename A,typename B> struct equality{enum{VALUE=0};};
	template<typename A> struct equality<A,A>{enum{VALUE=1};};
}
/*
 *	uri and prefix MUST be quoted, the macro could quote but forward slashes in URI confuse syntax highlighting in VIM
 *	hash namespace vs slash namespace, use hash for now because of optimization, could catch non hash uri at compile-time or run-time
 */
#define RDFS_NAMESPACE(uri,prefix) char _uri_[]=uri;char _prefix_[]=prefix;typedef objrdf::tpair<_uri_,_prefix_> rdfs_namespace;
#define OBJRDF_RDFS_NAMESPACE(uri,prefix) char __uri_[]=uri;char __prefix_[]=prefix;typedef objrdf::tpair<__uri_,__prefix_> _rdfs_namespace;
namespace rdf{
	//multiple definitions, linker will complain
	RDFS_NAMESPACE("http://www.w3.org/1999/02/22-rdf-syntax-ns#","rdf");
	const static objrdf::uri _RDF=objrdf::get_uri<rdfs_namespace>("RDF");
	const static objrdf::uri ID=objrdf::get_uri<rdfs_namespace>("ID");
	const static objrdf::uri about=objrdf::get_uri<rdfs_namespace>("about");
	const static objrdf::uri resource=objrdf::get_uri<rdfs_namespace>("resource");
	const static objrdf::uri Description=objrdf::get_uri<rdfs_namespace>("Description");
	struct Property;
}
namespace rdfs{
	RDFS_NAMESPACE("http://www.w3.org/2000/01/rdf-schema#","rdfs");
	struct Class;
}
namespace special{
	//to guarantee it is never used
	struct shared_ptr<rdfs::Class>;
	struct shared_ptr<rdf::Property>;
}
//
namespace objrdf{
	OBJRDF_RDFS_NAMESPACE("http://www.example.org/objrdf#","obj");
	struct NIL{
		typedef NIL SELF;
	};
	class base_resource;
}
//test to make sure we do not persist objrdf::base_resource, because of duplicates objrdf::base_resource::nil
template<> struct pseudo_ptr<objrdf::base_resource,persistent_store,false,uint16_t>;

namespace objrdf{
	/*
 	* it means that CONST_RESOURCE_PTR::construct will allocate on the free_store
 	* but any pseudo_ptr<T> can be cast to a CONST_RESOURCE_PTR
 	* this is confusing because each type should define its own typedef STORE
 	*/
	typedef pseudo_ptr<base_resource,free_store,true> RESOURCE_PTR;
	typedef pseudo_ptr<const base_resource,free_store,true> CONST_RESOURCE_PTR;
	typedef pseudo_ptr<const rdfs::Class,free_store,false,uint16_t> CLASS_PTR;
	typedef pseudo_ptr<const rdf::Property,free_store,false,uint16_t> PROPERTY_PTR;
	//does not compile:
	//typedef pseudo_ptr<const rdf::Property,rdf::Property::STORE,false,uint16_t> PROPERTY_PTR;
	typedef base_resource* ITERATOR_RESOURCE_PTR;
	typedef const base_resource* ITERATOR_CONST_RESOURCE_PTR;
	typedef char PROVENANCE;
	/*
 	*	can we define default functions that will give a helpful error message when called?
 	*/ 
	typedef std::tuple<
		/*literal*/
		void (*)(ITERATOR_RESOURCE_PTR,string,size_t),	/* 0	set_string */
		void (*)(ITERATOR_RESOURCE_PTR,istream&,size_t),//1 in
		void (*)(ITERATOR_CONST_RESOURCE_PTR,ostream&,size_t),//2 out
		/*non-literal*/
		RESOURCE_PTR (*)(ITERATOR_RESOURCE_PTR,size_t),//3 get_object
		CONST_RESOURCE_PTR (*)(ITERATOR_CONST_RESOURCE_PTR,CONST_RESOURCE_PTR,size_t),//4 get_const_object
		void (*)(ITERATOR_RESOURCE_PTR,RESOURCE_PTR,size_t),//5 set_object
		/*common*/
		size_t  (*)(ITERATOR_CONST_RESOURCE_PTR),//6 get_size
		void (*)(ITERATOR_RESOURCE_PTR,PROVENANCE),//7 add_property	
		void (*)(ITERATOR_RESOURCE_PTR,size_t,size_t),//8 erase
		PROVENANCE (*)(ITERATOR_CONST_RESOURCE_PTR,size_t)//9 get_provenance
	> function_table;
	struct property_info{
		//because now stored in vector<property_info>
		/*const*/ PROPERTY_PTR p;
		function_table t;
		/*const*/ bool literalp;
		property_info(PROPERTY_PTR p,function_table t);
	};
	typedef vector<property_info> V;
	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t INDEX=tuple_index<PROPERTY,typename SUBJECT::PROPERTIES>::value,
		bool FOUND=tuple_index<PROPERTY,typename SUBJECT::PROPERTIES>::value < tuple_size<typename SUBJECT::PROPERTIES>::value
	> struct helper{
		static PROPERTY& get(SUBJECT& s){return std::get<INDEX>(s.p);}
		static const PROPERTY& get_const(const SUBJECT& s){return std::get<INDEX>(s.p);}
	};
	template<typename P> struct get_store;
	template<
		typename T,
		typename STORE,
		bool POLYMORPHISM,
		typename INDEX
	> struct get_store<pseudo_ptr<T,STORE,POLYMORPHISM,INDEX>>{
		typedef STORE value;
	};
	template<
		typename T,
		typename pointer,
		typename reference
	> struct get_store<shared_ptr<T,pointer,reference>>:get_store<pointer>{
	};
	/*
 	*	problem with using array: the information is lost in the schema, now we use 
 	*	rdfs:member http://www.w3.org/TR/rdf-schema/#ch_member
 	*/ 	
	template<
		typename PROPERTY,
		typename STORE=typename get_store<typename PROPERTY::PTR>::value
	> class array:public vector<
		PROPERTY,
		custom_allocator<
			PROPERTY,
			pseudo_ptr_array<PROPERTY,STORE,false>
		>
	>{
	public:
		enum{TYPE=PROPERTY::TYPE};
		typedef array SELF;
		static PROPERTY_PTR get_property();
		~array(){cerr<<"~array()"<<this->size()<<endl;}
	};
	struct match_property{
		PROPERTY_PTR p;
		match_property(PROPERTY_PTR p):p(p){};
		bool operator()(const property_info& g)const{return g.p==p;}
		template<typename T> bool operator()(const T& g)const{return g.p==p;}
	};
	class base_resource{
	public:
		//to be investigated ...
		//template<typename T> friend class special::shared_ptr<T>; 
		//used by pool
		static bool TEST(const base_resource& r){
			//problem is that id is constant so we can clear it when deleted
			//we could wipe clean when deallocating
			//return !r.id.empty();
			return r.id.local[0];
		}
		typedef base_resource SELF;
		/*
 		*	default storage is the free store, to override your class must define STORE
 		*/ 
		typedef free_store STORE;
		//typedef persistent_store STORE;
		typedef std::tuple<> PROPERTIES; //base_resource does not have properties
		struct instance_iterator{
			friend class base_resource; //for base_resource::erase
			ITERATOR_RESOURCE_PTR subject;
			//we could also store a RESOURCE_PTR along
			V::iterator i;
			size_t index;
			instance_iterator(ITERATOR_RESOURCE_PTR subject,V::iterator i,size_t index):subject(subject),i(i),index(index){}
			instance_iterator& operator+=(const unsigned int& i){index+=i;return *this;}
			instance_iterator& operator++(){++index;return *this;}
			instance_iterator* operator->(){return this;}
			instance_iterator& operator*(){return *this;}
			/*
 			*	this operator is a bit more expensive 
 			*/ 
			bool operator==(const instance_iterator& j) const{return subject==j.subject && index==j.index;}
			bool operator!=(const instance_iterator& j) const{return index!=j.index;}
			bool operator<(const instance_iterator& j) const{return index<j.index;}
			void in(istream& is);
			void out(ostream& os) const;
			RESOURCE_PTR get_object() const;
			CONST_RESOURCE_PTR get_const_object() const;
			ITERATOR_RESOURCE_PTR get_subject()const;
			void set_object(RESOURCE_PTR);
			//need those sometime
			PROPERTY_PTR get_Property() const;
			bool literalp() const;
			friend ostream& operator<<(ostream& os,const instance_iterator& i){
				if(i.literalp()) 
					i.out(os);
				else
					os<<i.get_object()->id;
				return os;
			}
			string str();
			void set_string(string s);
			static instance_iterator help();
			PROVENANCE get_provenance() const;
		};
		struct const_instance_iterator{
			friend class base_resource; //for base_resource::erase
			ITERATOR_CONST_RESOURCE_PTR subject;
			CONST_RESOURCE_PTR alt_subject;
			V::const_iterator i;
			size_t index;
			const_instance_iterator():subject(0),index(0){}
			const_instance_iterator(ITERATOR_CONST_RESOURCE_PTR subject,V::const_iterator i,size_t index):subject(subject),alt_subject(CONST_RESOURCE_PTR()),i(i),index(index){}
			const_instance_iterator(ITERATOR_CONST_RESOURCE_PTR subject,CONST_RESOURCE_PTR alt_subject,V::const_iterator i,size_t index):subject(subject),alt_subject(alt_subject),i(i),index(index){
				//cerr<<"const_instance_iterator alt:"<<this->subject<<"\t"<<this->alt_subject<<endl;
			}
			const_instance_iterator& operator+=(const unsigned int& i){index+=i;return *this;}
			const_instance_iterator& operator++(){++index;return *this;}
			//tricky here
			const const_instance_iterator* operator->()const{return this;}
			const const_instance_iterator& operator*()const{return *this;}
			/*
 			*	this operator is a bit more expensive 
 			*/ 
			bool operator==(const const_instance_iterator& j) const{return subject==j.subject && index==j.index;}
			bool operator!=(const const_instance_iterator& j) const{return index!=j.index;}
			bool operator<(const const_instance_iterator& j) const{return index<j.index;}
			void out(ostream& os) const;
			CONST_RESOURCE_PTR get_const_object() const;
			ITERATOR_RESOURCE_PTR get_subject()const;
			//need those sometime
			PROPERTY_PTR get_Property() const;
			bool literalp() const;
			friend ostream& operator<<(ostream& os,const const_instance_iterator& i){
				if(i.literalp()) 
					i.out(os);
				else
					os<<i.get_const_object()->id;
				return os;
			}
			string str();
			static const_instance_iterator help();
			PROVENANCE get_provenance() const;
		};
		//should be moved to .cpp
		struct type_iterator:V::iterator{
			ITERATOR_RESOURCE_PTR subject;
			type_iterator(ITERATOR_RESOURCE_PTR subject,V::iterator i):V::iterator(i),subject(subject){}
			/* what is that for? to get an iterator from a type PROPERTY 
 			*	not sure it is worth the trouble
 			*/ 
			/*
			template<typename PROPERTY,typename RESOURCE> static type_iterator get(RESOURCE* subject){
				auto begin(RESOURCE::v.begin()),end(RESOURCE::v.end());
				//should be only done once
				while(begin!=end){
					if(begin->p==PROPERTY::get_property()) return type_iterator(subject,begin);
					++begin;
				}
				return type_iterator(subject,end);
			}
			*/
			V::iterator& get_base(){return *this;}
			size_t get_size() const;
			bool literalp() const;
			PROPERTY_PTR get_Property() const;
			/*
 			* very confusing syntactic sugar
 			* because V::iterator points to property_info
 			*/
			type_iterator* operator->(){return this;}
			instance_iterator begin(){return instance_iterator(subject,*this,0);}
			instance_iterator end(){return instance_iterator(subject,*this,get_size());}
			const_instance_iterator cbegin(){return const_instance_iterator(subject,*this,0);}
			const_instance_iterator cend(){return const_instance_iterator(subject,*this,get_size());}
			instance_iterator add_property(PROVENANCE p);
		};
		struct const_type_iterator:V::const_iterator{
			ITERATOR_CONST_RESOURCE_PTR subject;
			CONST_RESOURCE_PTR alt_subject;
			const_type_iterator(ITERATOR_CONST_RESOURCE_PTR subject,V::const_iterator i):V::const_iterator(i),subject(subject),alt_subject(CONST_RESOURCE_PTR(0)){}
			const_type_iterator(ITERATOR_CONST_RESOURCE_PTR subject,CONST_RESOURCE_PTR alt_subject,V::const_iterator i):V::const_iterator(i),subject(subject),alt_subject(alt_subject){
				//assert(0);
				//cerr<<"const_type_iterator alt:"<<this->subject<<"\t"<<this->alt_subject<<endl;
			}
			/* what is that for? */
			/*
			template<typename PROPERTY,typename RESOURCE> static const_type_iterator get(RESOURCE* subject){
				auto begin(RESOURCE::v.begin()),end(RESOURCE::v.end());
				//should be only done once
				while(begin!=end){
					if(begin->p==PROPERTY::get_property()) return const_type_iterator(subject,begin);
					++begin;
				}
				return const_type_iterator(subject,end);
			}
			*/
			V::const_iterator& get_base(){return *this;}
			size_t get_size() const;
			bool literalp() const;
			PROPERTY_PTR get_Property() const;
			/*
 			* very confusing syntactic sugar
 			* because V::iterator points to property_info
 			*/
			const_type_iterator& operator++(){
				this->V::const_iterator::operator++();
				return *this;
			}
			const_type_iterator* operator->(){return this;}
			const_instance_iterator cbegin(){return const_instance_iterator(subject,alt_subject,*this,0);}
			const_instance_iterator cend(){return const_instance_iterator(subject,alt_subject,*this,get_size());}
		};

		void erase(instance_iterator first,instance_iterator last);
		void erase(instance_iterator position);
		/*
 		* intrusive reference counting pg167 `Modern C++ design' Alexandrescu 
 		* problem: n is a pretty common variable name, should change or use setter/getter
 		* should use std::shared_ptr<> instead: no it does not work!!!
 		* must be made private!
		* could also be made a constant property so can be published for debugging
 		*/
		//short n;
		int n;//use 4 bytes to mask pool::info
		base_resource(uri id):n(0),id(id){
			#ifdef OBJRDF_VERB
			cerr<<"create base_resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		//shouldn't be const?
		uri id;
		static V v;
		~base_resource(){
			//for pool management
			id.local[0]=0;
			//
			#ifdef OBJRDF_VERB
			cerr<<"delete base_resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		CLASS_PTR get_Class() const{return get_class();};
		type_iterator begin();
		type_iterator end();
		const_type_iterator cbegin() const;
		const_type_iterator cend() const;
		void get_output(ostream& os) const;//local resources can have content accessible through a URL scheme 
		void end_resource(){};//will be invoked when finished parsing the element
		typedef std::tuple<
			//do we even need this when using PERSISTENT?
			//yes when creating new resource in the parser or sparql update query
			void (*)(void*,uri),	//in-place constructor
			type_iterator (*)(ITERATOR_RESOURCE_PTR),	//begin
			type_iterator (*)(ITERATOR_RESOURCE_PTR),	//end
			const_type_iterator (*)(CONST_RESOURCE_PTR),//cbegin
			const_type_iterator (*)(CONST_RESOURCE_PTR)	//cend
			//shall we add a clone function?
			/* add more functions here ... */
			/* pointer to member function */
			/*
			,type_iterator (base_resource::*)()
			,type_iterator (base_resource::*)()
			*/
		> class_function_table;
		/*
 		* 	we add information about the class to get rid of vtable	
		*	is there a reason we can't store everything in Class: one more level of indirection, 
		*	the best would be the pool, we could assign some space (char[n]) and just copy over
		*/	
		static CLASS_PTR get_class();	
		template<typename U> U& get(){return helper<base_resource,U>::get(*this);}
		template<typename U> const U& get_const() const{return helper<base_resource,U>::get_const(*this);}
		void to_turtle(ostream& os);
		void to_xml(ostream& os);
		void to_xml_leaf(ostream& os);
		static bool filter_provenance(PROVENANCE& p){return true;}
		void to_rdf_xml(ostream& os,const PROVENANCE& p=0) const;//the document should not have loops!!!
		//to use in bash
		void to_turtle_pretty(ostream& os);
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		static CONST_RESOURCE_PTR nil;
		int p_to_xml_size(const PROPERTY_PTR p);
		bool is_a(const CLASS_PTR&) const;
		COMMENT("The class resource, everything.");
	private:
		struct _tmp_{
			base_resource& r;
			string key;
			_tmp_(base_resource& _r,string _key):r(_r),key(_key){}
			void operator=(const string& val);
		};
	public:
		_tmp_ operator[](const string& key){return _tmp_(*this,key);}
	};
	//to get function pointers
	namespace f_ptr{
		template<typename T> base_resource::type_iterator begin(ITERATOR_RESOURCE_PTR r){return base_resource::type_iterator(r,T::v.begin());}
		template<typename T> base_resource::type_iterator end(ITERATOR_RESOURCE_PTR r){return base_resource::type_iterator(r,T::v.end());}
		template<typename T> base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return base_resource::const_type_iterator(r,r,T::v.cbegin());}
		template<typename T> base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return base_resource::const_type_iterator(r,r,T::v.cend());}
	}
	base_resource::instance_iterator operator+(const base_resource::instance_iterator& a,const unsigned int& b);

	CLASS_PTR get_class(CONST_RESOURCE_PTR r);
	base_resource::type_iterator begin(RESOURCE_PTR r);
	base_resource::type_iterator end(RESOURCE_PTR r);
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r);
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r);
	base_resource::const_instance_iterator get_const_self_iterator(CONST_RESOURCE_PTR r);
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os);
	
	
	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t INDEX
	> struct helper<SUBJECT,PROPERTY,INDEX,false>:helper<typename SUBJECT::SUPERCLASS,PROPERTY>{};

	template<
		typename NAMESPACE,
		const char* NAME,//maybe we can improve that in g++ > 4.5
		typename _PROPERTIES_=std::tuple<>, //MUST BE A std::tuple !!
		typename SUBCLASS=NIL,//default should be resource
		typename _SUPERCLASS_=base_resource
		/*
 		*	could add information about the expected number of instances when using persistent store
 		*/ 
		/*
 		*	information about the STORE 	
 		*/
	>
	struct resource:_SUPERCLASS_{
		typedef _PROPERTIES_ PROPERTIES;
		typedef _SUPERCLASS_ SUPERCLASS;
		typedef resource SELF;
		typedef persistent_store STORE;
		/*
 		*	not optimal when no properties (std::tuple<>)
 		*/ 
		PROPERTIES p;
		resource(uri id):SUPERCLASS(id){
			//risk of recursion!
			//
			#ifdef OBJRDF_VERB
			cerr<<"create resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		/*
 		*	all properties must be defined at once
 		*/
		resource(uri id,PROPERTIES p):SUPERCLASS(id),p(p){
			#ifdef OBJRDF_VERB
			cerr<<"create resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		~resource(){
			//to avoid recursion
			//how can we make sure that Classes are created?
			static auto tmp=resource::get_class();
			//does not work!
			#ifdef OBJRDF_VERB
			cerr<<"delete resource `"<<this->id<<"' "<<this<<endl;
			#endif
		}
		void operator=(const resource& r){
			p=r.p;
		}
		template<typename U> U& get(){return helper<resource,U>::get(*this);}
		template<typename U> const U& get_const() const{return helper<resource,U>::get_const(*this);}
		/*
 		*	a bit more complicated because we want to catch modifications, similar to db trigger
 		*	if overloaded for a property in derived class, a copy of template method must be present eg:
 		*	struct derived:resource<...,...,std::tuple<p_0,p_1,...>,derived>{
 		*		...
 		*		void set(p_0 p){
 		*			//handler is responsible for updating value
 		*			SELF::set(p);//qualified or infinite recursion!
 		*		}
		*		template<typename U> void set(U u){get<U>()=u;} //MUST be present otherwise compiler complaints
 		*	};
 		*/ 
		template<typename U> void set(U u){get<U>()=u;}
		static V v;
		base_resource::type_iterator begin(){return base_resource::type_iterator(this,v.begin());}
		base_resource::type_iterator end(){return base_resource::type_iterator(this,v.end());}  
		base_resource::const_type_iterator cbegin() const{return base_resource::const_type_iterator(this,v.cbegin());}
		base_resource::const_type_iterator cend() const{return base_resource::const_type_iterator(this,v.cend());}  
		static CLASS_PTR get_class();	
		/*	
		template<typename U,int FOUND=tuple_index<U,PROPERTIES>::value!=tuple_size<PROPERTIES>::value> struct help_set{
			static void go(resource* r,U u){r->get<U>()=u;}
		};
		template<typename U> struct help_set<U,2>{
			static void go(resource* r,U u){r->get<array<U> >().back()=u;}
		};
		*/
		//#define DEFAULT_SET template<typename U> void set(U u){help_set<U>::go(this,u);}
		//DEFAULT_SET;
	};
	template<typename S> struct get_generic_property;

	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS	
	> V resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS>::v=get_generic_property<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS> >::go();

	enum{LITERAL=0x1};
	enum{STRING=0x2};
	enum{CONST=0x4};
	template<typename RANGE> struct base_property{
		typedef persistent_store STORE;
		enum{TYPE=LITERAL};
		RANGE t;
		//should it be constant?
		base_property(RANGE t=RANGE()):t(t){}
		//sometime we want to override those functions, eg:hex type
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return 1;}
		void erase(){t=0;}
	};
	template<> struct base_property<string>{
		//no STORE defined because string is not supported yet, need to define special allocator
		//typedef persistent_store STORE;
		enum{TYPE=STRING|LITERAL};
		string t;
		base_property(string t=string()):t(t){}
		void set_string(string s){t=s;}
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return t.size()>0;}
		void erase(){t.clear();}
	};
	template<> struct base_property<uri>{
		typedef persistent_store STORE;
		enum{TYPE=STRING|LITERAL};
	};
	/*
 	* constant property, needs more work, can not be in array
 	* not really easy to use unless it is only property or tuple constructor
 	* can use default value, also will cause problem when parsing (but its value
 	* should be set programmatically anyway, ... not sure)
 	*/
	template<typename RANGE> struct base_property<const RANGE>{
		typedef persistent_store STORE;
		enum{TYPE=CONST|LITERAL};
		const RANGE t;
		base_property(const RANGE t=0):t(t){}
		size_t get_size() const{return 1;}
		void out(ostream& os){os<<t;}
		void erase(){}
	};
	template<int N> struct base_property<char[N]>{
		typedef persistent_store STORE;
		enum{TYPE=STRING|LITERAL};
		char t[N];
		base_property(){t[0]=0;}
		base_property(const char s[N]){strcpy(t,s);}
		void set_string(string s){strncpy(t,s.c_str(),N-1);}
		void in(istream& is){
			string tmp;
			is>>tmp;
			set_string(tmp);
		}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return strlen(t)>0;}
		void erase(){t[0]=0;}
	};
	template<
		typename T,
		typename STORE,
		bool POLYMORPHISM,
		typename INDEX
	>
	class base_property<pseudo_ptr<T,STORE,POLYMORPHISM,INDEX>>:public pseudo_ptr<T,STORE,POLYMORPHISM,INDEX>{
	public:
		enum{TYPE=0};
		typedef pseudo_ptr<T,STORE,POLYMORPHISM,INDEX> PTR;
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}//dangerous notation
		RESOURCE_PTR get_object() const{return *this;}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		void set_object(RESOURCE_PTR object){this->PTR::operator=(object);}
		void erase(){set_object(RESOURCE_PTR());}
	};

	template<
		typename T,
		typename STORE,
		bool POLYMORPHISM,
		typename INDEX
	>
	class base_property<pseudo_ptr<const T,STORE,POLYMORPHISM,INDEX>>:public pseudo_ptr<const T,STORE,POLYMORPHISM,INDEX>{
	public:
		enum{TYPE=CONST};
		typedef pseudo_ptr<const T,STORE,POLYMORPHISM,INDEX> PTR;
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}
		CONST_RESOURCE_PTR get_const_object() const{return (PTR)*this;}
		void set_object(RESOURCE_PTR object){this->PTR::operator=(object);}
		void erase(){set_object(RESOURCE_PTR());}
	};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename _RANGE_,
		typename IMPLEMENTATION=base_property<_RANGE_>
	> class property:public IMPLEMENTATION{
	public:
		PROVENANCE p;
		typedef _RANGE_ RANGE;
		typedef property SELF;
		//template<typename S> property(S s):base_property<RANGE>(s){}
		property(IMPLEMENTATION r):IMPLEMENTATION(r){
			//LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		property()/*:p(0)*/{
			//LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		~property(){
			//LOG<<"delete property `"<<NAME<<"' "<<this<<endl;
		}
		//we could add arguments to modify the property, for instance set the rdfs::domain, the rdfs::subPropertyOf
		static PROPERTY_PTR get_property();
		static PROPERTY_PTR get_property_array();
	};
	/*
 	*	property to keep track of versions, a pointer to the previous version
 	*	it can only be modified by the application
 	*/ 
	OBJRDF_PROPERTY(prev,RESOURCE_PTR);


	RESOURCE_PTR create_by_type(CLASS_PTR c,uri id);
	template<typename P> uri get_uri(const P& p){
		CONST_RESOURCE_PTR r(p);
		size_t _id=r.pool_ptr.index | (r.index<<(sizeof(r.pool_ptr.index)<<3));
		ostringstream os;
		os<<hex<<_id;
		return uri(os.str().c_str());
	}
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct base_f{
		static inline PROPERTY& get(ITERATOR_RESOURCE_PTR subject,size_t){return static_cast<SUBJECT*>(subject)->template get<PROPERTY>();}
		static inline const PROPERTY& get_const(ITERATOR_CONST_RESOURCE_PTR subject,size_t){return static_cast<const SUBJECT*>(subject)->template get_const<PROPERTY>();}
		static size_t get_size(ITERATOR_CONST_RESOURCE_PTR subject){return get_const(subject,0).get_size();}
		static void add_property(ITERATOR_RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void add_property_t(ITERATOR_RESOURCE_PTR subject,PROVENANCE p){
			/*
 			* could be the good place to implement a transaction
			* we will have patch the function table or there could be a flag in PROPERTY to indicate we want transaction
			* would be nice to hide a pointer to old version, could be a pseudo attribute based on id
			* should only be recorded if property value is modified
			* the pointer could also easily fit inside the base_resource::id
			* ideally it should be a double-linked list so we could remove old history
			* 	<obj:prev rdf:resource='...'/>
			* 	<obj:next rdf:resource='...'/>
			* What happens when a resource is deleted?
			* we have to make sure that a copy can not be modified
			* would be better done at compile time static_cast<SUBJECT*>(subject)->get<objrdf::prev>()=c;
			*/
			/*static*/auto i=find_if(static_cast<SUBJECT*>(subject)->begin(),static_cast<SUBJECT*>(subject)->end(),match_property(objrdf::prev::get_property()));
			if(i!=static_cast<SUBJECT*>(subject)->end()){
				RESOURCE_PTR c=create_by_type(SUBJECT::get_class(),uri(""));
				c->id=get_uri(c);	
				static_cast<SUBJECT&>(*c)=static_cast<SUBJECT&>(*subject);//deep copy
				i->add_property(0)->set_object(c);//careful with infinite recursion
			}
		}
		static void erase(ITERATOR_RESOURCE_PTR subject,size_t first,size_t last){get(subject,0).erase();}	
		static PROVENANCE get_provenance(ITERATOR_CONST_RESOURCE_PTR subject,size_t){return 0;/*get_const(subject).p;*/}
		static function_table get_table(){
			function_table t;
			std::get<6>(t)=get_size;
			std::get<7>(t)=add_property;
			std::get<8>(t)=erase;
			std::get<9>(t)=get_provenance;
			return t;	
		}
	};
	template<
		typename SUBJECT,
		typename PROPERTY,
		typename STORE
	> struct base_f<SUBJECT,array<PROPERTY,STORE>>{
		static inline array<PROPERTY,STORE>& get(ITERATOR_RESOURCE_PTR subject){return static_cast<SUBJECT*>(subject)->template get<array<PROPERTY,STORE>>();}
		static inline const array<PROPERTY,STORE>& get_const(ITERATOR_CONST_RESOURCE_PTR subject){return static_cast<const SUBJECT*>(subject)->template get_const<array<PROPERTY,STORE>>();}
		static inline PROPERTY& get(ITERATOR_RESOURCE_PTR subject,size_t index){return static_cast<SUBJECT*>(subject)->template get<array<PROPERTY,STORE>>()[index];}
		static inline const PROPERTY& get_const(ITERATOR_CONST_RESOURCE_PTR subject,size_t index){return static_cast<const SUBJECT*>(subject)->template get_const<array<PROPERTY,STORE>>()[index];}
		static size_t get_size(ITERATOR_CONST_RESOURCE_PTR subject){return get_const(subject).size();}
		static void add_property(ITERATOR_RESOURCE_PTR subject,PROVENANCE p){typedef PROPERTY P;get(subject).push_back(P());}
		static void erase(ITERATOR_RESOURCE_PTR subject,size_t first,size_t last){get(subject).erase(get(subject).begin()+first,get(subject).begin()+last);}
		static PROVENANCE get_provenance(ITERATOR_CONST_RESOURCE_PTR subject,size_t index){return 0;}
		static function_table get_table(){
			function_table t;
			std::get<6>(t)=get_size;
			std::get<7>(t)=add_property;
			std::get<8>(t)=erase;
			std::get<9>(t)=get_provenance;
			return t;	
		}
	};

	template<typename SUBJECT,typename PROPERTY,size_t TYPE=PROPERTY::TYPE> struct functions;

	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONST|LITERAL>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static void out(ITERATOR_CONST_RESOURCE_PTR subject,ostream& os,size_t index){BASE::get_const(subject,index).out(os);}	
		static function_table get_table(){
			auto t=BASE::get_table();
			std::get<2>(t)=out;
			return t;
			//std::tuple does not like 0 for function pointer
			//return function_table(0,0,out,0,0,0,BASE::get_size,BASE::add_property,BASE::erase,BASE::get_provenance);
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,LITERAL>:functions<SUBJECT,PROPERTY,CONST|LITERAL>{
		typedef functions<SUBJECT,PROPERTY,CONST|LITERAL> BASE;
		static void in(ITERATOR_RESOURCE_PTR subject,istream& is,size_t index){BASE::get(subject,index).in(is);}
		static function_table get_table(){
			auto t=BASE::get_table();
			std::get<1>(t)=in;
			return t;	
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,STRING|LITERAL>:functions<SUBJECT,PROPERTY,LITERAL>{
		typedef functions<SUBJECT,PROPERTY,LITERAL> BASE;
		static void set_string(ITERATOR_RESOURCE_PTR subject,string s,size_t index){BASE::get(subject,index).set_string(s);}
		static function_table get_table(){
			auto t=BASE::get_table();
			std::get<0>(t)=set_string;
			return t;	
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONST>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static CONST_RESOURCE_PTR get_const_object(ITERATOR_CONST_RESOURCE_PTR subject,CONST_RESOURCE_PTR alt_subject,size_t index){
			return BASE::get_const(subject,index).get_const_object();
		}
		//why do we have set_object if it is a const resource?
		static void set_object(ITERATOR_RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){BASE::get(subject,index).set_object(object);}
		//override
		static void erase(ITERATOR_RESOURCE_PTR subject,size_t first,size_t last){set_object(subject,RESOURCE_PTR(0),0);}	
		static function_table get_table(){
			auto t=BASE::get_table();
			std::get<4>(t)=get_const_object;
			std::get<5>(t)=set_object;
			//std::get<8>(t)=erase;
			return t;	
		}
	};	
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,0>:functions<SUBJECT,PROPERTY,CONST>{
		typedef functions<SUBJECT,PROPERTY,CONST> BASE;
		static RESOURCE_PTR get_object(ITERATOR_RESOURCE_PTR subject,size_t index){return BASE::get(subject,index).get_object();}
		static function_table get_table(){
			auto t=BASE::get_table();
			std::get<3>(t)=get_object;
			return t;	
		}
	};
	//schema
	typedef base_resource* (*fpt)(uri);
	namespace f_ptr{
		template<typename T> void constructor(void* p,uri u){new(p)T(u);}//a lot simpler
		template<> void constructor<rdfs::Class>(void* p,uri u);
		template<> base_resource::type_iterator end<rdfs::Class>(ITERATOR_RESOURCE_PTR r);
		template<> void constructor<rdf::Property>(void* p,uri u);
		template<> base_resource::type_iterator end<rdf::Property>(ITERATOR_RESOURCE_PTR r);
	}
	struct type_p{
		CLASS_PTR t;
		type_p(CLASS_PTR t):t(t){}
		bool operator()(RESOURCE_PTR r) const;
	};
}//end namespace objrdf

	/*
 	*	is S derived from or equal to T?
 	*	this is used to verify the cast operations in pseudo_ptr<>
 	*/ 
	//template<typename S,typename T> struct is_derived;//will not compile if S not derived from T
	/*
	template<typename S,typename T> struct is_derived{enum{value=false};};
	template<typename S> struct is_derived<S,S>{enum{value=true};};	
	*/
	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,	
		typename T
	> struct is_derived<objrdf::resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS>,T>:is_derived<typename SUPERCLASS::SELF,T>{};

namespace rdf{
	PROPERTY(type,objrdf::CLASS_PTR);
}
namespace objrdf{
	//make it possible to modify a resource's id after it has been created, it is possible because the db
	//relies on pointers, not on id
	OBJRDF_PROPERTY(id,uri);
	//we can specialize for objrdf::id
	template<typename SUBJECT> struct functions<SUBJECT,objrdf::id,STRING|LITERAL>{
		static void set_string(ITERATOR_RESOURCE_PTR subject,string s,size_t){
			//only makes sense with local resources
			if(subject->id.is_local())
				subject->id=uri(s);//could add code to detect duplicate id's
		}
		static void in(ITERATOR_RESOURCE_PTR subject,istream& is,size_t){
			string tmp;
			is>>tmp;
			set_string(subject,tmp,0);
		}	
		static void out(ITERATOR_CONST_RESOURCE_PTR subject,ostream& os,size_t){os<<subject->id;}	
		static size_t get_size(ITERATOR_CONST_RESOURCE_PTR subject){return 1;}
		static void add_property(ITERATOR_RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void erase(ITERATOR_RESOURCE_PTR subject,size_t first,size_t last){}	
		static function_table get_table(){
			function_table t;
			std::get<0>(t)=set_string;
			std::get<1>(t)=in;
			std::get<2>(t)=out;
			std::get<6>(t)=get_size;
			std::get<7>(t)=add_property;
			std::get<8>(t)=erase;
			return t;
		}
	};

	/*
 	*	very useful property that returns a pointer to the subject, it is used by the sparql engine and will also be used
 	*	later for fast resource retrieval, a pseudo_ptr can be encoded in hex very easily
 	*/ 
	OBJRDF_PROPERTY(self,CONST_RESOURCE_PTR);
	template<typename SUBJECT> struct functions<SUBJECT,objrdf::self,objrdf::self::TYPE>{
		static CONST_RESOURCE_PTR get_const_object(ITERATOR_CONST_RESOURCE_PTR subject,CONST_RESOURCE_PTR alt_subject,size_t index){return alt_subject;}
		static void set_object(ITERATOR_RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){}
		static size_t get_size(ITERATOR_CONST_RESOURCE_PTR subject){return 1;}
		static void add_property(ITERATOR_RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void erase(ITERATOR_RESOURCE_PTR subject,size_t first,size_t last){}	
		static function_table get_table(){
			function_table t;
			std::get<4>(t)=get_const_object;
			std::get<5>(t)=set_object;//why do we need this????
			std::get<6>(t)=get_size;
			std::get<7>(t)=add_property;
			std::get<8>(t)=erase;
			return t;
		}

	};
}
namespace objrdf{
	/*
 	*	pseudo-properties, they don't use any memory
 	*/
	template<
		typename SUBJECT,
		size_t INDEX
	> struct helper<SUBJECT,rdf::type,INDEX,false>{
		static rdf::type& get(SUBJECT&){
			static rdf::type t(SUBJECT::get_class());
			return t;
		}
		static const rdf::type& get_const(const SUBJECT&){
			static rdf::type t(SUBJECT::get_class());
			return t;
		}
	};
}
namespace rdfs{
	PROPERTY(domain,objrdf::CLASS_PTR);
	PROPERTY(range,objrdf::CLASS_PTR);
	PROPERTY(subClassOf,objrdf::CLASS_PTR);
	PROPERTY(comment,string);
	/*
 	* some properties whose domain is rdf:Resource but we don't want to make them member
 	* of objrdf::base_resource 
 	*/
	PROPERTY(isDefinedBy,objrdf::RESOURCE_PTR);
	PROPERTY(label,string);
	PROPERTY(subPropertyOf,objrdf::PROPERTY_PTR);
	PROPERTY(member,objrdf::RESOURCE_PTR);
}
namespace rdf{
	char _Literal[]="Literal";
	struct Literal:objrdf::resource<rdfs_namespace,_Literal,std::tuple<>,Literal>{
		Literal(objrdf::uri u):SELF(u){}
		typedef empty_store STORE;
		COMMENT("The class of literal values, eg. textual strings an integers")
	};
}
namespace xsd{
	/*
 	*	http://www.w3.org/TR/2004/REC-rdf-mt-20040210/
 	*	not clear what the syntax should be
 	*/ 
	RDFS_NAMESPACE("http://www.w3.org/2001/XMLSchema#","xsd");//or xs?
	char _Double[]="double";typedef objrdf::resource<rdfs_namespace,_Double,std::tuple<>,objrdf::NIL,rdf::Literal> Double;
	char _Float[]="float";typedef objrdf::resource<rdfs_namespace,_Float,std::tuple<>,objrdf::NIL,rdf::Literal> Float;
	char _Int[]="integer";typedef objrdf::resource<rdfs_namespace,_Int,std::tuple<>,objrdf::NIL,rdf::Literal> Int;
	char _Unsigned_Int[]="unsignedInt";typedef objrdf::resource<rdfs_namespace,_Unsigned_Int,std::tuple<>,objrdf::NIL,rdf::Literal> Unsigned_int;
	char _Short[]="short";typedef objrdf::resource<rdfs_namespace,_Short,std::tuple<>,objrdf::NIL,rdf::Literal> Short;
	char _Unsigned_Short[]="unsignedShort";typedef objrdf::resource<rdfs_namespace,_Unsigned_Short,std::tuple<>,objrdf::NIL,rdf::Literal> Unsigned_short;
	char _String[]="string";typedef objrdf::resource<rdfs_namespace,_String,std::tuple<>,objrdf::NIL,rdf::Literal> String;
	char _anyURI[]="anyURI";typedef objrdf::resource<rdfs_namespace,_anyURI,std::tuple<>,objrdf::NIL,rdf::Literal> anyURI;
}
namespace objrdf{
	char _Char[]="Char";typedef objrdf::resource<_rdfs_namespace,_Char,std::tuple<>,NIL,rdf::Literal> Char;
}
namespace objrdf{
	template<typename T> struct get_Literal:rdf::Literal{}; 
	template<> struct get_Literal<double>:xsd::Double{};
	template<> struct get_Literal<float>:xsd::Float{};
	template<> struct get_Literal<int>:xsd::Int{};
	template<> struct get_Literal<long int>:xsd::Int{};
	template<> struct get_Literal<unsigned int>:xsd::Unsigned_int{};
	template<> struct get_Literal<short>:xsd::Short{};
	template<> struct get_Literal<unsigned short>:xsd::Unsigned_short{};
	template<> struct get_Literal<char>:Char{};
	template<> struct get_Literal<string>:xsd::String{};
	template<size_t N> struct get_Literal<char[N]>:xsd::String{};
	template<> struct get_Literal<uri>:xsd::anyURI{};
	//extra types
	template<> struct get_Literal<size_t>:xsd::Unsigned_int{};//not accurate	
	template<> struct get_Literal<bool>:xsd::Int{};
	//what about complex numbers?
	//complex numbers could we define it only if complex is defined???
	//template<typename T> struct get_Literal<complex<T> >:get_Literal<T>{}; 
}
namespace rdfs{
	/*
		XMLLiteral should be (double-) quoted when using turtle notation
		but NOT when using XML serialization of SPARQL reply
	*/
	struct XMLLiteral{}; //symbolic type
	char _XML_Literal[]="XML_Literal";
	typedef objrdf::resource<rdfs_namespace,_XML_Literal,std::tuple<>,objrdf::NIL,rdf::Literal> XML_Literal;
	//DERIVED_CLASS(XML_Literal,std::tuple<>,rdf::Literal);
	//JSON type
	//struct JSON{};
	//char _JSON[]="JSON";
	//typedef objrdf::resource<rdfs_namespace,_JSON,std::tuple<>,objrdf::NIL,rdf::Literal> JSON_type;
	//DERIVED_CLASS(JSON,std::tuple<>,rdf::Literal);
}
namespace objrdf{
	template<> struct get_Literal<rdfs::XMLLiteral>:rdfs::XML_Literal{};
}
namespace objrdf{
	OBJRDF_PROPERTY(superClassOf,objrdf::CLASS_PTR);
}
namespace rdfs{
	char _Class[]="Class";
	struct Class:objrdf::resource<
		rdfs_namespace,
		_Class,
		std::tuple<
			objrdf::array<subClassOf>,
			objrdf::array<objrdf::superClassOf>,
			comment,
			isDefinedBy
		>,
		Class>{
		/*
 		*	should store all the information about the resources including function pointers, the only problem with that
 		*	is the user might want to add his own function pointer (unless she decides to use virtual functions) and that
 		*	would mean defining (deriving) her own rdfs::Class class	
 		*	A pointer to a resource should be coupled with one to the resource's rdfs::Class but still use
 		*	the standar pointer semantic 
 		*	similar to instance_iterator::get_Property(), although instance_iterator never returns an actual reference/pointer 
 		*	to the underlying property.
 		*/ 
		typedef free_store STORE;
		const objrdf::base_resource::class_function_table t;
		Class(objrdf::uri id,subClassOf s,objrdf::base_resource::class_function_table t,string comment);
		~Class(){
			//quick fix to avoid infinite recursion
			n=1;
			cerr<<"delete Class `"<<id<<"'"<<endl;	
		}
		bool operator==(const Class& c) const;
		bool operator<(const Class& c) const;
		bool operator<=(const Class& c) const;
		bool is_subclass_of(const Class& c) const;
		bool literalp() const;
		static objrdf::CLASS_PTR super(objrdf::CLASS_PTR c){
			for(auto i=c->get_const<objrdf::array<subClassOf>>().cbegin();i<c->get_const<objrdf::array<subClassOf>>().end();++i){
				//problem *i is CLASS_PTR, we need to cast away constness
				pseudo_ptr<rdfs::Class,objrdf::CLASS_PTR::STORE,false,objrdf::CLASS_PTR::INDEX> tmp(*i);
				tmp->get<objrdf::array<objrdf::superClassOf>>().push_back(objrdf::superClassOf(c));
			}
			return c;
		}
		/*
 		*	inference, should be run once all classes and properties have been created
 		*/ 
		static void analyze();//could be a sparql request
		COMMENT("The class of classes.");
	};
}//end namespace rdfs
namespace objrdf{
	OBJRDF_PROPERTY(p_index,short);//each property has a unique index useful for fast serialization/parsing
	/*
 	* a unique id for each resource that can be turn into a pointer for fast access (no look-up involved)
	* we need to encode the type too, should be serialized to hex:
	*  size_t _id=r.pool_ptr.index | (r.index<<(sizeof(r.pool_ptr.index)<<3));
	*  os<<hex<<"{"<<_id<<"}"<<dec;
	*  it can be safely resolved by checking range and using the TEST function, client should store that
	*  unique id for very fast look-up
	*/
	//OBJRDF_PROPERTY(_id,size_t);
}
namespace rdf{
	char _Property[]="Property";
	struct Property:objrdf::resource<
		rdfs_namespace,
		_Property,
		std::tuple<
			rdfs::domain,
			rdfs::range,
			rdfs::subPropertyOf
		>,Property>{
		typedef free_store STORE;
		Property(objrdf::uri u);
		Property(objrdf::uri u,rdfs::range r,const bool literalp);
		Property(objrdf::uri u,rdfs::range r,const bool literalp,rdfs::subPropertyOf);
		const bool literalp;
		//nil property?
		static objrdf::PROPERTY_PTR nil;
		COMMENT("The class of RDF properties.");
	};
}//end namespace rdf

namespace objrdf{
	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS
	>
	CLASS_PTR resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS>::get_class(){
		typedef typename IfThenElse<equality<SUBCLASS,NIL>::VALUE,resource,SUBCLASS>::ResultT TMP;
		//we can chain a function to add superClassOf
		static CLASS_PTR p=rdfs::Class::super(CLASS_PTR::construct_at(
			//in case of persistent storage we will override old version and refresh pointers and function pointers
			POOL_PTR::help<TMP>().index,
			objrdf::get_uri<NAMESPACE>(NAME),
			rdfs::subClassOf(SUPERCLASS::get_class()),
			objrdf::base_resource::class_function_table(
				f_ptr::constructor<TMP>,
				f_ptr::begin<TMP>,
				f_ptr::end<TMP>,
				f_ptr::cbegin<TMP>,
				f_ptr::cend<TMP>
			),
			TMP::get_comment!=SUPERCLASS::get_comment ? TMP::get_comment() : ""
		));
		return p;
	}
	//there should be a cleaner way to do that
	template<typename RANGE> struct selector{
		typedef get_Literal<RANGE> ResultT;
		enum{IS_LITERAL=1};
	};
	template<typename RANGE> struct selector<RANGE*>{
		typedef RANGE ResultT;
		enum{IS_LITERAL=0};
	};
	template<
		typename T,
		typename STORE,
		bool POLYMORPHISM
	>
	struct selector<pseudo_ptr<T,STORE,POLYMORPHISM>>{
		typedef T ResultT;
		enum{IS_LITERAL=0};
	};
	/*
 	*	we need to be able to tell if the property is an array
 	*
 	*/
	template<
		typename NAMESPACE,
		const char* NAME,
		typename RANGE,
		typename IMPLEMENTATION
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::get_property(){
		static PROPERTY_PTR c(
			PROPERTY_PTR::construct(
				objrdf::get_uri<NAMESPACE>(NAME),
				rdfs::range(selector<RANGE>::ResultT::get_class()),
				property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::TYPE&LITERAL
			)
		);
		return c;
	}
	template<
		typename NAMESPACE,
		const char* NAME,
		typename RANGE,
		typename IMPLEMENTATION
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::get_property_array(){
		static PROPERTY_PTR c(
			PROPERTY_PTR::construct(
				objrdf::get_uri<NAMESPACE>(NAME),
				rdfs::range(selector<RANGE>::ResultT::get_class()),
				property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::TYPE&LITERAL,
				rdfs::subPropertyOf(rdfs::member::get_property())
			)
		);
		return c;
	}
	template<
		typename PROPERTY,
		typename STORE
	> PROPERTY_PTR array<PROPERTY,STORE>::get_property(){
		return PROPERTY::get_property_array();
	}
	struct name_p{
		const uri n;
		name_p(const uri n);
		bool operator()(const property_info& p) const;
	};
	template<typename SUBJECT,typename PROPERTY> property_info get_property_info(){
		/*
 		*	once we have the function table we know if property literal, 
 		*
 		*/
		property_info p(PROPERTY::get_property(),functions<SUBJECT,PROPERTY>::get_table());
		/*
 		* by now we can't modify rdfs::domain, why is it broken down in the first place?
		* because property does not know about SUBJECT, it is an important piece of meta information
		* very useful to optimize SPARQL queries, can we cheat?  
		* or can we store the information in a different way? we can query objects and list all properties
		* we can temporarily cast using the same store so we don't create a new store just for that
		*/
		//p.p->get<rdfs::domain>()=rdfs::domain(SUBJECT::get_class());
		pseudo_ptr<rdf::Property,PROPERTY_PTR::STORE,false,PROPERTY_PTR::INDEX> tmp(p.p);
		tmp->get<rdfs::domain>()=rdfs::domain(SUBJECT::get_class());
		return p;
	};
	template<typename SUBJECT> struct _meta_{
		V v;
		template<typename PROPERTY> void operator()(){
			v.push_back(get_property_info<SUBJECT,PROPERTY>());
		};
	};
	template<> struct get_generic_property<base_resource>{
		static V go(){
			LOG<<"get_generic_property:`base_resource'"<<endl;
			V v;
			v.push_back(get_property_info<base_resource,rdf::type>());
			v.push_back(get_property_info<base_resource,objrdf::id>());
			v.push_back(get_property_info<base_resource,objrdf::self>());
			return v;
		}
	};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS 
	> struct get_generic_property<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS> >{
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS> RESOURCE;
		typedef typename IfThenElse<equality<SUBCLASS,NIL>::VALUE,RESOURCE,SUBCLASS>::ResultT TMP;
		static V go(){
			LOG<<"get_generic_property:`"<<NAME<<"'"<<endl;
			V v=get_generic_property<typename SUPERCLASS::SELF>::go();
			/*
 			* multiple rdf:type properties, would be more consistent to have an array or a single type
 			* actually not that simple because it is a pseudo property
 			* for now the best is a single property
 			*/
			v.front()=get_property_info<RESOURCE,rdf::type>();//will override SUPERCLASS's rdf::type
			return concat(v,std::static_for_each<PROPERTIES>(_meta_<TMP>()).v);
		}
	};
	//would be nice to put it inside a class or namespace
	//iterators to navigate the pools, it won't allow modification of the resources
	//create index per pool? save the index or not?  we want to avoid `table scan'
	//could we use pointer instead
	struct pool_iterator:pool::iterator<POOL_PTR>{
		pool_iterator(pool::iterator<POOL_PTR> i):pool::iterator<POOL_PTR>(i){}
		typedef generic_iterator<CONST_RESOURCE_PTR> cell_iterator;
		cell_iterator begin(){return cell_iterator(**this,(**this)->get_size());}
		cell_iterator end(){return cell_iterator(**this);}
	};
	struct test_by_uri{
		const uri u;
		test_by_uri(const uri& u):u(u){}
		//must be a pseudo_ptr<>
		template<typename T> bool operator()(T t) const{return t->id==u;}
	};
	pool_iterator begin();
	pool_iterator end();
	void to_rdf_xml(ostream& os);
	//dumb scanner
	CONST_RESOURCE_PTR find(uri u);
	//not as dumb because uses a single pool
	//could be specialized for rdfs::Class and rdf::Property and use map<> index
	template<typename PTR> PTR find_t(uri u){
		cerr<<"looking up uri `"<<u<<"' in pool `"<<PTR::value_type::get_class()->id<<"'"<<endl;
		auto r=find_if(::begin<PTR>(),::end<PTR>(),test_by_uri(u));
		return r!=::end<PTR>() ? *r : PTR(0);
	}
	RESOURCE_PTR create_by_type(CLASS_PTR c,uri id);
	RESOURCE_PTR create_by_type(uri type,uri id);
}
template<
	const char* NAMESPACE,
	const char* PREFIX,
	const char* NAME,
	typename PROPERTIES,
	typename SUBCLASS,
	typename SUPERCLASS 
> struct name<objrdf::resource<objrdf::tpair<NAMESPACE,PREFIX>,NAME,PROPERTIES,SUBCLASS,SUPERCLASS>>{ 
	static const string get(){return /*"resource/"+*/string(PREFIX)+"_"+NAME;}
	enum{IS_RESOURCE=true};
}; 
template<> struct name<objrdf::base_resource>{
	static const string get(){return /*"resource/"+*/"rdfs_Resource";}
	enum{IS_RESOURCE=true};
};
template<
	const char* NAMESPACE,
	const char* PREFIX,
	const char* NAME,
	typename RANGE,
	typename IMPLEMENTATION	
> struct name<objrdf::property<objrdf::tpair<NAMESPACE,PREFIX>,NAME,RANGE,IMPLEMENTATION>>{
	static const string get(){return /*"property/"+*/string(PREFIX)+"_"+NAME;}
	enum{IS_RESOURCE=false};
	
};
#endif
