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
#ifdef PERSISTENT
#include "custom_allocator.h"
#endif
template<typename T> vector<T> concat(/*const*/ vector<T>& a,const vector<T>& b){
	a.insert(a.end(),b.begin(),b.end());
	return a;
}
#define PROPERTY(n,range) char _##n[]=#n;typedef objrdf::property<rdfs_namespace,_##n,range> n
#define _PROPERTY(n,range) char _##n[]=#n;typedef objrdf::property<_rdfs_namespace,_##n,range> n
#define __PROPERTY(n,...) char _##n[]=#n;typedef objrdf::property<_rdfs_namespace,_##n,__VA_ARGS__> n
#define CLASS(n,...) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,__VA_ARGS__> n
//
#define CLASS0(n) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<>> n
#define CLASS1(n,p0) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0>> n
#define CLASS2(n,p0,p1) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0,p1>> n
#define CLASS3(n,p0,p1,p2) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0,p1,p2>> n
#define CLASS4(n,p0,p1,p2,p3) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0,p1,p2,p3>> n
#define CLASS5(n,p0,p1,p2,p3,p4) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0,p1,p2,p3,p4>> n
#define CLASS6(n,p0,p1,p2,p3,p4,p5) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,std::tuple<p0,p1,p2,p3,p4,p5>> n

//#define DEFAULT_SET template<typename U> void set(U u){help_set<U>::go(this,u);}//need to copy this line in any struct that specializes the function
/*
 * 	must be a public member of the Class!!!!!
 * 	multiple symbols will be defined
 * 	as an alternative could use a C++ comment and parse this file to generate RDF
 */
#define COMMENT(str) static string get_comment(){return str;}
#define HTML_COMMENT(str) static string get_comment(){return string("<p xmlns='http://www.w3.org/1999/xhtml'>")+str+"</p>";}
#ifdef WIN32
#pragma warning(disable: 4503)
#endif
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
#define _RDFS_NAMESPACE(uri,prefix) char __uri_[]=uri;char __prefix_[]=prefix;typedef objrdf::tpair<__uri_,__prefix_> _rdfs_namespace;
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
namespace objrdf{
	 _RDFS_NAMESPACE("http://www.example.org/objrdf#","obj");
	struct NIL{
		typedef NIL SELF;
	};
	//should become rdfs::Resource
	class base_resource;
	typedef shared_ptr<base_resource> RESOURCE_PTR;
	typedef pseudo_ptr<const base_resource,free_store,true> CONST_RESOURCE_PTR;
	typedef pseudo_ptr<const rdfs::Class,free_store,false,uint16_t> CLASS_PTR;
	typedef pseudo_ptr<const rdf::Property,free_store,false,uint16_t> PROPERTY_PTR;
	typedef char PROVENANCE;
	typedef std::tuple<
		/*literal*/
		void (*)(base_resource*,string,size_t),	/* 0	set_string */
		void (*)(base_resource*,istream&,size_t),//1 in
		void (*)(const base_resource*,ostream&,size_t),//2 out
		/*non-literal*/
		shared_ptr<base_resource> (*)(base_resource*,size_t),//3 get_object
		CONST_RESOURCE_PTR (*)(const base_resource*,size_t),//4 get_const_object
		void (*)(base_resource*,shared_ptr<base_resource>,size_t),//5 set_object
		/*common*/
		size_t  (*)(const base_resource*),//6 get_size
		void (*)(base_resource*,PROVENANCE),//7 add_property	
		void (*)(base_resource*,size_t,size_t),//8 erase
		PROVENANCE (*)(const base_resource*,size_t)//9 get_provenance
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
		bool FOUND=tuple_index<PROPERTY,typename SUBJECT::PROPERTIES>::value<tuple_size<typename SUBJECT::PROPERTIES>::value
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
	

	#ifdef PERSISTENT
	template<typename PROPERTY> class array:public vector<
		PROPERTY,
		custom_allocator<
			PROPERTY,
			pseudo_ptr_array<PROPERTY,typename get_store<typename PROPERTY::PTR>::value,false> //wont work with literal properties
		>
	>{
	#else
	template<
		typename PROPERTY,
		typename ALLOCATOR=std::allocator<PROPERTY>
	> class array:public vector<PROPERTY,ALLOCATOR>{
	#endif
	public:
		typedef array SELF;
		static PROPERTY_PTR get_property(){return PROPERTY::get_property();}
		~array(){cerr<<"~array()"<<this->size()<<endl;}
	};
	//could change name to rdfs::Resource
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
		typedef std::tuple<> PROPERTIES; //base_resource does not have properties
		struct instance_iterator{
			friend class base_resource; //for base_resource::erase
			base_resource* subject;
			V::iterator i;
			size_t index;
			//would be nice to have a constructor that takes static iterator
			//instance_iterator():subject(0),index(0){} //what can we do with this?, not much
			//can we get rid of this one?
			//instance_iterator(V::iterator i,size_t index):subject(0),i(i),index(index){}
			instance_iterator(base_resource* subject,V::iterator i,size_t index):subject(subject),i(i),index(index){}
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
			shared_ptr<base_resource> get_object() const;
			CONST_RESOURCE_PTR get_const_object() const;
			base_resource* get_subject()const;
			void set_object(shared_ptr<base_resource>);
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
			PROVENANCE get_provenance();
		};
		struct const_instance_iterator{
			friend class base_resource; //for base_resource::erase
			const base_resource* subject;
			V::const_iterator i;
			size_t index;
			const_instance_iterator(const base_resource* subject,V::const_iterator i,size_t index):subject(subject),i(i),index(index){}
			const_instance_iterator& operator+=(const unsigned int& i){index+=i;return *this;}
			const_instance_iterator& operator++(){++index;return *this;}
			//tricky here
			const const_instance_iterator* operator->(){return this;}
			const const_instance_iterator& operator*(){return *this;}
			/*
 			*	this operator is a bit more expensive 
 			*/ 
			bool operator==(const const_instance_iterator& j) const{return subject==j.subject && index==j.index;}
			bool operator!=(const const_instance_iterator& j) const{return index!=j.index;}
			bool operator<(const const_instance_iterator& j) const{return index<j.index;}
			//void in(istream& is);
			void out(ostream& os) const;
			//shared_ptr<base_resource> get_object() const;
			CONST_RESOURCE_PTR get_const_object() const;
			base_resource* get_subject()const;
			//void set_object(shared_ptr<base_resource>);
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
			//void set_string(string s);
			static const_instance_iterator help();
			PROVENANCE get_provenance();


		};
		//should be moved to .cpp
		struct type_iterator:V::iterator{
			base_resource* subject;
			type_iterator(base_resource* subject,V::iterator i):V::iterator(i),subject(subject){}
			/* what is that for? */
			template<typename PROPERTY,typename RESOURCE> static type_iterator get(RESOURCE* subject){
				auto begin(RESOURCE::v.begin()),end(RESOURCE::v.end());
				//should be only done once
				while(begin!=end){
					if(begin->p==PROPERTY::get_property()) return type_iterator(subject,begin);
					++begin;
				}
				return type_iterator(subject,end);
			}
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
			const base_resource* subject;
			const_type_iterator(const base_resource* subject,V::const_iterator i):V::const_iterator(i),subject(subject){}
			/* what is that for? */
			template<typename PROPERTY,typename RESOURCE> static const_type_iterator get(RESOURCE* subject){
				auto begin(RESOURCE::v.begin()),end(RESOURCE::v.end());
				//should be only done once
				while(begin!=end){
					if(begin->p==PROPERTY::get_property()) return const_type_iterator(subject,begin);
					++begin;
				}
				return const_type_iterator(subject,end);
			}
			V::const_iterator& get_base(){return *this;}
			size_t get_size() const;
			bool literalp() const;
			PROPERTY_PTR get_Property() const;
			/*
 			* very confusing syntactic sugar
 			* because V::iterator points to property_info
 			*/
			const_type_iterator* operator->(){return this;}
			const_instance_iterator cbegin(){return const_instance_iterator(subject,*this,0);}
			const_instance_iterator cend(){return const_instance_iterator(subject,*this,get_size());}
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
		/*
 		*	only virtual functions used
 		*	what if we use a vtable stored in the pool?
 		*	p->f(...) becomes v[p.pool_ptr].f(p.index,...)
 		*	we could also play with member function pointers
 		*/ 
		/*virtual*/ ~base_resource(){
			//for pool management
			id.local[0]=0;
			//
			#ifdef OBJRDF_VERB
			cerr<<"delete base_resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		//we want to change that to rdfs::Class const* or const pseudo_ptr<const rdfs::Class> 
		/*virtual*/ CLASS_PTR get_Class() const{return get_class();};
		/*virtual*/ type_iterator begin();
		/*virtual*/ type_iterator end();
		/*virtual*/ void get_output(ostream& os);//local resources can have content accessible through a URL scheme 
		/*virtual*/ void end_resource(){};//will be invoked when finished parsing the element
		/*
		template<typename T> type_iterator fbegin(){return type_iterator(this,T::v.begin());}
		template<typename T> type_iterator fend(){return type_iterator(this,T::v.end());}
		*/
		typedef std::tuple<
			//do we even need this when using PERSISTENT?
			shared_ptr<base_resource>::pointer (*)(uri),		//constructor
			type_iterator (*)(base_resource*),	//begin
			type_iterator (*)(base_resource*),	//end
			const_type_iterator (*)(const base_resource*),//cbegin
			const_type_iterator (*)(const base_resource*)	//cend
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
		//template<typename F> void to_rdf_xml(ostream& os,F f=filter_provenance);//the document should not have loops!!!
		void to_rdf_xml(ostream& os,const PROVENANCE& p=0);//the document should not have loops!!!
		//to use in bash
		void to_turtle_pretty(ostream& os);
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		static shared_ptr<base_resource> nil/*,cycle*/;
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

	template<typename T> base_resource::type_iterator begin(base_resource* r){return base_resource::type_iterator(r,T::v.begin());}
	template<typename T> base_resource::type_iterator end(base_resource* r){return base_resource::type_iterator(r,T::v.end());}
	template<typename T> base_resource::const_type_iterator cbegin(const base_resource* r){return base_resource::const_type_iterator(r,T::v.cbegin());}
	template<typename T> base_resource::const_type_iterator cend(const base_resource* r){return base_resource::const_type_iterator(r,T::v.cend());}

	base_resource::instance_iterator operator+(const base_resource::instance_iterator& a,const unsigned int& b){
		base_resource::instance_iterator tmp(a);
		return tmp+=b;
	}

	CLASS_PTR get_class(shared_ptr<base_resource>::pointer r);
	//void to_rdf_xml(shared_ptr<base_resource> r,ostream& os);
	//const correctness???
	void to_rdf_xml(shared_ptr<base_resource>::pointer r,ostream& os);
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
	>
	struct resource:_SUPERCLASS_{
		typedef _PROPERTIES_ PROPERTIES;
		typedef _SUPERCLASS_ SUPERCLASS;
		typedef resource SELF;
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
		void operator=(const resource& r){p=r.p;}
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
		/*virtual*/ //rdfs::Class* get_Class() const{return get_class().get();};
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


	struct base{
		//all the functions we might ever use, they SHOULD NEVER be called
		void in(istream&){}
		void out(ostream&)const{}
		void set_string(std::string){}
		shared_ptr<base_resource> get_object(){assert(0);return shared_ptr<base_resource>();};
		CONST_RESOURCE_PTR get_const_object() const{assert(0);return CONST_RESOURCE_PTR(0);};
		void set_object(shared_ptr<base_resource>){assert(0);};
	};
	template<typename RANGE> struct base_property:base{
		RANGE t;
		//should it be constant?
		base_property(RANGE t=RANGE()):t(t){}
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return 1;}
	};
	template<> struct base_property<string>:base{
		string t;
		base_property(string t=string()):t(t){}
		void set_string(string s){t=s;}
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return t.size()>0;}
	};
	/*
 	* constant property, needs more work, can not be in array
 	* not really easy to use unless it is only property or tuple constructor
 	* can use default value, also will cause problem when parsing (but its value
 	* should be set programmatically anyway	
 	*/
	template<typename RANGE> struct base_property<const RANGE>:base{
		const RANGE t;
		base_property(const RANGE t=0):t(t){}
		size_t get_size() const{return 1;}
		void out(ostream& os){os<<t;}
	};
	template<int N> struct base_property<char[N]>:base{
		char t[N];
		base_property(){strcpy(t,"ola");}
		base_property(const char[N]){strcpy(t,"ola");}
		size_t get_size() const{return 1;}
		void out(ostream& os) const{os<<t;}
	};
	/*
 	*	specialization for pointer
 	*/ 
	template<
		typename RANGE
	>
	class base_property<RANGE*>:public shared_ptr<RANGE>,public base{
	public:
		typedef shared_ptr<RANGE> PTR;
	/*
 	*	can we do something about cycle:
 	*	rdfs:Class rdf:type rdfs:Class.
 	*/
		//should it be constant?
		void set(const base_property& pr){*this=pr;}
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return PTR::get()!=0;}
		shared_ptr<base_resource> get_object(){return shared_ptr<base_resource>(this->get());}
		CONST_RESOURCE_PTR get_const_object() const{return this->get();}
		void set_object(shared_ptr<base_resource> object){
			//verify type?, depending implementation it could already be done
			*this=static_pointer_cast<RANGE>(object);
		};
	};
	template<
		typename T,
		typename STORE,
		typename INDEX
	>
	class base_property<pseudo_ptr<const T,STORE,false,INDEX>>:public pseudo_ptr<const T,STORE,false,INDEX>,public base{
	public:
		typedef pseudo_ptr<const T,STORE,false,INDEX> PTR;
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}

	};
	/*
 	*	full control of implementation
 	*/ 
	template<
		typename RANGE,
		typename _PTR_
	> class non_literal_property:public _PTR_,public base{
	public:
		typedef _PTR_ PTR;
		non_literal_property(){}
		non_literal_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		//what about get_object()?
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
			LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		property()/*:p(0)*/{
			LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		~property(){
			LOG<<"delete property `"<<NAME<<"' "<<this<<endl;
		}
		static PROPERTY_PTR get_property();
	};
	//optimization: get rid of functions (maybe compiler already does it)
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct functions{
		static inline PROPERTY& get(base_resource* subject){return static_cast<SUBJECT*>(subject)->template get<PROPERTY>();}
		static inline const PROPERTY& get_const(const base_resource* subject){return static_cast<const SUBJECT*>(subject)->template get_const<PROPERTY>();}
		static void set_string(base_resource* subject,string s,size_t){get(subject).set_string(s);}
		static void in(base_resource* subject,istream& is,size_t){get(subject).in(is);}
		static void out(const base_resource* subject,ostream& os,size_t){get_const(subject).out(os);}	
		static shared_ptr<base_resource> get_object(base_resource* subject,size_t){return get(subject).get_object();}
		static CONST_RESOURCE_PTR get_const_object(const base_resource* subject,size_t){return get_const(subject).get_const_object();}
		static void set_object(base_resource* subject,shared_ptr<base_resource> object,size_t){get(subject).set_object(object);}
		static size_t get_size(const base_resource* subject){return get_const(subject).get_size();}
		static void add_property(base_resource* subject,PROVENANCE p){}
		static void erase(base_resource* subject,size_t,size_t){}	
		static PROVENANCE get_provenance(const base_resource* subject,size_t){return 0;/*get_const(subject).p;*/}
		static function_table get_table(){
			/* some functions could be made null if literal */ 
			return function_table(set_string,in,out,get_object,get_const_object,set_object,get_size,add_property,erase,get_provenance);
		}
	};
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct functions<SUBJECT,array<PROPERTY>>{
		static inline array<PROPERTY>& get(base_resource* subject){return static_cast<SUBJECT*>(subject)->template get<array<PROPERTY>>();}
		static inline const array<PROPERTY>& get_const(const base_resource* subject){return static_cast<const SUBJECT*>(subject)->template get_const<array<PROPERTY>>();}
		static void set_string(base_resource* subject,string s,size_t index){get(subject)[index].set_string(s);}
		static void in(base_resource* subject,istream& is,size_t index){get(subject)[index].in(is);}
		static void out(const base_resource* subject,ostream& os,size_t index){get_const(subject)[index].out(os);}	
		static shared_ptr<base_resource> get_object(base_resource* subject,size_t index){return get(subject)[index].get_object();}
		static CONST_RESOURCE_PTR get_const_object(const base_resource* subject,size_t index){return get_const(subject)[index].get_const_object();}
		static void set_object(base_resource* subject,shared_ptr<base_resource> object,size_t index){get(subject)[index].set_object(object);}
		static size_t get_size(const base_resource* subject){return get_const(subject).size();}
		static void add_property(base_resource* subject,PROVENANCE p){typedef PROPERTY P;get(subject).push_back(P());}
		static void erase(base_resource* subject,size_t first,size_t last){get(subject).erase(get(subject).begin()+first,get(subject).begin()+last);}
		static PROVENANCE get_provenance(const base_resource* subject,size_t index){return 0;/*get_const(subject)[index].p;*/}
		static function_table get_table(){
			return function_table(set_string,in,out,get_object,get_const_object,set_object,get_size,add_property,erase,get_provenance);
		}
	};

	//schema
	typedef base_resource* (*fpt)(uri);
	#ifdef PERSISTENT
	//problem here
	template<typename T> shared_ptr<base_resource>::pointer constructor(uri u){return shared_ptr<T>::pointer::construct(u);}
	//should get rid of this we still need it
	template<> shared_ptr<base_resource>::pointer constructor<rdfs::Class>(uri u){return shared_ptr<base_resource>::pointer(0);}
	template<> shared_ptr<base_resource>::pointer constructor<rdf::Property>(uri u){return shared_ptr<base_resource>::pointer(0);}
	#else
	template<typename T> base_resource* constructor(uri u){return new T(u);}
	//should get rid of this
	template<> base_resource* constructor<rdfs::Class>(uri u){return 0;}
	#endif
	//placement new?
	template<typename T> base_resource* constructorp(void* p,uri u){return new(p)T(u);}
	template<> base_resource* constructorp<rdfs::Class>(void* p,uri u){return 0;}

	template<typename T> fpt get_constructor(){return &constructor<T>;}
	//no need for destructor if we use pseudo_ptr
	template<typename T> void destructor(base_resource* s){static_cast<T*>(s)->~T();}
			
	struct type_p{
		CLASS_PTR t;
		type_p(CLASS_PTR t):t(t){}
		bool operator()(shared_ptr<base_resource> r) const;
	};
}

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

namespace objrdf{
	_PROPERTY(pp,base_resource*);
}
namespace rdf{
	/*
 	*	the advantage of the vector storage is it provides some information
 	*	about timing hence provenance: all the resources created at a given
 	*	time but we can not use iterators as the container is modified	
 	*/
	/*
 	*	the biggest problem now is how to deal with static resources created at
 	*	run time, they will be serialized and create problems	
 	*	we need to turn the representation into triples, we can use iterators for that
 	*/
	char _RDF_[]="RDF";
	struct RDF:objrdf::resource<rdfs_namespace,_RDF_,std::tuple<objrdf::array<objrdf::pp> >,RDF>{//document
		/*
 		*	we have 3 different containers for resources: vector,map,multimap
 		*	maybe we could get rid of vector?
 		*
 		*/
		RDF(objrdf::uri id=objrdf::uri("rdf_doc"));
		~RDF();
		//index by id
		typedef map<objrdf::uri,shared_ptr<objrdf::base_resource> > MAP;
		MAP m;
		//index by type, what is the best way to implement that?
		typedef multimap<short/*objrdf::c_index::RANGE*/,shared_ptr<objrdf::base_resource> > MULTIMAP;	
		MULTIMAP mm;
		typedef objrdf::array<objrdf::pp> V;
		void insert(shared_ptr<objrdf::base_resource> r);
		shared_ptr<objrdf::base_resource> find(objrdf::uri s);
		void to_rdf_xml(ostream& os,const objrdf::PROVENANCE& p=0);//the document should not have loops!!!
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		void to_turtle(ostream& os);
		void to_turtle_pretty(ostream& os);
		shared_ptr<objrdf::base_resource> query(objrdf::uri id);
		/*
 		* 	templated static version, the query is only run once, a type (or enum)
 		* 	needs to be associated with the resource (similar to some_class::get_class())
 		*	so each query has its own function.
 		*	shared_ptr<base_resource> r=query_static<some_resource>(); 	
 		*	the returned could also be typed 
 		*/ 	
		template<typename T> shared_ptr<objrdf::base_resource> query_static(){
			return base_resource::nil;
		}
		template<typename T> shared_ptr<T> query_t(objrdf::uri id){
			shared_ptr<objrdf::base_resource> tmp=query(id);
			return (tmp.operator->()&&(*T::get_class()<=*tmp->get_Class()))? static_pointer_cast<T>(tmp) : shared_ptr<T>();
		}
		/*
		template<typename T> vector<shared_ptr<T> > query_by_type_array(){
			vector<shared_ptr<T> > r;
			for(V::iterator i=v.begin();i<v.end();++i){
				if((*i)->get_Class()==T::get_class())
					r.push_back(*i);
			}
			return r;
		}
		*/
	};
	PROPERTY(type,objrdf::CLASS_PTR);
	/*
	char _type[]="type";typedef objrdf::property<
		rdfs_namespace,
		_type,
		rdfs::Class*,
		objrdf::non_literal_property<rdfs::Class,objrdf::CLASS_PTR> //notation a bit stupid
	> type;
	*/
}
namespace objrdf{
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
	PROPERTY(isDefinedBy,objrdf::base_resource*);
	PROPERTY(label,string);
	PROPERTY(subPropertyOf,objrdf::PROPERTY_PTR);
}
namespace rdf{
	char _Literal[]="Literal";
	struct Literal:objrdf::resource<rdfs_namespace,_Literal,std::tuple<>,Literal>{
		Literal(objrdf::uri u):SELF(u){}
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
	typedef objrdf::resource<rdfs_namespace,_XML_Literal,std::tuple<>,rdf::Literal> XML_Literal;
	//JSON type
	struct JSON{};
	char _JSON[]="JSON";
	typedef objrdf::resource<rdfs_namespace,_JSON,std::tuple<>,rdf::Literal> JSON_type;
}
namespace objrdf{
	template<> struct get_Literal<rdfs::XMLLiteral>:rdfs::XML_Literal{};
}
namespace objrdf{
	#ifndef PERSISTENT
	_PROPERTY(c_index,short);//each class has a unique index useful for fast serialization/parsing
	#endif
	_PROPERTY(superClassOf,objrdf::CLASS_PTR);
}
namespace rdfs{
	char _Class[]="Class";
	struct Class:objrdf::resource<
		rdfs_namespace,
		_Class,
		std::tuple<
			objrdf::array<subClassOf>,
			objrdf::array<objrdf::superClassOf>,
			#ifndef PERSISTENT
			objrdf::c_index
			#endif
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
		const objrdf::base_resource::class_function_table t;
		//there must be a better way to do that	
		//what about union?
		tuple_element<0,objrdf::base_resource::class_function_table>::type constructor() const{return std::get<0>(t);}	
		tuple_element<1,objrdf::base_resource::class_function_table>::type _begin() const{return std::get<1>(t);}	
		tuple_element<2,objrdf::base_resource::class_function_table>::type _end() const{return std::get<2>(t);}	
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
		#ifndef PERSISTENT
		static vector<shared_ptr<Class> >& get_instances();
		static shared_ptr<Class> create_Class(objrdf::uri id,subClassOf s,objrdf::base_resource::class_function_table t,string comment);
		#endif
		/*
 		*	inference, should be run once all classes and properties have been created
 		*/ 
		static void analyze();//could be a sparql request
		COMMENT("The class of classes.");
	};
}//end namespace rdfs
namespace objrdf{
	_PROPERTY(p_index,short);//each property has a unique index useful for fast serialization/parsing
	_PROPERTY(p_self,objrdf::PROPERTY_PTR);
}
namespace rdf{
	char _Property[]="Property";
	struct Property:objrdf::resource<
		rdfs_namespace,
		_Property,
		std::tuple<
			rdfs::domain,
			rdfs::range,
			//rdfs::subPropertyOf,
			#ifndef PERSISTENT
			objrdf::p_index,
			#endif	
			objrdf::p_self
		>,Property>{
		Property(objrdf::uri u);
		Property(objrdf::uri u,rdfs::range r,const bool literalp);
		#ifndef PERSISTENT
		static vector<shared_ptr<Property> >& get_instances();
		#endif
		const bool literalp;
		//nil property?
		static objrdf::PROPERTY_PTR nil;
		COMMENT("The class of RDF properties.");
		objrdf::base_resource::instance_iterator get_self_iterator();
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
		#ifdef PERSISTENT
		//can be simplified because we already know the pointer value
		//check if resource is derived from Literal
		typedef typename IfThenElse<
			is_derived<TMP,rdf::Literal>::value,
			pseudo_ptr<TMP,empty_store,false>,//it MUST be false otherwise template ambiguity
			pseudo_ptr<TMP,free_store,false>
		>::ResultT PTR;
		static CLASS_PTR p=CLASS_PTR::construct_at(
			//in case of persistent storage we will override old version and refresh pointers and function pointers
			POOL_PTR::get_type_id<TMP>(),
			objrdf::get_uri<NAMESPACE>(NAME),
			rdfs::subClassOf(SUPERCLASS::get_class()),
			objrdf::base_resource::class_function_table(
				constructor<TMP>,
				objrdf::begin<TMP>,
				objrdf::end<TMP>,
				objrdf::cbegin<TMP>,
				objrdf::cend<TMP>
			),
			TMP::get_comment!=SUPERCLASS::get_comment ? TMP::get_comment() : ""
		);
		return p;
		#else
		static shared_ptr<rdfs::Class> c(rdfs::Class::create_Class(
			objrdf::get_uri<NAMESPACE>(NAME),
			rdfs::subClassOf(SUPERCLASS::get_class()),
			objrdf::base_resource::class_function_table(
				constructor<TMP>,
				constructorp<TMP>,
				destructor<TMP>,
				TMP::get_class
			),
			TMP::get_comment!=SUPERCLASS::get_comment ? TMP::get_comment() : ""
			)
		);
		return c;
		#endif
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
	template<
		typename NAMESPACE,
		const char* NAME,
		typename RANGE,
		typename IMPLEMENTATION
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::get_property(){
		cerr<<"about to construct Property `"<<NAME<<"'"<<endl;
		static PROPERTY_PTR c(
			PROPERTY_PTR::construct(
				objrdf::get_uri<NAMESPACE>(NAME),
				rdfs::range(selector<RANGE>::ResultT::get_class()),
				(bool)selector<RANGE>::IS_LITERAL
			)
		);
		return c;
	}
	struct namep{
		uri n;
		namep(uri n);
		bool operator() (property_info& p) const;
	};
	template<typename SUBJECT,typename PROPERTY> property_info get_property_info(){
		property_info p(PROPERTY::get_property(),functions<SUBJECT,PROPERTY>::get_table());
		/*
 		* by now we can't modify rdfs::domain, why is it broken down in the first place?
		* because property does not know about SUBJECT, it is an important of meta information
		* very useful to optimize SPARQL queries, can we cheat?  
		* or can we store the information in a different way? we can query objects and list all properties
		* we can temporarily cast using the same store so we don't create a new store just for that
		*/
		pseudo_ptr<rdf::Property,PROPERTY_PTR::STORE,false,PROPERTY_PTR::INDEX> tmp(p.p);
		tmp->get<rdfs::domain>()=rdfs::domain(SUBJECT::get_class());
		//p.p->get<rdfs::domain>()=rdfs::domain(SUBJECT::get_class());
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
			return V(1,get_property_info<base_resource,rdf::type>());
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
		//can we get rid of that? no!
		typedef typename IfThenElse<equality<SUBCLASS,NIL>::VALUE,RESOURCE,SUBCLASS>::ResultT TMP;
		static V go(){
			//let's make sure Classes are ready
			//RESOURCE::get_class();
			LOG<<"get_generic_property:`"<<NAME<<"'"<<endl;
			V v=get_generic_property<typename SUPERCLASS::SELF>::go();
			v.push_back(get_property_info<RESOURCE,rdf::type>());
			return concat(v,std::static_for_each<PROPERTIES>(_meta_<TMP>()).v);
		}
	};
	//iterators to navigate the pools, it won't allow modification of the resources
	struct pool_iterator:pool::iterator<POOL_PTR>{
		pool_iterator(pool::iterator<POOL_PTR> i):pool::iterator<POOL_PTR>(i){}
		typedef generic_iterator<CONST_RESOURCE_PTR> cell_iterator;
		cell_iterator begin(){return cell_iterator(**this,(**this)->get_size());}
		cell_iterator end(){return cell_iterator(**this);}
	};
	pool_iterator begin(){return pool_iterator(::begin<POOL_PTR>());}
	pool_iterator end(){return pool_iterator(::end<POOL_PTR>());}
	void to_rdf_xml(ostream& os){
		os<<"<"<<rdf::_RDF<<"\n";
		uri::ns_declaration(os);
		os<<">";
		for(auto i=objrdf::begin();i<objrdf::end();++i){
			for(auto j=i.begin();j<i.end();++j){
				to_rdf_xml(*j,os);
			}
			os<<endl;
		}
		os<<"\n</"<<rdf::_RDF<<">\n";
	}


}
#ifdef PERSISTENT
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
#endif

