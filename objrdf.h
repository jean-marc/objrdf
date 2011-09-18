#ifndef OBJRDF_H
#define OBJRDF_H
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <assert.h>
#include "shared_ptr.h"
//#include <memory>
#include "ifthenelse.hpp"
#include <algorithm>
#include <stdexcept>
#include "uri.h"
using namespace std;
using namespace special;//for shared_ptr
template<typename T> vector<T> concat(/*const*/ vector<T>& a,const vector<T>& b){
	a.insert(a.end(),b.begin(),b.end());
	return a;
}
#define PROPERTY(n,range) char _##n[]=#n;typedef objrdf::property<rdfs_namespace,_##n,range> n;
#define _PROPERTY(n,range) char _##n[]=#n;typedef objrdf::property<_rdfs_namespace,_##n,range> n;
#define CLASS0(n) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n> n;
#define CLASS1(n,p0) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,p0> n;
#define CLASS2(n,p0,p1) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::duo<p0,p1> > n;
#define CLASS3(n,p0,p1,p2) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2> > n;
#define CLASS4(n,p0,p1,p2,p3) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3> > n;
#define CLASS5(n,p0,p1,p2,p3,p4) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3,p4> > n;
#define CLASS6(n,p0,p1,p2,p3,p4,p5) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3,p4,p5> > n;

#define DEFAULT_SET template<typename U> void set(U u){help_set<U>::go(this,u);}//need to copy this line in any struct that specializes the function
/*
 * 	must be a public member of the Class!!!!!
 * 	multiple symbols will be defined
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
}
/*
 *	uri and prefix MUST be quoted, the macro could quote but forward slashes in URI confuse syntax highlighting in VIM
 *	hash namespace vs slash namespace, use hash for now because of optimization, could catch non hash uri at compile-time or run-time
 */
#define RDFS_NAMESPACE(uri,prefix) char _uri_[]=uri;char _prefix_[]=prefix;typedef objrdf::tpair<_uri_,_prefix_> rdfs_namespace;
#define _RDFS_NAMESPACE(uri,prefix) char __uri_[]=uri;char __prefix_[]=prefix;typedef objrdf::tpair<__uri_,__prefix_> _rdfs_namespace;
namespace rdf{
	//multiple definitions!!!
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
namespace objrdf{
	 _RDFS_NAMESPACE("http://www.example.org/objrdf#","obj");
	struct NIL{
		typedef NIL SELF;
	};
	template<typename T0,typename T1> struct duo{
		typedef duo SELF;
	};
	template<
		typename T_0     ,typename T_1 =NIL,typename T_2 =NIL,typename T_3 =NIL,
		typename T_4 =NIL,typename T_5 =NIL,typename T_6 =NIL,typename T_7 =NIL,
		typename T_8 =NIL,typename T_9 =NIL,typename T_A =NIL,typename T_B =NIL,
		typename T_C =NIL,typename T_D =NIL,typename T_E =NIL,typename T_F =NIL,
		typename T_10=NIL,typename T_11=NIL,typename T_12=NIL,typename T_13=NIL,
		typename T_14=NIL,typename T_15=NIL,typename T_16=NIL,typename T_17=NIL,
		typename T_18=NIL,typename T_19=NIL,typename T_1A=NIL,typename T_1B=NIL,
		typename T_1C=NIL,typename T_1D=NIL,typename T_1E=NIL,typename T_1F=NIL
	> struct tuple:duo<T_0,typename tuple<T_1,T_2,T_3,T_4,T_5,T_6,T_7,T_8,T_9,T_A,T_B,T_C,T_D,T_E,T_F,T_10,T_11,T_12,T_13,T_14,T_15,T_16,T_17,T_18,T_19,T_1A,T_1B,T_1C,T_1D,T_1E,T_1F>::SELF>{
		typedef duo<T_0,typename tuple<T_1,T_2,T_3,T_4,T_5,T_6,T_7,T_8,T_9,T_A,T_B,T_C,T_D,T_E,T_F,T_10,T_11,T_12,T_13,T_14,T_15,T_16,T_17,T_18,T_19,T_1A,T_1B,T_1C,T_1D,T_1E,T_1F>::SELF> SELF;
	};
	template<typename T_0> struct tuple<T_0,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL,NIL>:T_0{
		typedef T_0 SELF;
	};
	//enum{FALSE=0,TRUE,ARRAY}
	template<typename T> struct get_size{enum{VALUE=sizeof(T)};};
	template<typename T0,typename T1> struct get_size<duo<T0,T1> >{enum{VALUE=get_size<T0>::VALUE+get_size<T1>::VALUE};};
	template<typename T> struct get_n{enum{VALUE=1};};
	template<typename T0,typename T1> struct get_n<duo<T0,T1> >{enum{VALUE=get_n<T0>::VALUE+get_n<T1>::VALUE};};

	template<typename T,typename S> struct help{enum{VALUE=0};};
	template<typename T> struct help<T,T>{enum{VALUE=1};};
	//how to deal with p_array<>
	template<typename T0,typename T1,typename R> struct help<duo<T0,T1>,R>{enum{VALUE=help<T0,R>::VALUE | help<T1,R>::VALUE};};

	template<typename T,typename R> struct get_offset;
	template<typename T> struct get_offset<T,T>{enum{VALUE=0};};
	template<typename A,typename B> struct _plus_{enum{VALUE=A::VALUE+B::VALUE};};
	template<typename T0,typename T1,typename R> struct get_offset<duo<T0,T1>,R>{
		enum{VALUE=IfThenElse<help<T0,R>::VALUE,get_offset<T0,R>,_plus_<get_size<T0>,get_offset<T1,R> > >::ResultT::VALUE};	
	};
	template<typename P> struct p_array:vector<P>{
		typedef p_array SELF;
		static shared_ptr<rdf::Property> get_property(){return P::get_property();}
	};
	template<typename T> struct help<p_array<T>,T>{enum{VALUE=2};};
	class base_resource;
	typedef shared_ptr<base_resource> RES_PTR;
	/*
 	*	not very efficient: two level of indirections: array then vtable, it would be
 	*	better to store the vtable in the array, maybe we could reuse function?
 	*	a lot of them are similar 
 	*
 	*/ 
	class generic_property{
	public:
		typedef short PROVENANCE;
		const shared_ptr<rdf::Property> p;
		const bool literalp;
		int offset;
		virtual void set_string(base_resource*,string s);
		virtual void in(base_resource* subject,istream& is,int index);
		virtual void out(base_resource* subject,ostream& os,int index);
		virtual int get_size(base_resource* subject)=0;
		virtual void add_property(base_resource* subject,int index,PROVENANCE p);//could have insert instead
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index);
		virtual void set_object(base_resource* subject,shared_ptr<base_resource> object,int index);
		virtual void erase(base_resource* subject,int first,int last);
		generic_property(shared_ptr<rdf::Property> p,const bool literalp);
		void print() const;
		/*
 		*	returns a number that indicates the origin of the statement (distinct from resource ownership), 
 		*	a statement can be created from:
 		*	.parsing a document
 		*	.SPARQL update query
 		*	.program instructions
 		*
 		*	0->default value
 		*	1->main program(static variables,...)
 		*	2->...
 		*
 		*	it can help memory management: a literal property with default value can be discarded because
 		*	it wasn't created by anybody
 		*/ 
		virtual PROVENANCE get_provenance(base_resource* subject,int index)=0;
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		typename PREDICATE_SELF=typename PREDICATE::SELF
	> class _property_;
	typedef vector<generic_property*> V;
	template<typename SUBJECT> struct _meta_{
		V v;
		int n; //byte size
		_meta_(int _n){n=_n;}
		template<typename PREDICATE> void go(){
			v.push_back(new _property_<SUBJECT,PREDICATE>());
			v.back()->offset=n;
			n+=get_size<PREDICATE>::VALUE;
		};
	};
	template<typename T,typename FUNCTION> struct for_each_impl{
		static FUNCTION go(FUNCTION f){
			f.template go<T>(); 
			return f;	
		}
	};
	template<typename FUNCTION> struct for_each_impl<NIL,FUNCTION>{
		static FUNCTION go(FUNCTION f){return f;}
	};
	template<
		typename T0,
		typename T1,
		typename FUNCTION
	> struct for_each_impl<duo<T0,T1>,FUNCTION>{
		static FUNCTION go(FUNCTION f){
			f=for_each_impl<T0,FUNCTION>::go(f);
			f=for_each_impl<T1,FUNCTION>::go(f);
			return f;
		}
	}; 
	template<typename T,typename FUNCTION> FUNCTION for_each(FUNCTION f){
		return for_each_impl<T,FUNCTION>::go(f);
	};
	class base_resource{
		/*
 		*	not a great idea to inherit from V::iterator, would
 		*	be better to use an index because the number of properties per
 		*	class is limited (<256), but then we need a reference to the V array
 		*	also 
 		*/ 
		//should be moved to .cpp
		struct iterator:V::iterator{//need a typedef!!
			typedef V::iterator BASE;
			base_resource* subject;
			iterator():subject(0){}
			iterator(base_resource* subject,V::iterator i):BASE(i),subject(subject){}
			shared_ptr<rdf::Property> get_Property() const;
			int get_size() const;
			bool literalp() const;
			//still being used? no, only makes sense with function pointer
			//that means offset is not used either
			inline void* current() const {return (char*)subject+BASE::operator*()->offset;}
		};
	public:
		//to be investigated ...
		//template<typename T> friend class special::shared_ptr<T>; 
		typedef base_resource SELF;
		//should be moved to .cpp
		/*
 		*	private so it does not inherit iterator comparison operators
 		*/ 
		struct instance_iterator:private base_resource::iterator{
			friend class base_resource; //for base_resource::erase
			unsigned int index;
			instance_iterator():index(0){} //what can we do with this?, not much
			instance_iterator(base_resource::iterator i,int index):base_resource::iterator(i),index(index){}
			instance_iterator(base_resource* subject,V::iterator i,int index):base_resource::iterator(subject,i),index(index){}
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
			void set_object(shared_ptr<base_resource>);
			//need those sometime
			shared_ptr<rdf::Property> get_Property() const;
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
			generic_property::PROVENANCE get_provenance();
		};
		//should be moved to .cpp
		struct type_iterator:base_resource::iterator{
			type_iterator(base_resource* subject,V::iterator i):base_resource::iterator(subject,i){}
			type_iterator* operator->(){return this;}
			instance_iterator begin(){return instance_iterator(*this,0);}
			instance_iterator end(){return instance_iterator(*this,get_size());}
			instance_iterator add_property(generic_property::PROVENANCE p);
		};
		virtual type_iterator begin();
		virtual type_iterator end();
		void erase(instance_iterator first,instance_iterator last);
		void erase(instance_iterator position);
		virtual void end_resource(){};//will be invoked when finished parsing the element
		/*
 		* intrusive reference counting pg167 `Modern C++ design' Alexandrescu 
 		* problem: n is a pretty common variable name, should change or use setter/getter
 		* should use std::shared_ptr<> instead: no it does not work!!!
 		* must be made private!
 		*/
		short n;
		base_resource(uri id):n(0),id(id){
			#ifdef OBJRDF_VERB
			cerr<<"create resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		virtual ~base_resource(){
			#ifdef OBJRDF_VERB
			cerr<<"delete resource `"<<id<<"' "<<this<<endl;
			#endif
		}
		uri id;
		static V v;
		virtual rdfs::Class* get_Class() const{return get_class().get();};
		static shared_ptr<rdfs::Class> get_class();	
		void to_turtle(ostream& os);
		void to_xml(ostream& os);
		void to_xml_leaf(ostream& os);
		static bool filter_provenance(generic_property::PROVENANCE& p){return true;}
		//template<typename F> void to_rdf_xml(ostream& os,F f=filter_provenance);//the document should not have loops!!!
		void to_rdf_xml(ostream& os,const generic_property::PROVENANCE& p=0);//the document should not have loops!!!
		//to use in bash
		void to_turtle_pretty(ostream& os);
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		static shared_ptr<base_resource> nil/*,cycle*/;
		/*
 		*	local resources can have content accessible through a URL scheme 
 		*/ 
		virtual void get_output(ostream& os);
		template<typename U,int FOUND=0> struct _help_{
			//should never get here make sure compilation stops here
			typedef typename U::IT_IS_NOT_A_MEMBER VALUE;
		};
		int p_to_xml_size(const shared_ptr<rdf::Property> p);
		bool is_a(const shared_ptr<rdfs::Class>&) const;
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
	template<
		typename NAMESPACE,
		const char* NAME,
		typename _PROPERTIES_=NIL,//would be nice if we could use tuple<> instead of tuple<>::SELF
		typename SUPERCLASS=NIL,
		typename SUBCLASS=base_resource	
	>
	struct resource:SUBCLASS{
		typedef typename _PROPERTIES_::SELF PROPERTIES;
		struct initialize{
			resource* r;
			initialize(resource* r):r(r){}	
			template<typename P> void go(){new(&r->get<P>()) P;}//placement new
		};
		struct destroy{
			resource* r;
			destroy(resource* r):r(r){}	
			template<typename P> void go(){
				#ifdef OBJRDF_VERB
				cerr<<"delete property `"<<P::get_property()->id<<"' from "<<r<<endl;
				#endif
				r->get<P>().~P();
			}//placement delete
		};
		typedef resource SELF;
		char bin[get_size<PROPERTIES>::VALUE];
		resource(uri id):SUBCLASS(id){objrdf::for_each<PROPERTIES>(initialize(this));}
		~resource(){
			#ifdef OBJRDF_VERB
			cerr<<"delete "<<get_class()->id<<" `"<<SUBCLASS::id<<"' "<<this<<endl;
			#endif
			objrdf::for_each<PROPERTIES>(destroy(this));
		}
		template<typename U> U& _get_(){return *(U*)(void*)(resource::bin+get_offset<PROPERTIES,U>::VALUE);}
		template<typename U,int FOUND=help<PROPERTIES,U>::VALUE> struct _help_{typedef typename SUBCLASS::template _help_<U>::VALUE VALUE;};
		template<typename U> struct _help_<U,1>{typedef resource VALUE;};
		template<typename U> U& get(){return _help_<U>::VALUE::template _get_<U>();}
		static V v;
		base_resource::type_iterator begin(){return base_resource::type_iterator(this,v.begin());}
		base_resource::type_iterator end(){return base_resource::type_iterator(this,v.end());}  
		virtual rdfs::Class* get_Class() const{return get_class().get();};
		static shared_ptr<rdfs::Class> get_class();	
		static int get_offsetof(){return offsetof(resource,bin);}//ugly, should look into macro
		template<typename U,int FOUND=help<PROPERTIES,U>::VALUE> struct help_set{
			static void go(resource* r,U u){r->get<U>()=u;}
		};
		template<typename U> struct help_set<U,2>{
			static void go(resource* r,U u){r->get<p_array<U> >().back()=u;}
		};
		DEFAULT_SET;
	};
	template<typename S> struct get_generic_property;
	template<> struct get_generic_property<base_resource>{
		static V go(){return V();}//no property
	};

	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUPERCLASS,
		typename SUBCLASS	
	> V resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS>::v=get_generic_property<resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS> >::go();

	template<
		typename NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	struct resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>:SUBCLASS{
		typedef resource SELF;
		resource(){}
		resource(uri id):SUBCLASS(id){}
		static int get_offsetof(){return 0;}//ugly, should look into macro
		static shared_ptr<rdfs::Class> get_class();	
		virtual rdfs::Class* get_Class() const{return get_class().get();};
	};
	/*
	template<
		typename NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	//creates some weird problem, to be investigated
	> V resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>::v;//=get_generic_property<resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS> >::go();
	*/
	/*
 	*	we could add a field for provenance: who created the statement (not the resource)
 	*	we have to be careful with the memory footprint, it could be optional, 
 	*/ 
	template<typename RANGE> struct base_property{
		RANGE t;
		//should it be constant?
		generic_property::PROVENANCE p;
		base_property(RANGE t=RANGE()):t(t),p(0){}
		void in(istream& is){is>>t;}
		void out(ostream& os){os<<t;}
	};
	template<typename RANGE> class base_property<RANGE*>:public shared_ptr<RANGE>{
	public:
		//should it be constant?
		generic_property::PROVENANCE p;
		void set(const base_property& pr){*this=pr;}
		base_property():p(0){}
		base_property(const shared_ptr<RANGE>& s):shared_ptr<RANGE>(s),p(0){}
	};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename _RANGE_
	> class property:public base_property<_RANGE_>{
	public:
		typedef _RANGE_ RANGE;
		typedef property SELF;
		template<typename S> property(S s):base_property<RANGE>(s){}
		property(){}
		static shared_ptr<rdf::Property> get_property();
	};
	//schema
	typedef base_resource* (*fpt)(uri);
	template<typename T> base_resource* constructor(uri u){return new T(u);}
	template<typename T> fpt get_constructor(){return &constructor<T>;}
	struct type_p{
		shared_ptr<rdfs::Class> t;
		type_p(rdfs::Class* c):t(c){}
		bool operator()(shared_ptr<base_resource> r) const;
	};
}
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
 	*
 	*
 	*/
	char _RDF_[]="RDF";
	struct RDF:objrdf::resource<rdfs_namespace,_RDF_,objrdf::p_array<objrdf::pp>,RDF>{//document
		/*	it would be neat to have an iterator based on a sparql query but with
		 *	casting if all the types are the same, should the query be run first and 
		 *	the result stored in a temporary array?
		 */
		RDF(objrdf::uri id=objrdf::uri("rdf_doc"));
		~RDF();
		typedef map<objrdf::uri,shared_ptr<objrdf::base_resource> > MAP;
		typedef objrdf::p_array<objrdf::pp> V;
		MAP m;
		void insert(shared_ptr<objrdf::base_resource> r);
		shared_ptr<objrdf::base_resource> find(objrdf::uri s);
		void to_rdf_xml(ostream& os,const objrdf::generic_property::PROVENANCE& p=0);//the document should not have loops!!!
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
	PROPERTY(type,rdfs::Class*);
}
namespace rdfs{
	PROPERTY(domain,Class*);
	PROPERTY(range,Class*);
	PROPERTY(subClassOf,Class*);
	PROPERTY(comment,string);
	/*
 	* some properties whose domain is rdf:Resource but we don't want to make them member
 	* of objrdf::base_resource 
 	*/
	PROPERTY(isDefinedBy,objrdf::base_resource*);
	PROPERTY(label,string);
	PROPERTY(subPropertyOf,rdf::Property*);
}
namespace rdf{
	char _Literal[]="Literal";
	struct Literal:objrdf::resource<rdfs_namespace,_Literal,objrdf::NIL,Literal>{
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
	char _Double[]="double";typedef objrdf::resource<rdfs_namespace,_Double,objrdf::NIL,objrdf::NIL,rdf::Literal> Double;
	char _Float[]="float";typedef objrdf::resource<rdfs_namespace,_Float,objrdf::NIL,objrdf::NIL,rdf::Literal> Float;
	char _Int[]="integer";typedef objrdf::resource<rdfs_namespace,_Int,objrdf::NIL,objrdf::NIL,rdf::Literal> Int;
	char _Unsigned_Int[]="unsignedInt";typedef objrdf::resource<rdfs_namespace,_Unsigned_Int,objrdf::NIL,objrdf::NIL,rdf::Literal> Unsigned_int;
	char _Short[]="short";typedef objrdf::resource<rdfs_namespace,_Short,objrdf::NIL,objrdf::NIL,rdf::Literal> Short;
	char _Unsigned_Short[]="unsignedShort";typedef objrdf::resource<rdfs_namespace,_Unsigned_Short,objrdf::NIL,objrdf::NIL,rdf::Literal> Unsigned_short;
	char _String[]="string";typedef objrdf::resource<rdfs_namespace,_String,objrdf::NIL,objrdf::NIL,rdf::Literal> String;
}
namespace objrdf{
	char _Char[]="Char";typedef objrdf::resource<_rdfs_namespace,_Char,NIL,NIL,rdf::Literal> Char;
}
namespace objrdf{
	template<typename T> struct get_Literal:rdf::Literal{}; 
	//complex numbers could we define it only if complex is defined???
	//template<typename T> struct get_Literal<complex<T> >:get_Literal<T>{}; 
	template<> struct get_Literal<double>:xsd::Double{};
	template<> struct get_Literal<float>:xsd::Float{};
	template<> struct get_Literal<int>:xsd::Int{};
	template<> struct get_Literal<long int>:xsd::Int{};
	template<> struct get_Literal<unsigned int>:xsd::Unsigned_int{};
	template<> struct get_Literal<short>:xsd::Short{};
	template<> struct get_Literal<unsigned short>:xsd::Unsigned_short{};
	template<> struct get_Literal<char>:Char{};
	template<> struct get_Literal<string>:xsd::String{};
}
namespace rdfs{
	/*
		XMLLiteral should be (double-) quoted when using turtle notation
		but NOT when using XML serialization of SPARQL reply
	*/
	struct XMLLiteral{}; //symbolic type
	char _XML_Literal[]="XML_Literal";
	typedef objrdf::resource<rdfs_namespace,_XML_Literal,objrdf::NIL,rdf::Literal> XML_Literal;
	//JSON type
	struct JSON{};
	char _JSON[]="JSON";
	typedef objrdf::resource<rdfs_namespace,_JSON,objrdf::NIL,rdf::Literal> JSON_type;
}
namespace objrdf{
	template<typename NAMESPACE,const char* NAME> struct get_size<property<NAMESPACE,NAME,rdfs::XMLLiteral> >{enum{VALUE=0};};
	template<> struct get_Literal<rdfs::XMLLiteral>:rdfs::XML_Literal{};
}
namespace objrdf{
	_PROPERTY(c_index,short);//each class has a unique index useful for fast serialization/parsing
}
namespace rdfs{
	char _Class[]="Class";
	struct Class:objrdf::resource<rdfs_namespace,_Class,objrdf::tuple<subClassOf,comment,isDefinedBy,objrdf::c_index>::SELF,Class>{
		const objrdf::fpt f;
		static vector<shared_ptr<Class> >& get_instances();
		Class(objrdf::uri u);
		Class(objrdf::uri id,subClassOf s,objrdf::fpt f,string comment);
		bool operator==(Class& c) const;
		bool operator<(Class& c) const;
		bool operator<=(Class& c) const;
		bool literalp();
		COMMENT("The class of classes.");
	};
}//end namespace rdfs
namespace objrdf{
	_PROPERTY(p_index,short);//each property has a unique index useful for fast serialization/parsing
	_PROPERTY(p_self,rdf::Property*);
}
namespace rdf{
	char _Property[]="Property";
	struct Property:objrdf::resource<rdfs_namespace,_Property,objrdf::tuple<rdfs::domain,rdfs::range,rdfs::subPropertyOf,objrdf::p_index,objrdf::p_self>::SELF,Property>{
		Property(objrdf::uri u);
		Property(objrdf::uri u,rdfs::range r,const bool _literalp);
		static vector<shared_ptr<Property> >& get_instances();
		const bool literalp;
		static shared_ptr<Property> nil;
		COMMENT("The class of RDF properties.");
		objrdf::base_resource::instance_iterator get_self_iterator();
	};
}//end namespace rdf
namespace objrdf{
	template<typename A,typename B> struct equality{enum{VALUE=0};};
	template<typename A> struct equality<A,A>{enum{VALUE=1};};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	shared_ptr<rdfs::Class> resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS>::get_class(){
		typedef typename IfThenElse<equality<SUPERCLASS,NIL>::VALUE,resource,SUPERCLASS>::ResultT TMP;
		static shared_ptr<rdfs::Class> c(new rdfs::Class(objrdf::get_uri<NAMESPACE>(NAME),rdfs::subClassOf(SUBCLASS::get_class()),get_constructor<TMP>(),TMP::get_comment!=SUBCLASS::get_comment ? TMP::get_comment() : ""));
		return c;
	}
	//does it really need to be specialized?: yes!
	template<
		typename NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	shared_ptr<rdfs::Class> resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>::get_class(){
		typedef typename IfThenElse<equality<SUPERCLASS,NIL>::VALUE,resource,SUPERCLASS>::ResultT TMP;
		//the static variable c will exist until program exits
		static shared_ptr<rdfs::Class> c(new rdfs::Class(objrdf::get_uri<NAMESPACE>(NAME),rdfs::subClassOf(SUBCLASS::get_class()),get_constructor<TMP>(),TMP::get_comment!=SUBCLASS::get_comment ? TMP::get_comment() : ""));
		return c;
	}
	template<typename RANGE> struct selector{
		typedef get_Literal<RANGE> ResultT;
		enum{IS_LITERAL=1};
	};
	template<typename RANGE> struct selector<RANGE*>{
		typedef RANGE ResultT;
		enum{IS_LITERAL=0};
	};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename RANGE
	> shared_ptr<rdf::Property> property<NAMESPACE,NAME,RANGE>::get_property(){
		static shared_ptr<rdf::Property> c(new rdf::Property(objrdf::get_uri<NAMESPACE>(NAME),rdfs::range(selector<RANGE>::ResultT::get_class()),selector<RANGE>::IS_LITERAL));
		return c;
	}
	struct namep{
		uri n;
		namep(uri n);
		bool operator() (generic_property* p) const;
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		typename NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,RANGE> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void add_property(base_resource* subject,int index,PROVENANCE p){
			cerr<<subject->id<<" set provenance:"<<p<<endl;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p=p;	
		}
		virtual void in(base_resource* subject,istream& is,int index){
			PREDICATE tmp;
			tmp.in(is);
			tmp.p=get_provenance(subject,index);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		} 
		virtual void out(base_resource* subject,ostream& os,int){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().out(os);	
		}
		virtual int get_size(base_resource*){return 1;}
		virtual PROVENANCE get_provenance(base_resource* subject,int){
			//cerr<<"get_provenance:"<<static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p<<endl;	
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p;	
		}
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		typename NAMESPACE,
		const char* NAME
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,std::string> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void add_property(base_resource* subject,int index,PROVENANCE p){
			cerr<<subject->id<<" set provenance:"<<p<<endl;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p=p;	
		}
		//should not use this method
		virtual void in(base_resource* subject,istream& is,int index){
			PREDICATE tmp;
			tmp.in(is);
			tmp.p=get_provenance(subject,index);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		}
		virtual void out(base_resource* subject,ostream& os,int){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().out(os);	
		}
		virtual void set_string(base_resource* subject,string s){
			PREDICATE tmp(s);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().t.size()>0;	
		}
		virtual PROVENANCE get_provenance(base_resource* subject,int){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p;	
		}
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		typename NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,RANGE*> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),false){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void add_property(base_resource* subject,int index,PROVENANCE p){
			cerr<<subject->id<<" set provenance:"<<p<<endl;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p=p;	
		}
		virtual void set_object(base_resource* subject,shared_ptr<base_resource> object,int index){
			PREDICATE pr(static_pointer_cast<typename PREDICATE::element_type>(object));
			pr.p=get_provenance(subject,index);//awkward
			//PREDICATE::RANGE is a pointer!
			//check the type first
			if(object->is_a(RANGE::get_class()))
				//static_cast<SUBJECT*>(subject)->set(PREDICATE(static_pointer_cast<typename PREDICATE::element_type>(object)));
				static_cast<SUBJECT*>(subject)->set(pr);
			else
				cerr<<"wrong type"<<endl;
		}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>();
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().get()!=0;
		}
		virtual void erase(base_resource* subject,int first,int last){
			//cerr<<"erase:"<<subject<<"\t"<<first<<"\t"<<last<<endl;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>()=shared_ptr<typename PREDICATE::element_type>();
		}
		virtual PROVENANCE get_provenance(base_resource* subject,int){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p;	
		}
	};
	/*
	template<
		typename SUBJECT
	> class _property_<SUBJECT,rdfs::type,rdfs::type>:public generic_property{
	public:
		_property_():generic_property(rdfs::type::get_property(),false){}
		//_property_():generic_property(rdfs::type::get_property(),false){p->get<rdfs::domain>()=base_resource::get_class();}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int){
			//return SUBJECT::get_class();
			return static_cast<SUBJECT*>(subject)->get_class();
		}
		virtual int get_size(base_resource*){
			return 1;
		}
	};
	*/
	template<
		typename SUBJECT,
		typename NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,p_array<property<NAMESPACE,NAME,RANGE*> > >:public generic_property{
	public:
		typedef property<NAMESPACE,NAME,RANGE*> PREDICATE;
		typedef p_array<PREDICATE> PREDICATE_ARRAY;
		_property_():generic_property(PREDICATE::get_property(),false){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index];
		}
		virtual void set_object(base_resource* subject,shared_ptr<base_resource> object,int index){
			//provenance is lost
			PREDICATE pr(static_pointer_cast<typename PREDICATE::element_type>(object));
			pr.p=get_provenance(subject,index);//awkward
			//static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].set(PREDICATE(static_pointer_cast<typename PREDICATE::element_type>(object)));
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].set(pr);
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().size();
		}
		virtual void add_property(base_resource* subject,int index,PROVENANCE p){
			cerr<<subject->id<<" set provenance:"<<p<<endl;
			PREDICATE pr;
			pr.p=p;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().push_back(pr);
		}
		virtual void erase(base_resource* subject,int first,int last){
			PREDICATE_ARRAY& v=static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>();
			v.erase(v.begin()+first,v.begin()+last);
		}
		virtual PROVENANCE get_provenance(base_resource* subject,int index){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].p;
		}
	};
	template<
		typename SUBJECT,
		typename NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,p_array<property<NAMESPACE,NAME,RANGE > > >:public generic_property{
	public:
		typedef property<NAMESPACE,NAME,RANGE> PREDICATE;
		typedef p_array<PREDICATE> PREDICATE_ARRAY;
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void in(base_resource* subject,istream& is,int index){
			PREDICATE tmp;
			tmp.in(is);
			tmp.p=get_provenance(subject,index);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		} 
		virtual void out(base_resource* subject,ostream& os,int index){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].out(os);	
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().size();
		}
		virtual void add_property(base_resource* subject,int index,PROVENANCE p){
			cerr<<subject->id<<" set provenance:"<<p<<endl;
			PREDICATE pr;
			pr.p=p;
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().push_back(pr);
		}
		virtual void erase(base_resource* subject,int first,int last){
			PREDICATE_ARRAY& v=static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>();
			v.erase(v.begin()+first,v.begin()+last);
		}
		virtual PROVENANCE get_provenance(base_resource* subject,int index){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].p;
		}
	};
	/*
	template<
		typename SUBJECT,
		typename NAMESPACE,
		const char* NAME
	> class _property_<SUBJECT,property<NAMESPACE,NAME,rdfs::JSON>,property<NAMESPACE,NAME,rdfs::JSON> >:public generic_property{
	public:
		virtual shared_ptr<rdf::Property> get_Property(){return property<NAMESPACE,NAME,rdfs::JSON>::get_property();}
		virtual void out(base_resource* subject,void* p,ostream& os){
			static_cast<SUBJECT*>(subject)->to_json(os);
		}
		
	};
	*/
	template<
		typename SUBJECT,
		typename PREDICATE,
		typename NAMESPACE,
		const char* NAME
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,rdfs::XMLLiteral> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->p_to_xml_size(PREDICATE::get_property());
		}
		virtual void out(base_resource* subject,ostream& os,int index){
			static_cast<SUBJECT*>(subject)->p_to_xml(PREDICATE::get_property(),os);
		}
		virtual void in(base_resource* subject,istream& is,int index){ 
		}
		virtual PROVENANCE get_provenance(base_resource* subject,int){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().p;
		}
	};
	class pseudo_property:public generic_property{
		shared_ptr<rdfs::Class> object;
	public:
		pseudo_property(shared_ptr<rdfs::Class> object);//:generic_property(rdfs::type::get_property(),false){object=_object;}
		virtual int get_size(base_resource* subject);//{return 1;}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index);//{return object;}
		virtual void set_object(base_resource* subject,shared_ptr<base_resource> object,int index);
		virtual PROVENANCE get_provenance(base_resource* subject,int index){return 0;}
		/*
		virtual void in(base_resource*,istream& is,int); 
		virtual void out(base_resource*,ostream& os,int);
		*/
	};
	template<
		typename NAMESPACE,
		const char* NAME,
		typename _PROPERTIES_,
		typename SUPERCLASS,
		typename SUBCLASS 
	> struct get_generic_property<resource<NAMESPACE,NAME,_PROPERTIES_,SUPERCLASS,SUBCLASS> >{
		typedef typename _PROPERTIES_::SELF PROPERTIES;
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS> RESOURCE;
		typedef typename IfThenElse<equality<SUPERCLASS,NIL>::VALUE,RESOURCE,SUPERCLASS>::ResultT TMP;
		static V go(){
			V v=get_generic_property<typename SUBCLASS::SELF>::go();
			//problem there
			v.insert(v.begin(),new pseudo_property(RESOURCE::get_class()));
			//v.push_back(new _property_<RESOURCE,rdfs::type,rdfs::type>());
			//v.push_back(new _property_<RESOURCE,rdfs::type>());
			//v.push_back(rdf::type);
			return concat(v,objrdf::for_each<PROPERTIES>(_meta_<TMP>(TMP::get_offsetof())).v);
		}
	};
}
#endif

