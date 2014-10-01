#ifndef OBJRDF_H
#define OBJRDF_H
#include <iostream>
#include <vector>
#include <string.h>
#include <map>
#include <tuple>
#include <assert.h>
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <sstream>
/*
 *	std::basic_string is not compliant with 
 *	http://www.boost.org/doc/libs/1_54_0/doc/html/interprocess/allocators_containers.html#interprocess.allocators_containers.containers_explained.stl_container_requirements
 */
//#include <boost/container/string.hpp>
#include <string>
#include <complex>
#include "uri.h"
#include "tuple_helper.h"
#include "ifthenelse.hpp"
using namespace std;
#ifdef OBJRDF_VERB
#define LOG std::cerr
#else
#define LOG if(0) cerr
#endif
#include <pool_allocator/pool_allocator.h>
template<typename T> vector<T> concat(/*const*/ vector<T>& a,const vector<T>& b){
	a.insert(a.end(),b.begin(),b.end());
	return a;
}
template <typename T> int sgn(T val){
	return (T(0) < val) - (val < T(0));
}
//what about complex value? is that the phase?
template <typename T> int sgn(complex<T> val){
	return 0;
}

#ifdef TEST_NAME
#define PROPERTY(n,...) struct n_##n{static char* name(){return #n;};};typedef objrdf::property<rdfs_namespace,n_##n,__VA_ARGS__> n
#define OBJRDF_PROPERTY(n,...) struct n_##n{static char* name(){return #n;};};typedef objrdf::property<_rdfs_namespace,n_##n,__VA_ARGS__> n
#define CLASS(n,...) struct n_##n{static char* name(){return #n;};};typedef objrdf::resource<rdfs_namespace,n_##n,__VA_ARGS__> n
#define OBJRDF_CLASS(n,...) struct n_##n{static char* name(){return #n;};};typedef objrdf::resource<_rdfs_namespace,n_##n,__VA_ARGS__> n
#else
#define PROPERTY(n,...) char _##n[]=#n;typedef objrdf::property<rdfs_namespace,_##n,__VA_ARGS__> n
#define OBJRDF_PROPERTY(n,...) char _##n[]=#n;typedef objrdf::property<_rdfs_namespace,_##n,__VA_ARGS__> n
#define CLASS(n,...) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,__VA_ARGS__> n
#define OBJRDF_CLASS(n,...) char _##n[]=#n;typedef objrdf::resource<_rdfs_namespace,_##n,__VA_ARGS__> n
#define DERIVED_CLASS(n,BASE,...) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,__VA_ARGS__,objrdf::NIL,BASE> n
#endif
/*
 *	could we define lightweight classes? to reuse code?, that would all use the same pool because they are identical
 *	they only differ by their rdfs::type
 */
//
#define DEFAULT_SET template<typename U> void set_p(U u){get<U>()=u;} //need to copy this line in any struct that specializes the function

/*
 * 	must be a public member of the Class!!!!!
 * 	multiple symbols will be defined
 * 	as an alternative could use a C++ comment and parse this file to generate RDF, preprocessor
 */
#define COMMENT(str) static string get_comment(){return str;}
#define HTML_COMMENT(str) static string get_comment(){return string("<p xmlns='http://www.w3.org/1999/xhtml'>")+str+"</p>";}
namespace objrdf{
	#ifdef TEST_NAME
	template<typename NAMESPACE> uri get_uri(string name){return uri(NAMESPACE::name().first,NAMESPACE::name().second,name);}	
	template<typename NAMESPACE,typename NAME> uri get_uri(){return uri(NAMESPACE::name().first,NAMESPACE::name().second,NAME::name());}	
	#else
	template<const char* _A,const char* _B> struct tpair{
		//enum{N=(int)_A}; would be nice to have enums for Class/Property so we can use switch statements
	};
	template<typename T> struct get_uri_help;
	template<const char* A,const char* B> struct get_uri_help<tpair<A,B>>{static uri go(string name){return uri(A,B,name);}};
	template<typename T> uri get_uri(string name){return get_uri_help<T>::go(name);}	
	#endif
}
/*
 *	uri and prefix MUST be quoted, the macro could quote but forward slashes in URI confuse syntax highlighting in VIM
 *	hash namespace vs slash namespace, use hash for now because of optimization, could catch non hash uri at compile-time or run-time
 */
#ifdef TEST_NAME
#define RDFS_NAMESPACE(uri,prefix) struct rdfs_namespace{static std::pair<char*,char*> name(){return std::pair<char*,char*>(uri,prefix);}};
#define OBJRDF_RDFS_NAMESPACE(uri,prefix) struct _rdfs_namespace{static std::pair<char*,char*> name(){return std::pair<char*,char*>(uri,prefix);}};

#else
#define RDFS_NAMESPACE(uri,prefix) char _uri_[]=uri;char _prefix_[]=prefix;typedef objrdf::tpair<_uri_,_prefix_> rdfs_namespace;
#define OBJRDF_RDFS_NAMESPACE(uri,prefix) char __uri_[]=uri;char __prefix_[]=prefix;typedef objrdf::tpair<__uri_,__prefix_> _rdfs_namespace;
#endif
namespace rdf{
	//multiple definitions, linker will complain
	RDFS_NAMESPACE("http://www.w3.org/1999/02/22-rdf-syntax-ns#","rdf");
	const static objrdf::uri _RDF=objrdf::get_uri<rdfs_namespace>("RDF");
	const static objrdf::uri ID=objrdf::get_uri<rdfs_namespace>("ID");
	const static objrdf::uri nodeID=objrdf::get_uri<rdfs_namespace>("nodeID");//for blank nodes
	const static objrdf::uri about=objrdf::get_uri<rdfs_namespace>("about");
	const static objrdf::uri resource=objrdf::get_uri<rdfs_namespace>("resource");
	const static objrdf::uri Description=objrdf::get_uri<rdfs_namespace>("Description");
	struct Property;
}
namespace rdfs{
	RDFS_NAMESPACE("http://www.w3.org/2000/01/rdf-schema#","rdfs");
	struct Class;
}
namespace objrdf{
	OBJRDF_RDFS_NAMESPACE("http://www.example.org/objrdf#","obj");
	struct NIL{
		typedef NIL SELF;
	};
	class base_resource;
}
//test to make sure we do not persist objrdf::base_resource, because of duplicates objrdf::base_resource::nil

namespace objrdf{
	enum{TEST=0};
	/*
 	* all resource pointers will be cast to this type, so it must be able to handle any pointer size, let's
 	* set it to uint32_t for now, the problem is that the back conversion will cause problem
 	*/ 
	//typedef volatile_allocator_unmanaged<base_resource,uint32_t>::derived_pointer RESOURCE_PTR;
	//typedef volatile_allocator_unmanaged<base_resource,uint32_t>::const_derived_pointer CONST_RESOURCE_PTR;
	//when did we decide to use managed???
	typedef volatile_allocator_managed<base_resource,uint32_t>::derived_pointer RESOURCE_PTR;
	typedef volatile_allocator_managed<base_resource,uint32_t>::const_derived_pointer CONST_RESOURCE_PTR;
	//we don't store classes, why?
	typedef volatile_allocator_managed<rdfs::Class,uint8_t>::pointer CLASS_PTR;
	typedef volatile_allocator_managed<rdfs::Class,uint8_t>::const_pointer CONST_CLASS_PTR;
	typedef volatile_allocator_managed<rdf::Property,uint8_t>::pointer PROPERTY_PTR;
	typedef volatile_allocator_managed<rdf::Property,uint8_t>::const_pointer CONST_PROPERTY_PTR;
	typedef char PROVENANCE;
	/*
 	*	can we define default functions that will give a helpful error message when called?
 	*	this function table can be used to modify privileges
 	*/ 
	struct function_table{
		typedef void (*set_string_f)(RESOURCE_PTR,string,size_t);
		typedef void (*in_f)(RESOURCE_PTR,istream&,size_t);
		typedef void (*out_f)(CONST_RESOURCE_PTR,ostream&,size_t);
		typedef RESOURCE_PTR (*get_object_f)(RESOURCE_PTR,size_t);//could we use CONST_RESOURCE_PTR?
		typedef CONST_RESOURCE_PTR (*cget_object_f)(CONST_RESOURCE_PTR,size_t);
		typedef void (*set_object_f)(RESOURCE_PTR,RESOURCE_PTR,size_t);
		typedef size_t (*get_size_f)(CONST_RESOURCE_PTR);
		typedef void (*add_property_f)(RESOURCE_PTR,PROVENANCE);
		typedef void (*erase_f)(RESOURCE_PTR,size_t,size_t);
		//let's make this more general using reification 
		typedef PROVENANCE (*get_provenance_f)(CONST_RESOURCE_PTR,size_t);
		typedef RESOURCE_PTR (*get_statement_f)(CONST_RESOURCE_PTR,size_t);
		typedef int (*compare_f)(CONST_RESOURCE_PTR,size_t,CONST_RESOURCE_PTR,size_t);//should be with literal but we are still using indices 
		set_string_f set_string;
		in_f in;
		out_f out;
		get_object_f get_object;
		cget_object_f cget_object;
		set_object_f set_object;
		get_size_f get_size;
		add_property_f add_property;
		erase_f erase;
		get_provenance_f get_provenance;
		get_statement_f get_statement;
		compare_f compare;
		function_table():set_string(0),in(0),out(0),get_object(0),cget_object(0),set_object(0),get_size(0),add_property(0),erase(0),get_provenance(0),get_statement(0),compare(0){}
		//we can have different constructors depending on type
		static function_table literal(in_f _in,out_f _out,get_size_f _get_size){
			function_table t;
			t.in=_in;
			t.out=_out;
			t.get_size=_get_size;
			return t;
		}
		friend ostream& operator<<(ostream& os,const function_table& f){
			os<<"in\t"<<(void*)f.in<<"\n";
			os<<"out\t"<<(void*)f.out<<"\n";
			os<<"get_object\t"<<(void*)f.get_object<<"\n";
			os<<"cget_object\t"<<(void*)f.cget_object<<"\n";
			os<<"set_object\t"<<(void*)f.set_object<<"\n";
			os<<"get_size\t"<<(void*)f.get_size<<"\n";
			os<<"add_property\t"<<(void*)f.add_property<<"\n";
			os<<"erase\t"<<(void*)f.erase<<"\n";
			os<<"get_provenance\t"<<(void*)f.get_provenance<<"\n";
			os<<"get_statement\t"<<(void*)f.get_statement<<"\n";
			os<<"compare\t"<<(void*)f.compare;
			return os;
		}
	};
	struct property_info{
		//because now stored in vector<property_info>
		/*const*/ CONST_PROPERTY_PTR p;
		function_table t;
		/*const*/ bool literalp;
		property_info(CONST_PROPERTY_PTR p,function_table t);
	};
	/*
 	*	can we access all those tables in a generic way?
 	*/ 
	typedef vector<property_info> V;
	/*
 	*	C++14 support this: http://en.cppreference.com/w/cpp/utility/tuple/get
 	*/ 
	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t INDEX=tuple_index<PROPERTY,typename SUBJECT::PROPERTIES>::value,
		bool FOUND=in_tuple<PROPERTY,typename SUBJECT::PROPERTIES>::value
	> struct helper{
		static PROPERTY& get(SUBJECT& s){return std::get<INDEX>(s.p);}
		static const PROPERTY& get_const(const SUBJECT& s){return std::get<INDEX>(s.p);}
	};
	/*
 	*	problem with using array: the information is lost in the schema, now we use 
 	*	rdfs:member http://www.w3.org/TR/rdf-schema/#ch_member
 	*/ 	
	template<
		typename PROPERTY,
		typename ALLOCATOR
	> class array:public vector<PROPERTY,ALLOCATOR>{
	public:
		enum{TYPE=PROPERTY::TYPE};
		typedef array SELF;
		static PROPERTY_PTR get_property();
		typedef typename PROPERTY::RANGE RANGE;
		array(){}
		array(initializer_list<PROPERTY> pr):vector<PROPERTY,ALLOCATOR>(pr){}
		~array(){cerr<<"~array()"<<this->size()<<endl;}
	};
	struct match_property{
		CONST_PROPERTY_PTR p;
		match_property(CONST_PROPERTY_PTR p):p(p){};
		bool operator()(const property_info& g)const{return g.p==p;}
		template<typename T> bool operator()(const T& g)const{return g.p==p;}
	};
	class base_resource{
	public:
		//global index, will be populated by
		static map<uri,RESOURCE_PTR>& get_index();
		static void do_index(RESOURCE_PTR p);
		typedef base_resource VERSION;
		typedef base_resource SELF;
		typedef volatile_allocator_managed<base_resource,uint32_t> allocator_type;
		typedef std::tuple<> PROPERTIES; //base_resource does not have properties
		struct instance_iterator{
			friend class base_resource; //for base_resource::erase
			RESOURCE_PTR subject;
			V::const_iterator i;
			size_t index;
			instance_iterator():subject(0,0),index(0){}
			instance_iterator(RESOURCE_PTR subject,V::const_iterator i,size_t index):subject(subject),i(i),index(index){}
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
			RESOURCE_PTR get_subject()const;
			void set_object(RESOURCE_PTR);
			//need those sometime
			CONST_PROPERTY_PTR get_Property() const;
			bool literalp() const;
			friend ostream& operator<<(ostream& os,const instance_iterator& i){
				if(i.literalp()) 
					i.out(os);
				else
					os<<i.get_object()->id;
				return os;
			}
			string str() const;
			void set_string(string s);
			static instance_iterator help();
			int compare(const instance_iterator&) const;	
			PROVENANCE get_provenance() const;
			//reification
			RESOURCE_PTR get_statement() const;
		};
		struct const_instance_iterator{
			friend class base_resource; //for base_resource::erase
			CONST_RESOURCE_PTR subject;
			V::const_iterator i;
			size_t index;
			const_instance_iterator():subject(0,0),index(0){}//should use nullptr
			const_instance_iterator(CONST_RESOURCE_PTR subject,V::const_iterator i,size_t index):subject(subject),i(i),index(index){}
			const_instance_iterator(const instance_iterator& _i):subject(_i.subject),i(_i.i),index(_i.index){}
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
			RESOURCE_PTR get_subject()const;
			//need those sometime
			CONST_PROPERTY_PTR get_Property() const;
			bool literalp() const;
			friend ostream& operator<<(ostream& os,const const_instance_iterator& i){
				if(i.literalp()) 
					i.out(os);
				else
					os<<i.get_const_object()->id;
				return os;
			}
			string str() const;
			static const_instance_iterator help();
			/*
 			*	for sparql result serialization, can be tricky with different range
 			*/
			int compare(const const_instance_iterator&) const;	
			PROVENANCE get_provenance() const;
			RESOURCE_PTR get_statement() const;
		};
		//should be moved to .cpp
		//that's a bit stupid, will never actually modify the function table
		//struct type_iterator:V::iterator{
		struct type_iterator:V::const_iterator{
			RESOURCE_PTR subject;
			type_iterator(RESOURCE_PTR subject,V::const_iterator i):V::const_iterator(i),subject(subject){}
			V::const_iterator& get_base(){return *this;}
			size_t get_size() const;
			bool literalp() const;
			bool constp() const;
			CONST_PROPERTY_PTR get_Property() const;
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
			CONST_RESOURCE_PTR subject;
			const_type_iterator(CONST_RESOURCE_PTR subject,V::const_iterator i):V::const_iterator(i),subject(subject){}
			V::const_iterator& get_base(){return *this;}
			size_t get_size() const;
			bool literalp() const;
			bool constp() const;
			CONST_PROPERTY_PTR get_Property() const;
			/*
 			* very confusing syntactic sugar
 			* because V::iterator points to property_info
 			*/
			const_type_iterator& operator++(){
				this->V::const_iterator::operator++();
				return *this;
			}
			const_type_iterator* operator->(){return this;}
			const_instance_iterator cbegin(){return const_instance_iterator(subject,*this,0);}
			const_instance_iterator cend(){return const_instance_iterator(subject,*this,get_size());}
		};

		void erase(instance_iterator first,instance_iterator last);
		void erase(instance_iterator position);
		base_resource(uri id);
		~base_resource();
		//shouldn't be const?
		uri id;
		static V v;
		CONST_CLASS_PTR get_Class() const{return get_class();};
		/*
 		*	gets called after generating function table, can be used to add or remove 
 		*	properties
 		*/ 
		static void patch(V& v);//patch function table
		/*
		type_iterator begin();
		type_iterator end();
		const_type_iterator cbegin() const;
		const_type_iterator cend() const;
		*/
		void end_resource(){};//will be invoked when finished parsing the element
		/*
 		*	do we have to use functions? what about storing type_iterator begin,end,...
 		*	it could work as long as iterators don't get invalidated by container (resource::v) modification, it means
 		*	that the container must be ready by the time we define the class, could be tricky
 		*	it will also be necessary if we modify the container by adding new entries to implement privileges
 		*/ 
		struct class_function_table{
			typedef void (*ctor_f)(RESOURCE_PTR,uri);
			typedef void (*dtor_f)(RESOURCE_PTR);
			typedef void (*cctor_f)(void*,CONST_RESOURCE_PTR);//why void*?
			typedef type_iterator (*begin_f)(RESOURCE_PTR);
			typedef type_iterator (*end_f)(RESOURCE_PTR);
			typedef const_type_iterator (*cbegin_f)(CONST_RESOURCE_PTR);
			typedef const_type_iterator (*cend_f)(CONST_RESOURCE_PTR);
			typedef RESOURCE_PTR (*allocate_f)();
			typedef void (*deallocate_f)(CONST_RESOURCE_PTR);
			typedef void (*get_output_f)(CONST_RESOURCE_PTR,ostream& os);
			ctor_f ctor;
			dtor_f dtor;
			cctor_f cctor;
			begin_f begin;
			end_f end;
			cbegin_f cbegin;
			cend_f cend;
			allocate_f allocate;
			deallocate_f deallocate;
			get_output_f get_output;			
			class_function_table():ctor(0),dtor(0),cctor(0),begin(0),end(0),cbegin(0),cend(0),allocate(0),deallocate(0),get_output(0){}
			class_function_table(ctor_f ctor,dtor_f dtor,cctor_f cctor,begin_f begin,end_f end,cbegin_f cbegin,cend_f cend,allocate_f allocate,deallocate_f deallocate,get_output_f get_output):ctor(ctor),dtor(dtor),cctor(cctor),begin(begin),end(end),cbegin(cbegin),cend(cend),allocate(allocate),deallocate(deallocate),get_output(get_output){}
		};
		/*
 		* 	we add information about the class to get rid of vtable	
		*	is there a reason we can't store everything in Class: one more level of indirection, 
		*	the best would be the pool, we could assign some space (char[n]) and just copy over
		*/	
		void get_output(ostream& os) const;
		static CONST_CLASS_PTR get_class();	
		template<typename U> U& get(){return helper<base_resource,U>::get(*this);}
		template<typename U> const U& get_const() const{return helper<base_resource,U>::get_const(*this);}
		//shorter name
		template<typename U> const U& cget() const{return helper<base_resource,U>::get_const(*this);}
		static RESOURCE_PTR nil;
		int p_to_xml_size(const CONST_PROPERTY_PTR p);
		COMMENT("The class resource, everything.");
	};
	//to get function pointers
	namespace f_ptr{
		//a type_iterator does not need an actual instance of the class
		template<typename T> base_resource::type_iterator begin(RESOURCE_PTR r){return base_resource::type_iterator(r,T::v.begin());}
		template<typename T> base_resource::type_iterator end(RESOURCE_PTR r){return base_resource::type_iterator(r,T::v.end());}
		template<typename T> base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return base_resource::const_type_iterator(r,T::v.cbegin());}
		template<typename T> base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return base_resource::const_type_iterator(r,T::v.cend());}
		template<typename T> RESOURCE_PTR allocate(){
			typename T::allocator_type a;
			return a.allocate(1);
		}
		template<typename T> void deallocate(CONST_RESOURCE_PTR r){
			typename T::allocator_type a;
			return a.deallocate(r,1);
		}
		template<typename T> void get_output(CONST_RESOURCE_PTR r,ostream& os){
			static_cast<const T&>(*r).get_output(os);
		}
	}
	base_resource::instance_iterator operator+(const base_resource::instance_iterator& a,const unsigned int& b);

	CONST_CLASS_PTR get_class(CONST_RESOURCE_PTR r);
	bool is_subclass(CONST_CLASS_PTR a,CONST_CLASS_PTR b);//a is derived from b
	bool is_a(CONST_CLASS_PTR a,CONST_CLASS_PTR b);
	base_resource::type_iterator begin(RESOURCE_PTR r);
	base_resource::type_iterator end(RESOURCE_PTR r);
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r);
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r);

	base_resource::const_instance_iterator get_const_self_iterator(CONST_RESOURCE_PTR r);
	base_resource::instance_iterator get_self_iterator(RESOURCE_PTR r);
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os);
	void erase(RESOURCE_PTR r,base_resource::instance_iterator first,base_resource::instance_iterator last);
	void erase(RESOURCE_PTR r,base_resource::instance_iterator position);
	
	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t INDEX
	> struct helper<SUBJECT,PROPERTY,INDEX,false>:helper<typename SUBJECT::SUPERCLASS,PROPERTY>{};
}
namespace objrdf{
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,//maybe we can improve that in g++ > 4.5
		#endif
		typename _PROPERTIES_=std::tuple<>, //MUST BE A std::tuple !!
		typename SUBCLASS=NIL,//default should be resource
		typename _SUPERCLASS_=base_resource,
		typename TRIGGER=std::tuple<>, //if you define a trigger you must derive the class to add the handlers
		typename ALLOCATOR=typename _SUPERCLASS_::allocator_type
	>
	struct resource:_SUPERCLASS_{
		typedef _PROPERTIES_ PROPERTIES;
		typedef _SUPERCLASS_ SUPERCLASS;
		typedef resource SELF;
		typedef typename IfThenElse<is_same<SUBCLASS,NIL>::value,resource,SUBCLASS>::ResultT TMP;
		/*
 		*	by default allocator should be the same as base class's 
 		*/ 
		typedef typename ALLOCATOR::template rebind<TMP>::other allocator_type;
		//typedef typename DEFAULT_ALLOCATOR::template rebind<TMP>::other allocator_type;
		//typedef ALLOCATOR allocator_type;
		/*
 		*	not optimal when no properties (std::tuple<>)
 		*	can we pass type information to properties?
 		*/ 
		PROPERTIES p;
		resource(uri id):SUPERCLASS(id){
			//risk of recursion!
			//
			#ifdef TEST_NAME
			LOG<<"create resource `"<<NAME::name()<<"' `"<<id<<"' "<<this<<endl;
			#else
			LOG<<"create resource `"<<NAME<<"' `"<<id<<"' "<<this<<endl;
			#endif
		}
		/*
 		*	all properties must be defined at once
 		*/
		resource(uri id,PROPERTIES p):SUPERCLASS(id),p(p){
			#ifdef TEST_NAME
			LOG<<"create resource `"<<NAME::name()<<"' `"<<id<<"' "<<this<<endl;
			#else
			LOG<<"create resource `"<<NAME<<"' `"<<id<<"' "<<this<<endl;
			#endif
		}
		~resource(){
			#ifdef OBJRDF_VERB
			cerr<<"delete resource `"<<this->id<<"' "<<this<<endl;
			#endif
		}
		/*
		void operator=(const resource& r){
			cerr<<"operator="<<endl;
			p=r.p;
			
		}
		*/
		template<typename U> U& get(){return helper<resource,U>::get(*this);}
		template<typename U> const U& get_const() const{return helper<resource,U>::get_const(*this);}
		template<typename U> const U& cget() const{return helper<resource,U>::get_const(*this);}
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
		static CONST_CLASS_PTR get_class();	
	};
	template<
		typename SUBJECT
	> 
	struct get_generic_property;

	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,
		typename TRIGGER,
		typename ALLOCATOR
	> V resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>::v=get_generic_property<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>>::go();

	enum{LITERAL=0x1};
	enum{STRING=0x2};
	enum{CONST=0x4};
	template<typename RANGE> struct base_property{
		enum{TYPE=LITERAL};
		RANGE t;
		//should it be constant?
		base_property(RANGE t=RANGE()):t(t){}
		//sometime we want to override those functions, eg:hex type: see struct hex_adapter
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return 1;}//would be nice to have a bit to tell us if it has been set or not
		//could also define static functions save one function call
		int compare(const base_property& a)const{
			cerr<<"compare `"<<t<<"' and `"<<a.t<<"'"<<endl;
			return sgn(t-a.t);
		}
		void erase(){t=0;}
	};
	
	template<typename T> struct hex_adapter{};
	template<typename T> struct base_property<hex_adapter<T>>:base_property<T>{
		base_property(T t=T()):base_property<T>(t){}
		//should not get rid of zeros
		void in(istream& is){is>>hex>>this->t>>dec;}
		//void out(ostream& os)const{os<<hex<<setfill('0')<<setwidth(2*sizeof(T)<<this->t<<dec;}
		void out(ostream& os)const{os<<hex<<this->t<<dec;}
	};
	template<> struct base_property<string>{
		enum{TYPE=STRING|LITERAL};
		string t;
		base_property(string s=string()):t(s){}
		void set_string(string s){t=s;}
		void in(istream& is){is>>t;}
		void out(ostream& os) const{os<<t;}
		size_t get_size() const{return !t.empty();}
		int compare(const base_property& a)const{return t.compare(a.t);}
		void erase(){t.clear();}
	};
	/*
	*	this does not work: basic_string<> does not use Allocator::pointer
	*	for its own storage, so the pool is never loaded and it gives segfault
	*	alternative: store in vector<char>
	*	we should only specialize when using persistent store
 	*/
	template<> struct base_property<uri>{
		enum{TYPE=STRING|LITERAL};
	};
	/*
 	* constant property, needs more work, can not be in array
 	* not really easy to use unless it is only property or tuple constructor
 	* can use default value, also will cause problem when parsing (but its value
 	* should be set programmatically anyway, ... not sure)
 	*/
	template<typename RANGE> struct base_property<const RANGE>{
		enum{TYPE=CONST|LITERAL};
		const RANGE t;
		base_property(const RANGE t=0):t(t){}
		void out(ostream& os){os<<t;}
		size_t get_size() const{return 1;}
		int compare(const base_property& a)const{return sgn(t-a.t);}
		void erase(){}
	};
	template<
		typename INDEX,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>
	struct base_property<pool_allocator::pool::ptr<char,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		enum{TYPE=STRING|LITERAL};
		typedef typename pool_allocator::pool::allocator<char,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT> allocator_type;
		typedef std::vector<char,allocator_type> STR;//to make our life easy
		STR str;
		base_property(const string s=string()):str(STR(s.size()+1)){
			strcpy(str.data(),s.c_str());
		}
		void set_string(string s){
			str.resize(s.size()+1);
			strcpy(str.data(),s.c_str());
		}
		void in(istream& is){
			string tmp;
			is>>tmp;
			set_string(tmp);
		}
		void out(ostream& os) const{os<<str.data();}
		size_t get_size() const{return !str.empty();}
		int compare(const base_property& a)const{return strcmp(str.data(),a.str.data());}
		void erase(){
			str.resize(1);
			str[0]=0;
		}
	};
	template<int N> struct base_property<char[N]>{
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
	//could we make this generic using traits?
	template<
		typename INDEX,
		typename VALUE_TYPE,	
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>
	class base_property<pool_allocator::pool::ptr<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>:public pool_allocator::pool::ptr<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>{
	public:
		enum{TYPE=0};
		typedef pool_allocator::pool::ptr<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT> PTR;
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}//dangerous notation
		RESOURCE_PTR get_object() const{return *this;}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		void set_object(RESOURCE_PTR object){
			//downcasting from RESOURCE_PTR to base_property	
			*this=static_cast<PTR>(object);
		}
		void erase(){set_object(PTR());}
	};
	template<
		typename INDEX,
		typename VALUE_TYPE,	
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>
	class base_property<pool_allocator::pool::ptr<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>:public pool_allocator::pool::ptr<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>{
	public:
		enum{TYPE=CONST};
		typedef pool_allocator::pool::ptr<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT> PTR;
		base_property(){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}//dangerous notation
		//RESOURCE_PTR get_object() const{return *this;}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		void set_object(RESOURCE_PTR object){
			//downcasting from RESOURCE_PTR to base_property	
			*this=static_cast<PTR>(object);
		}
		void erase(){set_object(RESOURCE_PTR(0,0));}
	};

	template<
		typename INDEX,
		typename VALUE_TYPE,	
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>
	class base_property<pool_allocator::pool::ptr_d<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>:public pool_allocator::pool::ptr_d<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>{
	public:
		enum{TYPE=0};
		typedef pool_allocator::pool::ptr_d<VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT> PTR;
		base_property():PTR(0,0){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}//dangerous notation
		RESOURCE_PTR get_object() const{return *this;}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		void set_object(RESOURCE_PTR object){
			//downcasting from RESOURCE_PTR to base_property	
			*this=static_cast<PTR>(object);
		}
		void erase(){set_object(PTR(0,0));}
		/*
		base_property& operator=(const PTR& ptr){
			
		}
		*/
	};
	template<
		typename INDEX,
		typename VALUE_TYPE,	
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>
	class base_property<pool_allocator::pool::ptr_d<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>:public pool_allocator::pool::ptr_d<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>{
	public:
		enum{TYPE=CONST};
		typedef pool_allocator::pool::ptr_d<const VALUE_TYPE,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT> PTR;
		base_property():PTR(0,0){}
		base_property(const PTR& s):PTR(s){}
		size_t get_size() const{return (bool)PTR(*this);}//dangerous notation
		//RESOURCE_PTR get_object() const{return *this;}
		CONST_RESOURCE_PTR get_const_object() const{return *this;}
		void set_object(RESOURCE_PTR object){
			//downcasting from RESOURCE_PTR to base_property	
			*this=static_cast<PTR>(object);
		}
		void erase(){set_object(PTR(0,0));}
	};
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename _RANGE_,
		typename IMPLEMENTATION=base_property<_RANGE_>
	> class property:public IMPLEMENTATION{
	public:
		PROVENANCE p;//let's get rid of that
		typedef _RANGE_ RANGE;//not the range
		typedef property SELF;
		//template<typename S> property(S s):base_property<RANGE>(s){}
		explicit property(IMPLEMENTATION r):IMPLEMENTATION(r){
			//LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		property()/*:p(0)*/{
			//LOG<<"create property `"<<NAME<<"' "<<this<<endl;
		}
		~property(){
			//LOG<<"delete property `"<<NAME<<"' "<<this<<endl;
		}
		/*
 		*	problem here: will allow copy of different properties with same implementation
 		*
 		*/ 
		property& operator=(const IMPLEMENTATION& r){
			(IMPLEMENTATION&)*this=r;
			return *this;
		}
		static PROPERTY_PTR get_property();
		static PROPERTY_PTR get_property_array();
	};
	/*
 	*	pseudo-property used for views only, always literal, always size 1 and constant
 	*/ 
	template<> class base_property<NIL>{
	public:
		enum{TYPE=LITERAL};

	};
	//alternatively, so we can set the RANGE
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename RANGE
	> class property<NAMESPACE,NAME,RANGE,NIL>{
	public:
		enum{TYPE=LITERAL};
		static PROPERTY_PTR get_property();
	};
	/*
 	*	property to keep track of versions, a pointer to the previous version
 	*	it can only be modified by the application, it would be nice to store the sequence outside of the object so as
 	*	to not modify the class when deciding to use versioning
 	*/ 
	OBJRDF_PROPERTY(prev,RESOURCE_PTR);
	OBJRDF_PROPERTY(next,RESOURCE_PTR);


	RESOURCE_PTR create_by_type(CONST_CLASS_PTR c,uri id);
	RESOURCE_PTR create_by_type(uri type,uri id);
	RESOURCE_PTR create_by_type_blank(CONST_CLASS_PTR c);
	RESOURCE_PTR clone(CONST_RESOURCE_PTR r);
	//RESOURCE_PTR clone_and_swap(CONST_RESOURCE_PTR r);
	RESOURCE_PTR clone_and_swap(RESOURCE_PTR r);
	RESOURCE_PTR create_by_type_blank(uri type);
	template<typename P> uri get_uri(const P& p){
		CONST_RESOURCE_PTR r(p);
		size_t _id=r.pool_ptr.index | (r.index<<(sizeof(r.pool_ptr.index)<<3));
		ostringstream os;
		os<<hex<<"_"<<_id;
		return uri(os.str().c_str());
	}
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct base_f{
		typedef PROPERTY PP;
		static inline PROPERTY& get(RESOURCE_PTR subject,size_t){
			//why is it derived?
			return static_cast<typename SUBJECT::allocator_type::derived_pointer>(subject)->template get<PROPERTY>();
		}
		static inline const PROPERTY& get_const(CONST_RESOURCE_PTR subject,size_t){
			//why is it derived?
			return static_cast<typename SUBJECT::allocator_type::const_derived_pointer>(subject)->template cget<PROPERTY>();
		}
		static size_t get_size(CONST_RESOURCE_PTR subject){return get_const(subject,0).get_size();}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		struct normal{
			static void erase(RESOURCE_PTR subject,size_t first,size_t last){
				get(subject,0).erase();
			}	
		};
		struct version{
			static void erase(RESOURCE_PTR subject,size_t first,size_t last){
				RESOURCE_PTR old=clone_and_swap(subject);//now subject points to the cloned resource
				ostringstream os;
				old._print(os);
				old->id=uri(subject->id.ns(),string(subject->id.local)+"."+os.str());
				//obj::next is wrong when more than 2 generations, should only use obj::prev for now
				static_cast<typename SUBJECT::allocator_type::derived_pointer>(subject)->template get<objrdf::prev>().set_object(old);
				static_cast<typename SUBJECT::allocator_type::derived_pointer>(old)->template get<objrdf::next>().set_object(subject);
				get(subject,0).erase();
			}	
		};
		typedef typename IfThenElse<std::is_same<typename SUBJECT::VERSION,PROPERTY>::value,version,normal>::ResultT ERASE;
		static PROVENANCE get_provenance(CONST_RESOURCE_PTR subject,size_t){return 0;/*get_const(subject).p;*/}
		static function_table get_table(){
			function_table t;
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=ERASE::erase;
			t.get_provenance=get_provenance;
			return t;	
		}
	};
	template<
		typename SUBJECT,
		typename PROPERTY,
		typename ALLOCATOR
	> struct base_f<SUBJECT,objrdf::array<PROPERTY,ALLOCATOR>>{
		typedef objrdf::array<PROPERTY,ALLOCATOR> ARRAY;
		typedef PROPERTY PP;
		//why is it derived_pointer?
		static inline ARRAY& get(RESOURCE_PTR subject)
		{
			return static_cast<typename SUBJECT::allocator_type::derived_pointer>(subject)->template get<ARRAY>();
		}
		static inline const ARRAY& get_const(CONST_RESOURCE_PTR subject){
			return static_cast<typename SUBJECT::allocator_type::const_derived_pointer>(subject)->template get_const<ARRAY>();
		}
		static inline PROPERTY& get(RESOURCE_PTR subject,size_t index){
			return static_cast<typename SUBJECT::allocator_type::derived_pointer>(subject)->template get<ARRAY>()[index];
		}
		static inline const PROPERTY& get_const(CONST_RESOURCE_PTR subject,size_t index){
			return static_cast<typename SUBJECT::allocator_type::const_derived_pointer>(subject)->template get_const<ARRAY>()[index];
		}
		static size_t get_size(CONST_RESOURCE_PTR subject){
			return get_const(subject).size();
		}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){
			typedef PROPERTY P;
			get(subject).push_back(P());
		}
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){
			get(subject).erase(get(subject).begin()+first,get(subject).begin()+last);
		}
		static PROVENANCE get_provenance(CONST_RESOURCE_PTR subject,size_t index){
			return 0;
		}
		static function_table get_table(){
			function_table t;
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=erase;
			t.get_provenance=get_provenance;
			return t;	
		}
	};

	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t TYPE=PROPERTY::TYPE
	> struct functions;

	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONST|LITERAL>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			/*
 			*	at this stage we can detect if out has been overriden in a base class
 			*/ 
			BASE::get_const(subject,index).out(os);
		}	
		static int compare(CONST_RESOURCE_PTR a,size_t index_a,CONST_RESOURCE_PTR b,size_t index_b){
			return BASE::get_const(a,index_a).compare(BASE::get_const(b,index_b));
		}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.out=out;
			t.compare=compare;
			return t;
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,LITERAL>:functions<SUBJECT,PROPERTY,CONST|LITERAL>{
		typedef functions<SUBJECT,PROPERTY,CONST|LITERAL> BASE;
		static void in(RESOURCE_PTR subject,istream& is,size_t index){
			BASE::get(subject,index).in(is);
		}
		template<typename LEAF> struct trigger{
			static void in(RESOURCE_PTR subject,istream& is,size_t index){
				PROPERTY tmp;
				tmp.in(is);
				//add pointer because `this' cannot be converted to pointer
				static_cast<typename LEAF::allocator_type::derived_pointer>(subject)->set_p(tmp,static_cast<typename LEAF::allocator_type::derived_pointer>(subject));
			}
			static function_table patch(function_table t){
				t.in=in;
				return t;
			}
		};
		static function_table get_table(){
			auto t=BASE::get_table();//we could drop T here
			t.in=in;
			return t;	
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,STRING|LITERAL>:functions<SUBJECT,PROPERTY,LITERAL>{
		typedef functions<SUBJECT,PROPERTY,LITERAL> BASE;
		static void set_string(RESOURCE_PTR subject,string s,size_t index){BASE::get(subject,index).set_string(s);}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.set_string=set_string;
			return t;	
		}
	};
	//what if set_object is invoked on const property?
	static void set_const_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){cerr<<"error: const property"<<endl;}
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONST>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static CONST_RESOURCE_PTR get_const_object(CONST_RESOURCE_PTR subject,size_t index){
			return BASE::get_const(subject,index).get_const_object();
		}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.cget_object=get_const_object;
			t.set_object=set_const_object;//???
			return t;	
		}
	};	
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,0>:functions<SUBJECT,PROPERTY,CONST>{
		typedef functions<SUBJECT,PROPERTY,CONST> BASE;
		static RESOURCE_PTR get_object(RESOURCE_PTR subject,size_t index){return BASE::get(subject,index).get_object();}
		static void set_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){
			BASE::get(subject,index).set_object(object);
		}
		template<typename LEAF> struct trigger{
			static void set_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){
				typename BASE::PP tmp;
				tmp.set_object(object);
				static_cast<typename LEAF::allocator_type::derived_pointer>(subject)->set_p(tmp,static_cast<typename LEAF::allocator_type::derived_pointer>(subject));
			}
			static function_table patch(function_table t){
				t.set_object=set_object;
				return t;
			}
		};
		static function_table get_table(){
			auto t=BASE::get_table();
			t.get_object=get_object;
			t.set_object=set_object;
			return t;	
		}
	};
	/*
 	*	properties are constant, how can we tell the parser?
 	*	one way is to group constant properties and put offset in type_iterator, but awkward with derived classes,
 	*	we could also return a fake property so that parsing can be carried on but will not modify the document
 	*	we can just sort by constness: interesting idea...
 	*
 	*/
	template<
		typename SUBJECT,
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename IMPLEMENTATION
	> struct functions<SUBJECT,property<NAMESPACE,NAME,NIL,IMPLEMENTATION>,LITERAL>{
		static size_t get_size(CONST_RESOURCE_PTR subject){return 1;}
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			property<NAMESPACE,NAME,NIL,IMPLEMENTATION> tmp;
			static_cast<typename SUBJECT::allocator_type::const_derived_pointer>(subject)->out_p(tmp,os);
		}
		static function_table get_table(){
			function_table t;
			t.out=out;
			t.set_object=set_const_object;
			t.get_size=get_size;
			return t;
		}
	};
	template<
		typename SUBJECT,
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename RANGE
	> struct functions<SUBJECT,property<NAMESPACE,NAME,RANGE,NIL>,LITERAL>{
		static size_t get_size(CONST_RESOURCE_PTR subject){return 1;}
		/*static void set_string(RESOURCE_PTR subject,string s,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<SUBJECT*>(subject)->set_string_p(tmp,s);//won't find function if in super-class
		}*/
		static void in(RESOURCE_PTR subject,istream& is,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<typename SUBJECT::allocator_type::derived_pointer>(subject)->in_p(tmp,is);//won't find function if in super-class
		}
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<typename SUBJECT::allocator_type::const_derived_pointer>(subject)->out_p(tmp,os);
		}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){}//idem
		static function_table get_table(){
			function_table t;
			t.in=in;
			t.out=out;
			//t.set_object=set_const_object;
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=erase;
			return t;
		}
	};

	//schema
	typedef base_resource* (*fpt)(uri);
	namespace f_ptr{
		template<typename T> void constructor(RESOURCE_PTR p,uri u){
			new(p)T(u);
			T::do_index(p);
		}
		template<typename T> void destructor(RESOURCE_PTR p){
			static_cast<typename T::allocator_type::pointer>(p)->~T();
			//should remove from index
		}
		//have to look into that, sometime we don't want a copy constructor
		template<typename T> void copy_constructor(void* p,CONST_RESOURCE_PTR r){new(p)T(static_cast<const T&>(*r));}
		//template<> void constructor<rdfs::Class>(RESOURCE_PTR p,uri u);
		template<> base_resource::type_iterator end<rdfs::Class>(RESOURCE_PTR r);
		template<> base_resource::type_iterator end<rdf::Property>(RESOURCE_PTR r);
		template<> void constructor<rdf::Property>(RESOURCE_PTR p,uri u);
	}
	struct type_p{
		CONST_CLASS_PTR t;
		type_p(CONST_CLASS_PTR t):t(t){}
		bool operator()(RESOURCE_PTR r) const;
	};
}//end namespace objrdf
namespace rdf{
	PROPERTY(type,objrdf::CONST_CLASS_PTR);
}
namespace objrdf{
	//make it possible to modify a resource's id after it has been created, it is possible because the db
	//relies on pointers, not on id
	OBJRDF_PROPERTY(id,uri);
	template<typename SUBJECT> struct functions<SUBJECT/*,SUBJECT*/,objrdf::id,STRING|LITERAL>{
		static void set_string(RESOURCE_PTR subject,string s,size_t){
			if(subject->id.is_local()) //only makes sense with local resources
				subject->id=uri(s);//could add code to detect duplicate id's
		}
		static void in(RESOURCE_PTR subject,istream& is,size_t){
			string tmp;
			is>>tmp;
			set_string(subject,tmp,0);
		}	
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t){os<<subject->id;}	
		static size_t get_size(CONST_RESOURCE_PTR subject){return 1;}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){}	
		static function_table get_table(){
			function_table t;
			t.set_string=set_string;
			t.in=in;
			t.out=out;
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=erase;
			return t;
		}
	};
	/*
 	*	very useful property that returns a pointer to the subject, it is used by the sparql engine and will also be used
 	*	later for fast resource retrieval, a pseudo_ptr can be encoded in hex very easily
 	*/ 
	OBJRDF_PROPERTY(self,CONST_RESOURCE_PTR);
	template<typename SUBJECT> struct functions<SUBJECT/*,SUBJECT*/,objrdf::self,objrdf::self::TYPE>{
		static CONST_RESOURCE_PTR get_const_object(CONST_RESOURCE_PTR subject,size_t index){return subject;}
		static void set_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){}
		static size_t get_size(CONST_RESOURCE_PTR subject){return 1;}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){}//does not have to do anything
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){}	
		static function_table get_table(){
			function_table t;
			t.cget_object=get_const_object;
			t.set_object=set_object;//why do we need this????
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=erase;
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
			return t;//that's not good: reference to local variable, could define a static member but confusing
		}
		static const rdf::type& get_const(const SUBJECT&){
			static rdf::type t(SUBJECT::get_class());
			return t;
		}
	};
}
namespace rdfs{
	PROPERTY(domain,objrdf::CONST_CLASS_PTR);
	PROPERTY(range,objrdf::CONST_CLASS_PTR);
	PROPERTY(subClassOf,objrdf::CONST_CLASS_PTR);
	PROPERTY(comment,string);
	/*
 	* some properties whose domain is rdf:Resource but we don't want to make them member
 	* of objrdf::base_resource 
 	*/
	PROPERTY(isDefinedBy,objrdf::RESOURCE_PTR);
	PROPERTY(label,string);
	PROPERTY(subPropertyOf,objrdf::CONST_PROPERTY_PTR);
	PROPERTY(member,objrdf::RESOURCE_PTR);
}
namespace rdf{
	#ifdef TEST_NAME
	struct Literal:objrdf::resource<rdfs_namespace,Literal,std::tuple<>,Literal>{
		static char* name(){return "Literal";}
		Literal(objrdf::uri u):SELF(u){}
		COMMENT("The class of literal values, eg. textual strings and integers")
	};
	#else
	char _Literal[]="Literal";
	struct Literal:objrdf::resource<rdfs_namespace,_Literal,std::tuple<>,Literal>{
		Literal(objrdf::uri u):SELF(u){}
		COMMENT("The class of literal values, eg. textual strings and integers")
	};
	#endif
}
namespace xsd{
	/*
 	*	http://www.w3.org/TR/2004/REC-rdf-mt-20040210/
 	*	not clear what the syntax should be
 	*/ 
	RDFS_NAMESPACE("http://www.w3.org/2001/XMLSchema#","xsd");//or xs?
	CLASS(integer,std::tuple<>,objrdf::NIL,rdf::Literal);
	CLASS(unsignedInt,std::tuple<>,objrdf::NIL,rdf::Literal);
	CLASS(anyURI,std::tuple<>,objrdf::NIL,rdf::Literal);
	CLASS(date,std::tuple<>,objrdf::NIL,rdf::Literal);
	CLASS(dateTime,std::tuple<>,objrdf::NIL,rdf::Literal);
	CLASS(unsignedShort,std::tuple<>,objrdf::NIL,rdf::Literal);
	#ifdef TEST_NAME
	struct _Double{static char* name(){return "double";}};typedef objrdf::resource<rdfs_namespace,_Double,std::tuple<>,objrdf::NIL,rdf::Literal> Double;
	struct _Float{static char* name(){return "float";}};typedef objrdf::resource<rdfs_namespace,_Float,std::tuple<>,objrdf::NIL,rdf::Literal> Float;
	struct _Short{static char* name(){return "short";}};typedef objrdf::resource<rdfs_namespace,_Short,std::tuple<>,objrdf::NIL,rdf::Literal> Short;
	struct _String{static char* name(){return "string";}};typedef objrdf::resource<rdfs_namespace,_String,std::tuple<>,objrdf::NIL,rdf::Literal> String;
	#else
	char _Double[]="double";typedef objrdf::resource<rdfs_namespace,_Double,std::tuple<>,objrdf::NIL,rdf::Literal> Double;
	char _Float[]="float";typedef objrdf::resource<rdfs_namespace,_Float,std::tuple<>,objrdf::NIL,rdf::Literal> Float;
	char _Short[]="short";typedef objrdf::resource<rdfs_namespace,_Short,std::tuple<>,objrdf::NIL,rdf::Literal> Short;
	char _String[]="string";typedef objrdf::resource<rdfs_namespace,_String,std::tuple<>,objrdf::NIL,rdf::Literal> String;
	#endif
}
namespace objrdf{
	OBJRDF_CLASS(Char,std::tuple<>,NIL,rdf::Literal);
}
namespace objrdf{
	template<typename T> struct get_Literal:rdf::Literal{}; 
	template<> struct get_Literal<double>:xsd::Double{};
	template<> struct get_Literal<float>:xsd::Float{};
	template<> struct get_Literal<int>:xsd::integer{};
	template<> struct get_Literal<long int>:xsd::integer{};
	template<> struct get_Literal<unsigned int>:xsd::unsignedInt{};
	template<> struct get_Literal<short>:xsd::Short{};
	template<> struct get_Literal<unsigned short>:xsd::unsignedShort{};
	template<> struct get_Literal<uint8_t>:xsd::unsignedShort{};
	template<> struct get_Literal<char>:Char{};
	template< 
		class CharT,
		class Traits,
		class Allocator
	> struct get_Literal<basic_string<CharT,Traits,Allocator>>:xsd::String{};
	template<size_t N> struct get_Literal<char[N]>:xsd::String{};
	template<> struct get_Literal<uri>:xsd::anyURI{};
	//extra types
	//template<> struct get_Literal<size_t>:xsd::unsignedInt{};//not accurate	
	template<> struct get_Literal<bool>:xsd::integer{};
	//what about complex numbers?
	//complex numbers could we define it only if complex is defined???
	//template<typename T> struct get_Literal<complex<T>>:get_Literal<T>{}; 
}
namespace rdfs{
	/*
		XMLLiteral should be (double-) quoted when using turtle notation
		but NOT when using XML serialization of SPARQL reply
	*/
	struct XMLLiteral{}; //symbolic type
	CLASS(XML_Literal,std::tuple<>,objrdf::NIL,rdf::Literal);
}
namespace objrdf{
	template<> struct get_Literal<rdfs::XMLLiteral>:rdfs::XML_Literal{};
}
namespace objrdf{
	OBJRDF_PROPERTY(superClassOf,objrdf::CONST_CLASS_PTR);
	/*
 	*	in memory size, we could also add info about pool management: rw/ro, usage,...
 	*/ 
	OBJRDF_PROPERTY(sizeOf,size_t);
	OBJRDF_PROPERTY(hashOf,hex_adapter<size_t>);
}
namespace objrdf{
	/*
 	*	users have privileges on classes and the associated properties
 	*	user 1 is root
 	*	password is md5sum encrypted for now
 	*	when running in a shell or through httpd there could be a pseudo-property that returns the user
 	*/ 
	/*
	PROPERTY(on,rdfs::Class::allocator_type::const_pointer);
	CLASS(Privilege,std::tuple<type,array<on>>);
	*/
	OBJRDF_CLASS(User,std::tuple<>,NIL,base_resource,std::tuple<>,persistent_allocator_managed<void>);
	/*
	char _User[]="User";
	struct User:objrdf::resource<_rdfs_namespace,_User,std::tuple<>,User>{
		//could also be made transient and users are created at run time for security
		typedef persistent_allocator_managed<User> allocator_type;
		User(objrdf::uri id):SELF(id){}
	};
	*/
	typedef User::allocator_type::const_pointer CONST_USER_PTR;
	//role based version
	base_resource::type_iterator begin(RESOURCE_PTR r,CONST_USER_PTR u);
	base_resource::type_iterator end(RESOURCE_PTR r,CONST_USER_PTR u);
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r,CONST_USER_PTR u);
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r,CONST_USER_PTR u);
	void get_output(CONST_RESOURCE_PTR r,ostream& os);
	//to investigate new method to store indices
	OBJRDF_CLASS(Test_class,std::tuple<>);
	/*	version control, use the git commit hash + date
	*	<objrdf:Version>
	*		<objrdf:hash  rdf:resource='https://github.com/jean-marc/objrdf/commit/cd5475382d68999730a6b7623dfdfd0b1026b8ad'/>
	*	</objrdf:Version>
	*	we could store so we know how to read any database dump or we could just use a text file
	*	we need a way to store external references
	*
	*/
}
namespace rdfs{
	#ifdef TEST_NAME
	struct Class:objrdf::resource<rdfs_namespace,Class,
	#else
	char _Class[]="Class";
	struct Class:objrdf::resource<rdfs_namespace,_Class,
	#endif
		std::tuple<
			objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>,
			objrdf::array<objrdf::superClassOf,volatile_allocator_unmanaged<objrdf::superClassOf,uint16_t>>,
			comment,
			isDefinedBy,
			objrdf::sizeOf,
			objrdf::hashOf
		>,
		Class,
		objrdf::base_resource,
		std::tuple<>,
		volatile_allocator_managed<void,uint8_t>
	>{
		#ifdef TEST_NAME
		static char* name(){return "Class";}
		#endif
		//convenience typedef to retrieve properties
		typedef std::tuple_element<0,PROPERTIES>::type array_subClassOf;
		typedef std::tuple_element<1,PROPERTIES>::type array_superClassOf;
		/*
 		*	should store all the information about the resources including function pointers, the only problem with that
 		*	is the user might want to add his own function pointer (unless he decides to use virtual functions) and that
 		*	would mean defining (deriving) her own rdfs::Class class	
 		*	A pointer to a resource should be coupled with one to the resource's rdfs::Class but still use
 		*	the standard pointer semantic 
 		*	similar to instance_iterator::get_Property(), although instance_iterator never returns an actual reference/pointer 
 		*	to the underlying property.
 		*
 		*	can we store the pool inside the Class?, it would make sense
 		*	when creating a Class, first we look in the array:
 		*		if the Class exists, we just refresh the function table 
 		*		if not: append the new Class
 		*	so in this case the Class index would decide the pool index, not the other way
 		*	the advantage is that we could catch modification to the class definition using a hash function
 		*
 		*/
		/*
 		*	is it possible to store multiple function tables corresponding to different users?
 		*
 		*/
		const objrdf::base_resource::class_function_table t;
		Class(objrdf::uri u);
		Class(objrdf::uri id,subClassOf s,objrdf::base_resource::class_function_table t,string comment,objrdf::sizeOf,objrdf::hashOf);
		~Class(){
			cerr<<"delete Class `"<<id<<"'"<<endl;	
		}
		//why doesn't this compile?
		//static objrdf::CONST_CLASS_PTR super(objrdf::CLASS_PTR c){
		static objrdf::CONST_CLASS_PTR super(objrdf::CONST_CLASS_PTR c){
			for(auto i=c->cget<array_subClassOf>().cbegin();i<c->cget<array_subClassOf>().end();++i){
				//problem *i is CONST_CLASS_PTR, we need to cast away constness
				rdfs::Class::allocator_type::pointer tmp((*i).index);
				tmp->get<array_superClassOf>().push_back(objrdf::superClassOf(c));
			}
			//let us index the new class here
			Class::do_index(rdfs::Class::allocator_type::pointer(c.index));//cast away constness
			return c;
		}
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
	#ifdef TEST_NAME
	struct Property:objrdf::resource<rdfs_namespace,Property,
	#else	
	char _Property[]="Property";
	struct Property:objrdf::resource<rdfs_namespace,_Property,
	#endif
		std::tuple<
			//rdfs::domain,//same property could applies to different unrelated classes (might be in conflict with strict RDFS)
			objrdf::array<rdfs::domain,volatile_allocator_unmanaged<rdfs::domain,uint16_t>>,
			rdfs::range,
			rdfs::subPropertyOf
		>,
		Property,
		objrdf::base_resource,
		std::tuple<>,
		volatile_allocator_managed<void,uint8_t>//we could have more than 256 propertie?
	>{
		#ifdef TEST_NAME
		static char* name(){return "Property";}
		#endif
		typedef std::tuple_element<0,PROPERTIES>::type domains;
		Property(objrdf::uri u);
		Property(objrdf::uri u,rdfs::range r,const bool literalp);
		Property(objrdf::uri u,rdfs::range r,const bool literalp,rdfs::subPropertyOf);
		const bool literalp;
		COMMENT("The class of RDF properties.");
	};
}//end namespace rdf

namespace objrdf{
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,
		typename TRIGGER,
		typename ALLOCATOR
	>
	CONST_CLASS_PTR resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>::get_class(){
		typedef typename IfThenElse<std::is_same<SUBCLASS,NIL>::value,resource,SUBCLASS>::ResultT TMP;
		static CONST_CLASS_PTR p=rdfs::Class::super(rdfs::Class::allocator_type::construct_allocate_at(
			//in case of persistent storage we will override old version and refresh pointers and function pointers
			//we have to find the index of pool where instances are stored, this is also where the pool is initialized
			//this is where the pool is created, actually it could have already been created 
			TMP::allocator_type::get_pool().index,
			#ifdef TEST_NAME
			objrdf::get_uri<NAMESPACE,NAME>(),
			#else
			objrdf::get_uri<NAMESPACE>(NAME),
			#endif
			rdfs::subClassOf(SUPERCLASS::get_class()),
			objrdf::base_resource::class_function_table(
				f_ptr::constructor<TMP>,
				f_ptr::destructor<TMP>,
				f_ptr::copy_constructor<TMP>,
				f_ptr::begin<TMP>,
				//if the pool is not writable all instances will be locked
				TMP::allocator_type::get_pool()->writable ? 
					static_cast<objrdf::base_resource::type_iterator (*)(RESOURCE_PTR)>(f_ptr::end<TMP>) : 
					static_cast<objrdf::base_resource::type_iterator (*)(RESOURCE_PTR)>(f_ptr::begin<TMP>),
				f_ptr::cbegin<TMP>,
				f_ptr::cend<TMP>,
				f_ptr::allocate<TMP>,
				f_ptr::deallocate<TMP>,
				f_ptr::get_output<TMP>
			),
			TMP::get_comment!=SUPERCLASS::get_comment ? TMP::get_comment() : "",
			objrdf::sizeOf(sizeof(TMP)),
			objrdf::hashOf(pool_allocator::pool::get_hash<TMP>())
		));
		//is that a good place to generate indexes?
		static auto tmp=TMP::allocator_type::_index();
		return p;
	}
	//there should be a cleaner way to do that
	template<typename RANGE> struct selector{
		typedef get_Literal<RANGE> ResultT;
		enum{IS_LITERAL=1};
	};
	template<
		typename INDEX,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>struct selector<pool_allocator::pool::ptr<char,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		typedef xsd::String ResultT;
		enum{IS_LITERAL=1};
	};
	template<
		typename INDEX,
		typename PAYLOAD,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>struct selector<pool_allocator::pool::ptr<PAYLOAD,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		typedef PAYLOAD ResultT;
		enum{IS_LITERAL=0};
	};
	/*
 	*	we need to be able to tell if the property is an array
 	*
 	*/
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename RANGE,
		typename IMPLEMENTATION
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::get_property(){
		static PROPERTY_PTR c=rdf::Property::allocator_type::construct_allocate(
				#ifdef TEST_NAME
				objrdf::get_uri<NAMESPACE,NAME>(),
				#else
				objrdf::get_uri<NAMESPACE>(NAME),
				#endif
				rdfs::range(selector<RANGE>::ResultT::get_class()),
				property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::TYPE&LITERAL
		);
		return c;
	}
	//pseudo property
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename RANGE
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,NIL>::get_property(){
		static PROPERTY_PTR c(
			rdf::Property::allocator_type::construct_allocate(
				#ifdef TEST_NAME
				objrdf::get_uri<NAMESPACE,NAME>(),
				#else
				objrdf::get_uri<NAMESPACE>(NAME),
				#endif
				rdfs::range(RANGE::get_class()),
				property<NAMESPACE,NAME,RANGE,NIL>::TYPE&LITERAL
			)
		);
		return c;
	}
	//creates duplicates!!!!
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename RANGE,
		typename IMPLEMENTATION
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::get_property_array(){
		static PROPERTY_PTR c(
			rdf::Property::allocator_type::construct_allocate(
				#ifdef TEST_NAME
				objrdf::get_uri<NAMESPACE,NAME>(),
				#else
				objrdf::get_uri<NAMESPACE>(NAME),//let's modify the name to avoid ambiguity
				#endif
				rdfs::range(selector<RANGE>::ResultT::get_class()),
				property<NAMESPACE,NAME,RANGE,IMPLEMENTATION>::TYPE&LITERAL,
				rdfs::subPropertyOf(static_cast<CONST_PROPERTY_PTR>(rdfs::member::get_property()))
			)
		);
		return c;
	}
	template<
		typename PROPERTY,
		typename ALLOCATOR
	> PROPERTY_PTR array<PROPERTY,ALLOCATOR>::get_property(){
		/*
 		*	temporary fix, property and array<property> are 2 distinct properties with the SAME uri -> confusion
 		*/
		//return PROPERTY::get_property_array();
		return PROPERTY::get_property();
	}
	struct name_p{
		const uri n;
		name_p(const uri n);
		bool operator()(const property_info& p) const;
	};
	/*
	*	enforces consistency between the stores:
	*	subject		object
	*	persistent	persistent	ok
	*	persistent	volatile	no!
	*	volatile	persistent	ok (but might need garbage collection)
	*	volatile	volatile	ok
	*	
	*/
	/*
	template<typename SUBJECT_STORE,typename OBJECT_STORE> struct validate_store{enum{value=1};};
	template<> struct validate_store<persistent_store,free_store>{enum{value=0};};

	template<
		typename SUBJECT,
		typename PROPERTY,
		bool IS_LITERAL=PROPERTY::TYPE&LITERAL
	> struct help_validate_store:validate_store<typename SUBJECT::allocator_type::STORE,typename selector<typename PROPERTY::RANGE>::ResultT::allocator::STORE>{};
	//could still be allocated on the heap: eg. string
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct help_validate_store<SUBJECT,PROPERTY,true>{enum{value=1};};
	*/	

	template<
		typename SUBJECT,
		typename PROPERTY
	> property_info get_property_info(){
		//what happens when array of properties?
		PROPERTY::get_property()->get<rdf::Property::domains>().push_back(rdfs::domain(SUBJECT::get_class()));
		return property_info(PROPERTY::get_property(),functions<SUBJECT,PROPERTY>::get_table());
	};
	template<
		typename SUBJECT
	> struct _meta_{
		V v;
		template<typename PROPERTY> void operator()(){
			//would be nice to have message
			//static_assert(help_validate_store<SUBJECT,PROPERTY>::value,"inconsistent stores");
			/*if(!help_validate_store<SUBJECT,PROPERTY>::value){
				cerr<<SUBJECT::get_class()->id<<" "<<PROPERTY::get_property()->id<<endl;
				exit(1);
			}*/
			v.push_back(get_property_info<SUBJECT,PROPERTY>());
		};
	};
	template<> struct get_generic_property<base_resource>{
		static V go(){
			LOG<<"get_generic_property:`base_resource'"<<endl;
			V v={
				get_property_info<base_resource,rdf::type>(),
				get_property_info<base_resource,objrdf::self>(),
				get_property_info<base_resource,objrdf::id>()
			};
			return v;
		}
	};
	template<typename SUBJECT> struct patch{
		V v;
		patch(const V& v):v(v){}
		template<typename PROPERTY> void operator()(){
			cerr<<"trigger on property `"<<PROPERTY::get_property()->id.local<<"'"<<endl;
			//look-up property in array
			auto i=find_if(v.begin(),v.end(),[](property_info& p){return p.p==PROPERTY::get_property();});
			if(i!=v.end()){
				cerr<<"patching function table for `"<<i->p->id.local<<"'"<<endl;
				i->t=functions<SUBJECT,PROPERTY>::template trigger<SUBJECT>::patch(i->t);				
			}
		}
	};
	template<
		typename NAMESPACE,
		#ifdef TEST_NAME
		typename NAME,
		#else
		const char* NAME,
		#endif
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,
		typename TRIGGER,
		typename ALLOCATOR
	> struct get_generic_property<
		resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>
	>{
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR> RESOURCE;
		typedef typename IfThenElse<is_same<SUBCLASS,NIL>::value,RESOURCE,SUBCLASS>::ResultT TMP;
		static V go(){
			#ifdef TEST_NAME
			LOG<<"get_generic_property:`"<<NAME::name()<<"'"<<endl;
			#else
			LOG<<"get_generic_property:`"<<NAME<<"'"<<endl;
			#endif
			V v=get_generic_property<typename SUPERCLASS::SELF>::go();
			/*
 			* multiple rdf:type properties, would be more consistent to have an array or a single type
 			* actually not that simple because it is a pseudo property
 			* for now the best is a single property
 			*/
			v.front()=get_property_info<RESOURCE,rdf::type>();//will override SUPERCLASS's rdf::type
			//filter properties for convenience, we need to store index of first non-const property somewhere
			auto r=concat(v,std::static_for_each<PROPERTIES>(_meta_<TMP>()).v);
			//need to process triggers at this stage: 
			#ifdef TEST_NAME
			cerr<<"listing triggers for class `"<<NAME::name()<<"'"<<endl;
			#else
			cerr<<"listing triggers for class `"<<NAME<<"'"<<endl;
			#endif
			r=std::static_for_each<TRIGGER>(patch<TMP>(r)).v;
			//make sure we only invoke once
			if(TMP::patch!=SUPERCLASS::patch){
				TMP::patch(r);//each class can decide to modify table: add pseudo-properties,...
				//if a property is added 
			}
			return r;
		}
	};
	struct test_by_uri{
		const uri u;
		test_by_uri(const uri& u):u(u){}
		//must be a pseudo_ptr<>
		template<typename T> bool operator()(const T& t) const{return t.id==u;}
	};
	void to_rdf_xml(ostream& os);
	//dumb scanner
	RESOURCE_PTR find(uri u);
	void generate_index();
	//not as dumb because uses a single pool
	//could be specialized for rdfs::Class and rdf::Property and use map<> index
	template<typename T> typename T::allocator_type::pointer find_t(uri u){
		cerr<<"looking up uri `"<<u<<"' in pool `"<<T::get_class()->id<<"'"<<endl;
		typename T::allocator_type a;
		auto r=find_if(a.cbegin(),a.cend(),test_by_uri(u));
		return typename T::allocator_type::pointer(r!=a.cend() ? r.get_cell_index() : 0);
	}
	ostream& operator<<(ostream& os,const property_info& i);
}
#endif
