#ifndef OBJRDF_H
#define OBJRDF_H
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <assert.h>
#include "shared_ptr.h"
#include "ifthenelse.hpp"
#include <algorithm>
#include <stdexcept>
using namespace std;
template<typename T> vector<T> concat(const vector<T>& a,const vector<T>& b){
	vector<T> v=a;
	for(typename vector<T>::const_iterator i=b.begin();i<b.end();++i) v.push_back(*i);
	return v;
}
#define PROPERTY(n,range) char _##n[]=#n;typedef objrdf::property<rdfs_namespace,_##n,range> n;
#define CLASS0(n) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n> n;
#define CLASS1(n,p0) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,p0> n;
#define CLASS2(n,p0,p1) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::duo<p0,p1> > n;
#define CLASS3(n,p0,p1,p2) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2> > n;
#define CLASS4(n,p0,p1,p2,p3) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3> > n;
#define CLASS5(n,p0,p1,p2,p3,p4) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3,p4> > n;
#define CLASS6(n,p0,p1,p2,p3,p4,p5) char _##n[]=#n;typedef objrdf::resource<rdfs_namespace,_##n,objrdf::tuple<p0,p1,p2,p3,p4,p5> > n;
#define SET(r) //backward compatibility

#define DEFAULT_SET template<typename U> void set(U u){help_set<U>::go(this,u);}//need to copy this line in any struct that specializes the function
/*
 * 	must be a public member of the Class!!!!!
 * 	multiple symbols will be defined
 * 	if html is used inside the comment the namespace must be present eg:
 * 		<p xmlns='http://www.w3.org/1999/xhtml'>....</p>
 */
#define COMMENT(str) static string get_comment(){return str;}
#define HTML_COMMENT(str) static string get_comment(){return string("<p xmlns='http://www.w3.org/1999/xhtml'>")+str+"</p>";}
#ifdef WIN32
#pragma warning(disable: 4503)
#endif
//fwd
namespace rdf{
	//multiple definitions!!!
	char rdfs_namespace[]="http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	struct Property;
}
namespace rdfs{
	char rdfs_namespace[]="http://www.w3.org/2000/01/rdf-schema#";
	struct Class;
}
namespace objrdf{
	char rdfs_namespace[]="";
	/*
		introduces basic namespace support
		there should be two different ways to create a resource depending whether it is local or not
		usually static resources are not local
		optimization for size:
		there should also be a way to set the alias
	*/
	struct uri{
		typedef std::pair<string,string> sp;	
		string name;
		uri(){}
		static vector<sp> v;//v[0] is empty	
		char index;
		uri(const string name){
			index=0;
			this->name=name;
		}
		uri(const string name,const string ns,const string alias=""){
			this->name=name;
			v.push_back(sp(ns,alias));
			index=v.size()-1;
		}
		uri& operator=(string s){
			name=s;
			return *this;
		}	
		bool empty() const{return name.empty();}
		bool operator==(const string n) const {
			return name==n;
		}
		//degenerate to string
		operator string(){return name;}
		friend ostream& operator<<(ostream& os,uri& u){
			return os<<u.name;
		}
	};
	/*
		attempt to make it more flexible, 
		the number of bits need to be converted to number of bytes 
		9->2
		8->1
		7->1
		..
		the problem is that we cannot access 
		somehow similar to std::bitset<N>
	*/
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
		static rdf::Property* get_property(){return P::get_property();}
	/*	
		virtual ~p_array(){
			cerr<<"deleting p_array"<<endl;
		}*/
	};
	template<typename T> struct help<p_array<T>,T>{enum{VALUE=2};};
	class base_resource;
	class generic_property{
	public:
		//rdf::Property* p;
		shared_ptr<rdf::Property> p;
		const bool literalp;
		virtual void set_string(base_resource*,string s);
		virtual void in(base_resource* subject,istream& is,int index);
		virtual void out(base_resource* subject,ostream& os,int index);
		virtual int get_size(base_resource* subject)=0;
		virtual void add_property(base_resource* subject,int index);//could have insert instead
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index);
		virtual void set_object(base_resource* subject,base_resource* object,int index);
		int offset;
		generic_property(rdf::Property* _p,const bool literalp);
		void print() const;
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
		struct iterator:V::iterator{//need a typedef!!
			typedef V::iterator BASE;
			base_resource* subject;
			iterator():subject(0){}
			iterator(base_resource* _subject,V::iterator _i):V::iterator(_i){
				subject=_subject;
			}
			//rdf::Property* get_Property() const;
			shared_ptr<rdf::Property> get_Property() const;
			int get_size() const;
			bool literalp() const;
			inline void* current() const {return (char*)subject+BASE::operator*()->offset;}
		};
	public:
		typedef base_resource SELF;
		struct instance_iterator:base_resource::iterator{
			int index;
			instance_iterator(){ //what can we do with this?, not much
				index=0;
			}
			instance_iterator(base_resource::iterator _i,int _index):base_resource::iterator(_i){
				index=_index;	
			}
			instance_iterator& operator++(){++index;return *this;}
			instance_iterator* operator->(){return this;}
			instance_iterator& operator*(){return *this;}
			//careful with signature!!!!!!!!!!!!!!!
			bool operator!=(const instance_iterator& j) const{return index!=j.index;}
			bool operator<(const instance_iterator& j) const{return index<j.index;}
			void in(istream& is);
			void out(ostream& os);
			shared_ptr<base_resource> get_object();
			void set_object(base_resource*);
			friend ostream& operator<<(ostream& os,instance_iterator& i){
				if(i->literalp()) 
					i->out(os);
				else{
					if(i->get_object()->id.empty())
						os<<i->get_object().get();//print pointer
					else
						os<<i->get_object()->id;
				}
				return os;
			}
			string str();
			void set_string(string s);
		};
		struct type_iterator:base_resource::iterator{
			type_iterator(base_resource* _subject,V::iterator _i):base_resource::iterator(_subject,_i){}
			type_iterator* operator->(){return this;}
			instance_iterator begin(){return instance_iterator(*this,0);}
			instance_iterator end(){return instance_iterator(*this,get_size());}
			instance_iterator add_property();
		};
		virtual type_iterator begin(){return type_iterator(this,v.begin());}
		virtual type_iterator end(){return type_iterator(this,v.end());}
		virtual void end_resource(){};//will be invoked when finished parsing the element
		//
		//more general : URI
		//string id;
		//intrusive reference counting pg167 `Modern C++ design' Alexandrescu 
		//problem: n is a pretty common variable name, should change or use setter/getter
		short n;
		base_resource():n(0){}
		//base_resource(uri _id,short _n=0):n(_n),id(_id){}
		base_resource(uri _id):n(0),id(_id){}
		virtual ~base_resource(){
			#ifdef VERBOSE
			cerr<<"delete resource `"<<id<<"'"<<endl;
			#endif
		}
		uri id;
		static V v;
		//would be nice to have get<rdfs::type>
		virtual rdfs::Class* get_Class(){return get_class();};
		static rdfs::Class* get_class();	
		void to_turtle(ostream& os);
		void to_xml(ostream& os);
		void to_rdf_xml(ostream& os);//the document should not have loops!!!
		//to use in bash
		void to_turtle_pretty(ostream& os);
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		static shared_ptr<base_resource> nil;
		template<typename U,int FOUND=0> struct _help_{
			//should never get here make sure compilation stops here
			typedef typename U::IT_IS_NOT_A_MEMBER VALUE;
		};
		COMMENT("The class resource, everything.");
		//collect all the classes
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
		const char* NAMESPACE,
		const char* NAME,
		typename _PROPERTIES_=NIL,//would be nice if we could use tuple<> instead of tuple<>::SELF
		typename SUPERCLASS=NIL,
		typename SUBCLASS=base_resource	
	>
	struct resource:SUBCLASS{
		typedef typename _PROPERTIES_::SELF PROPERTIES;
		struct initialize{
			resource* r;
			initialize(resource* _r):r(_r){}	
			template<typename P> void go(){new(&r->get<P>()) P;}//placement new
		};
		struct destroy{
			resource* r;
			destroy(resource* _r):r(_r){}	
			template<typename P> void go(){
				#ifdef VERBOSE
				cerr<<"delete property `"<<P::get_property()->id<<"'"<<endl;
				#endif
				r->get<P>().~P();
			}//placement delete
		};
		typedef resource SELF;
		char bin[get_size<PROPERTIES>::VALUE];
		resource(uri _id):SUBCLASS(_id){objrdf::for_each<PROPERTIES>(initialize(this));}
		resource(){objrdf::for_each<PROPERTIES>(initialize(this));}
		~resource(){
			#ifdef VERBOSE
			cerr<<"delete "<<get_class()->id<<" `"<<SUBCLASS::id<<"'"<<endl;
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
		virtual rdfs::Class* get_Class(){return get_class();};
		static rdfs::Class* get_class();	
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
		const char* NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUPERCLASS,
		typename SUBCLASS	
	> V resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS>::v=get_generic_property<resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS> >::go();

	template<
		const char* NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	struct resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>:SUBCLASS{
		typedef resource SELF;
		resource(){}
		resource(uri _id):SUBCLASS(_id){}
		static int get_offsetof(){return 0;}//ugly, should look into macro
		static rdfs::Class* get_class();	
		virtual rdfs::Class* get_Class(){return get_class();};
		//we still need a rdfs::type property
		//static V v;
		//base_resource::type_iterator begin(){return base_resource::type_iterator(this,v.begin());}
		//base_resource::type_iterator end(){return base_resource::type_iterator(this,v.end());}
	};
	/*
	template<
		const char* NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	//creates some weird problem, to be investigated
	> V resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>::v;//=get_generic_property<resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS> >::go();
	*/
	template<typename RANGE> struct base_property{
		RANGE t;
		base_property(RANGE _t=RANGE()):t(_t){}
		void in(istream& is){is>>t;}
		void out(ostream& os){os<<t;}
		//could we do something with generic_property?
		//what about copy operator?
		//base_property(const 
	};
	template<typename RANGE> class base_property<RANGE*>:public objrdf::shared_ptr<RANGE>{
	public:
		RANGE* get() const{return objrdf::shared_ptr<RANGE>::operator->();}
		void set(const base_property& p){*this=p;}
		base_property(){}
		base_property(RANGE* r):objrdf::shared_ptr<RANGE>(r){}
		base_property(const objrdf::shared_ptr<RANGE>& s):objrdf::shared_ptr<RANGE>(s){}
	};
	template<
		const char* NAMESPACE,
		const char* NAME,
		typename _RANGE_
	> class property:public base_property<_RANGE_>{
	public:
		typedef _RANGE_ RANGE;
		typedef property SELF;
		template<typename S> property(S s):base_property<RANGE>(s){}
		property(){}
		static rdf::Property* get_property();
	};
	//schema
	typedef base_resource* (*fpt)();
	template<typename T> base_resource* constructor(){return new T;}
	template<typename T> fpt get_constructor(){return &constructor<T>;}
	struct type_p{
		shared_ptr<rdfs::Class> t;
		type_p(rdfs::Class* c):t(c){}
		bool operator()(shared_ptr<base_resource> r) const;
	};
}
namespace rdf{
	struct RDF{//document
		/*	it would be neat to have an iterator based on a sparql query but with
		 *	casting if all the types are the same, should the query be run first and 
		 *	the result stored in a temporary array
		 *
		 */
		typedef map<string,objrdf::base_resource*> MAP;
		typedef vector<objrdf::shared_ptr<objrdf::base_resource> > V;
		MAP m;
		V v;
		RDF();
		void introspect();
		void insert(objrdf::shared_ptr<objrdf::base_resource> r);
		objrdf::base_resource* find(string s);
		void to_rdf_xml(ostream& os);
		void to_rdf_xml_pretty(ostream& os);//the document should not have loops!!!
		void to_turtle(ostream& os);
		void to_turtle_pretty(ostream& os);
		objrdf::shared_ptr<objrdf::base_resource> query(string _i);
		template<typename T> objrdf::shared_ptr<T> query_t(string id){
			objrdf::shared_ptr<objrdf::base_resource> tmp=query(id);
			return (tmp.operator->()&&(*T::get_class()<=*tmp->get_Class()))? static_cast<T*>(tmp.operator->()) : 0;
		}
		template<typename T> objrdf::shared_ptr<T> query_by_type(){
			V::iterator i=find_if(v.begin(),v.end(),objrdf::type_p(T::get_class()));
			return (i!=v.end()) ? objrdf::shared_ptr<T>(static_cast<T*>(i->get())): 0;
		}
		template<typename T> vector<objrdf::shared_ptr<T> > query_by_type_array(){
			vector<objrdf::shared_ptr<T> > r;
			for(V::iterator i=v.begin();i<v.end();++i){
				if((*i)->get_Class()==T::get_class())
					r.push_back(*i);
			}
			return r;
		}
	};
}
namespace rdfs{
	PROPERTY(domain,Class*);
	PROPERTY(range,Class*);
	PROPERTY(subClassOf,Class*);
	PROPERTY(type,Class*);
	PROPERTY(comment,string);
}
namespace rdf{
	char _Literal[]="Literal";
	struct Literal:objrdf::resource<rdfs_namespace,_Literal,objrdf::NIL,Literal>{
		COMMENT("The class of literal values, eg. textual strings an integers")
	};
}
namespace objrdf{//for now
	char _Double[]="Double";typedef objrdf::resource<rdfs_namespace,_Double,NIL,NIL,rdf::Literal> Double;
	char _Float[]="Float";typedef objrdf::resource<rdfs_namespace,_Float,NIL,NIL,rdf::Literal> Float;
	char _Int[]="Int";typedef objrdf::resource<rdfs_namespace,_Int,NIL,NIL,rdf::Literal> Int;
	char _Unsigned_Int[]="Unsigned_int";typedef objrdf::resource<rdfs_namespace,_Unsigned_Int,NIL,NIL,rdf::Literal> Unsigned_int;
	char _Short[]="Short";typedef objrdf::resource<rdfs_namespace,_Short,NIL,NIL,rdf::Literal> Short;
	char _Unsigned_Short[]="Unsigned_short";typedef objrdf::resource<rdfs_namespace,_Unsigned_Short,NIL,NIL,rdf::Literal> Unsigned_short;
	char _Char[]="Char";typedef objrdf::resource<rdfs_namespace,_Char,NIL,NIL,rdf::Literal> Char;
	char _String[]="String";typedef objrdf::resource<rdfs_namespace,_String,NIL,NIL,rdf::Literal> String;
}
namespace objrdf{
	template<typename T> struct get_Literal:rdf::Literal{}; 
	//complex numbers could we define it only if complex is defined???
	//template<typename T> struct get_Literal<complex<T> >:get_Literal<T>{}; 
	template<> struct get_Literal<double>:Double{};
	template<> struct get_Literal<float>:Float{};
	template<> struct get_Literal<int>:Int{};
	template<> struct get_Literal<long int>:Int{};
	template<> struct get_Literal<unsigned int>:Unsigned_int{};
	template<> struct get_Literal<short>:Short{};
	template<> struct get_Literal<unsigned short>:Unsigned_short{};
	template<> struct get_Literal<char>:Char{};
	template<> struct get_Literal<string>:String{};
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
	template<const char* NAMESPACE,const char* NAME> struct get_size<property<NAMESPACE,NAME,rdfs::XMLLiteral> >{enum{VALUE=0};};
	template<> struct get_Literal<rdfs::XMLLiteral>:rdfs::XML_Literal{};
}
namespace rdfs{
	PROPERTY(c_index,short);//each class has a unique index useful for fast serialization/parsing
	char _Class[]="Class";
	struct Class:objrdf::resource<rdfs_namespace,_Class,objrdf::duo<subClassOf,objrdf::duo<comment,c_index > >,Class>{
		objrdf::fpt f;
		//static short counter;
		static vector<Class*>& get_instances();
		Class();
		Class(string id);
		Class(string id,subClassOf s,objrdf::fpt f,string comment);
		bool operator==(Class& c) const;
		bool operator<(Class& c) const;
		bool operator<=(Class& c) const;
		bool literalp();
		COMMENT("The class of classes.");
	};
}//end namespace rdfs
namespace rdf{
	PROPERTY(p_index,short);//each property has a unique index useful for fast serialization/parsing
	char _Property[]="Property";
	struct Property:objrdf::resource<rdfs_namespace,_Property,objrdf::duo<rdfs::domain,objrdf::duo<rdfs::range,p_index> >,Property>{
		Property();
		Property(string id,rdfs::range r,const bool _literalp);
		static vector<Property*>& get_instances();
		const bool literalp;
	};
}//end namespace rdf
namespace objrdf{
	template<typename A,typename B> struct equality{enum{VALUE=0};};
	template<typename A> struct equality<A,A>{enum{VALUE=1};};
	template<
		const char* NAMESPACE,
		const char* NAME,
		typename PROPERTIES,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	rdfs::Class* resource<NAMESPACE,NAME,PROPERTIES,SUPERCLASS,SUBCLASS>::get_class(){
		typedef typename IfThenElse<equality<SUPERCLASS,NIL>::VALUE,resource,SUPERCLASS>::ResultT TMP;
		static rdfs::Class* c=new rdfs::Class(NAME,rdfs::subClassOf(SUBCLASS::get_class()),get_constructor<TMP>(),TMP::get_comment!=SUBCLASS::get_comment ? TMP::get_comment() : "");
		return c;
	}
	template<
		const char* NAMESPACE,
		const char* NAME,
		typename SUPERCLASS,
		typename SUBCLASS
	>
	rdfs::Class* resource<NAMESPACE,NAME,NIL,SUPERCLASS,SUBCLASS>::get_class(){
		typedef typename IfThenElse<equality<SUPERCLASS,NIL>::VALUE,resource,SUPERCLASS>::ResultT TMP;
		static rdfs::Class* c=new rdfs::Class(NAME,rdfs::subClassOf(SUBCLASS::get_class()),get_constructor<TMP>(),TMP::get_comment!=SUBCLASS::get_comment ? TMP::get_comment() : "");
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
		const char* NAMESPACE,
		const char* NAME,
		typename RANGE
	> rdf::Property* property<NAMESPACE,NAME,RANGE>::get_property(){
		static rdf::Property* c=new rdf::Property(NAME,rdfs::range(selector<RANGE>::ResultT::get_class()),selector<RANGE>::IS_LITERAL);
		return c;
	}
	struct namep{
		string n;
		namep(string _n);
		bool operator() (generic_property* p) const;
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		const char* NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,RANGE> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void in(base_resource* subject,istream& is,int){
			PREDICATE tmp;
			tmp.in(is);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		} 
		virtual void out(base_resource* subject,ostream& os,int){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().out(os);	
		}
		virtual int get_size(base_resource*){return 1;}
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		const char* NAMESPACE,
		const char* NAME
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,std::string> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		//should not use this method
		virtual void in(base_resource* subject,istream& is,int){
			PREDICATE tmp;
			tmp.in(is);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		}
		virtual void out(base_resource* subject,ostream& os,int){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE>().out(os);	
		}
		virtual void set_string(base_resource* subject,string s){
			PREDICATE tmp(s);
			static_cast<SUBJECT*>(subject)->set(tmp);	
		}
		virtual int get_size(base_resource*){return 1;}
	};
	template<
		typename SUBJECT,
		typename PREDICATE,
		const char* NAMESPACE,
		const char* NAME,
		typename RANGE
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,RANGE*> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),false){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual void set_object(base_resource* subject,base_resource* object,int index){
			static_cast<SUBJECT*>(subject)->set(PREDICATE(static_cast<typename PREDICATE::RANGE>(object)));
		}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>();
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE>().get()!=0;
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
		const char* NAMESPACE,
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
		virtual void set_object(base_resource* subject,base_resource* object,int index){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].set(PREDICATE(static_cast<typename PREDICATE::RANGE>(object)));
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().size();
		}
		virtual void add_property(base_resource* subject,int index){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().push_back(PREDICATE());
		}
	};
	template<
		typename SUBJECT,
		const char* NAMESPACE,
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
			static_cast<SUBJECT*>(subject)->set(tmp);	
		} 
		virtual void out(base_resource* subject,ostream& os,int index){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>()[index].out(os);	
		}
		virtual int get_size(base_resource* subject){
			return static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().size();
		}
		virtual void add_property(base_resource* subject,int index){
			static_cast<SUBJECT*>(subject)->template get<PREDICATE_ARRAY>().push_back(PREDICATE());
		}
	};
	/*
	template<
		typename SUBJECT,
		const char* NAMESPACE,
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
		const char* NAMESPACE,
		const char* NAME
	> class _property_<SUBJECT,PREDICATE,property<NAMESPACE,NAME,rdfs::XMLLiteral> >:public generic_property{
	public:
		_property_():generic_property(PREDICATE::get_property(),true){p->get<rdfs::domain>()=SUBJECT::get_class();}
		virtual int get_size(base_resource* subject){return 1;}
		virtual void out(base_resource* subject,ostream& os,int index){
			//static_cast<SUBJECT*>(subject)->template p_to_xml<PREDICATE>(os);
			static_cast<SUBJECT*>(subject)->p_to_xml(PREDICATE::get_property(),os);
		}
	};
	class pseudo_property:public generic_property{
		rdfs::Class* object;
	public:
		pseudo_property(rdfs::Class* _object);//:generic_property(rdfs::type::get_property(),false){object=_object;}
		virtual int get_size(base_resource* subject);//{return 1;}
		virtual shared_ptr<base_resource> get_object(base_resource* subject,int index);//{return object;}
		virtual void set_object(base_resource* subject,base_resource* object,int index);
		/*
		virtual void in(base_resource*,istream& is,int); 
		virtual void out(base_resource*,ostream& os,int);
		*/
	};
	template<
		const char* NAMESPACE,
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
			v.push_back(new pseudo_property(RESOURCE::get_class()));
			//v.push_back(new _property_<RESOURCE,rdfs::type,rdfs::type>());
			//v.push_back(new _property_<RESOURCE,rdfs::type>());
			//v.push_back(rdf::type);
			return concat(v,objrdf::for_each<PROPERTIES>(_meta_<TMP>(TMP::get_offsetof())).v);
		}
	};
}
#endif

