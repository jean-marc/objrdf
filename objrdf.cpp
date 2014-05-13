#include "objrdf.h"
#include <sstream>
#include <algorithm>
#include <fstream>
namespace objrdf{
	//color scheme in bash, could be customized by namespace
	char start_id[]="\033[32m";
	char stop_id[]="\033[m";
}
using namespace objrdf;

template<> void f_ptr::constructor<rdfs::Class>(RESOURCE_PTR p,uri u){assert(0);}
template<> void f_ptr::constructor<rdf::Property>(RESOURCE_PTR p,uri u){assert(0);}
template<> base_resource::type_iterator f_ptr::end<rdfs::Class>(RESOURCE_PTR r){return base_resource::type_iterator(r,rdfs::Class::v.begin());}
template<> base_resource::type_iterator f_ptr::end<rdf::Property>(RESOURCE_PTR r){return base_resource::type_iterator(r,rdf::Property::v.begin());}

name_p::name_p(uri n):n(n){}
bool name_p::operator()(const property_info& p) const{return p.p->id==n;}
bool type_p::operator()(RESOURCE_PTR r) const{return *t<=*r->get_Class();}
/*
base_resource::type_iterator base_resource::begin(){return base_resource::type_iterator(this,v.begin());}
base_resource::const_type_iterator base_resource::cbegin() const{return base_resource::const_type_iterator(this,v.cbegin());}
base_resource::type_iterator base_resource::end(){return base_resource::type_iterator(this,v.end());}
base_resource::const_type_iterator base_resource::cend() const{return base_resource::const_type_iterator(this,v.cend());}
*/
CONST_PROPERTY_PTR base_resource::type_iterator::get_Property() const{return static_cast<V::iterator>(*this)->p;}
CONST_PROPERTY_PTR base_resource::const_type_iterator::get_Property() const{return static_cast<V::const_iterator>(*this)->p;}
size_t base_resource::type_iterator::get_size() const{return std::get<6>(static_cast<V::iterator>(*this)->t)(subject);}//very confusing notation
size_t base_resource::const_type_iterator::get_size() const{return std::get<6>(static_cast<V::const_iterator>(*this)->t)(subject);}//very confusing notation
bool base_resource::type_iterator::literalp() const{return static_cast<V::iterator>(*this)->literalp;}
bool base_resource::const_type_iterator::literalp() const{return static_cast<V::const_iterator>(*this)->literalp;}
bool base_resource::type_iterator::constp() const{return !std::get<7>(static_cast<V::iterator>(*this)->t);}
bool base_resource::const_type_iterator::constp() const{return !std::get<7>(static_cast<V::const_iterator>(*this)->t);}

map<uri,RESOURCE_PTR>& base_resource::get_index(){
	static map<uri,RESOURCE_PTR> *m=new map<uri,RESOURCE_PTR>();
	return *m;
}
void base_resource::do_index(RESOURCE_PTR p){
	//LOG<<"indexing resource `"<<p->id<<"'"<<endl;
	//get_index()[p->id]=p;
}
property_info::property_info(CONST_PROPERTY_PTR p,function_table t):p(p),t(t),literalp(p->literalp){}
//should be deprecated
/*
void base_resource::erase(instance_iterator first,instance_iterator last){
	std::get<8>(first.i->t)(this,RESOURCE_PTR(),first.index,last.index);
}
void base_resource::erase(instance_iterator position){
	std::get<8>(position.i->t)(this,RESOURCE_PTR(),position.index,position.index+1);
}
*/
void base_resource::get_output(ostream& os) const{
	//what would be most appropriate HTTP message?	
}

base_resource::instance_iterator operator+(const base_resource::instance_iterator& a,const unsigned int& b){
	base_resource::instance_iterator tmp(a);
	return tmp+=b;
}
struct cmp_uri{
	uri u;
	cmp_uri(uri u):u(u){}
	bool operator()(CONST_CLASS_PTR s)const{return s->id==u;}
};
rdfs::Class::Class(objrdf::uri id,rdfs::subClassOf s,objrdf::base_resource::class_function_table t,string comment_str,objrdf::sizeOf size,objrdf::hashOf h):SELF(id),t(t){
	/*objrdf::Test_class::allocator a;
	auto ptr=a.allocate(1);
	a.construct(ptr,id);
	for(auto i=a.cbegin();i<a.cend();++i){
		//cout<<(*i).index<<endl;
		//crashes, does id become stale?, yes: the namespace handle!!!
		cerr<<"Test_Class:`"<<(*i)->id.local<<"'"<<endl;
		//we could use a hash to identify class
	}
	*/
	#ifdef OBJRDF_VERB
	cerr<<"create rdfs::Class `"<<id<<"'\t"<<this<</*"\t"<<t<<*/endl;
	#endif
	get<comment>().set_string(comment_str);
	get<objrdf::sizeOf>()=size;
	get<objrdf::hashOf>()=h;
	if(s){
		//what if we change allocators??
		get<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().push_back(s);
		get<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().insert(
			get<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().end(),
			s->get_const<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().cbegin(),
			s->get_const<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().cend()
		);
	}
}
bool rdfs::Class::operator==(const rdfs::Class& c) const{
	return this==&c;	
}
bool rdfs::Class::operator<(const rdfs::Class& c) const{//is c subClass of this?
	auto i=find(c.get_const<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().cbegin(),c.get_const<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().cend(),this);
	return i!=c.get_const<objrdf::array<subClassOf,volatile_allocator_unmanaged<subClassOf>>>().cend();
}
bool rdfs::Class::operator<=(const rdfs::Class& c) const{
	return (*this==c)||(*this<c);
}
bool rdfs::Class::is_subclass_of(const Class& c) const{
	return c<=*this;
}
bool rdfs::Class::literalp() const{
	return *rdf::Literal::get_class()<*this;
}
void rdfs::Class::analyze(){};
CONST_CLASS_PTR base_resource::get_class(){
	static CONST_CLASS_PTR p=rdfs::Class::allocator_type::construct_allocate_at(
		allocator_type::get_pool().index,
		objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),
		rdfs::subClassOf(),
		objrdf::base_resource::class_function_table(
			f_ptr::constructor<base_resource>,
			f_ptr::begin<base_resource>,
			f_ptr::end<base_resource>,
			f_ptr::cbegin<base_resource>,
			f_ptr::cend<base_resource>
			,f_ptr::copy_constructor<base_resource>
			,f_ptr::allocate<base_resource>
		)			
		,get_comment()
		,objrdf::sizeOf(sizeof(base_resource))
		,objrdf::hashOf(pool::get_hash<base_resource>())
	);
	return p;
}
rdf::Property::Property(objrdf::uri id):SELF(id),literalp(true){}
rdf::Property::Property(objrdf::uri id,rdfs::range r,const bool literalp):rdf::Property::SELF(id),literalp(literalp){
	get<rdfs::range>()=r;
}
rdf::Property::Property(objrdf::uri id,rdfs::range r,const bool literalp,rdfs::subPropertyOf s):rdf::Property::SELF(id),literalp(literalp){
	get<rdfs::range>()=r;
	get<rdfs::subPropertyOf>()=s;
}
/*
 *	problem: no garbage collection so a new resource `nil' is created at each run, creating duplicates 
 *	one solution is to not persist base_resources but we might need them as cheap symbols 
 *
 */

//CONST_RESOURCE_PTR base_resource::nil=CONST_RESOURCE_PTR::construct(uri("nil"));
//RESOURCE_PTR base_resource::nil=RESOURCE_PTR::construct(uri("nil"));
RESOURCE_PTR base_resource::nil=base_resource::allocator_type::construct_allocate(uri("nil"));
V base_resource::v=get_generic_property<base_resource>::go();
CONST_PROPERTY_PTR base_resource::instance_iterator::get_Property() const{return i->p;}
CONST_PROPERTY_PTR base_resource::const_instance_iterator::get_Property() const{return i->p;}

bool base_resource::instance_iterator::literalp() const{return i->literalp;}
bool base_resource::const_instance_iterator::literalp() const{return i->literalp;}
void base_resource::instance_iterator::in(istream& is){
	std::get<1>(i->t)(subject,is,index);
}
void base_resource::instance_iterator::out(ostream& os) const{
	std::get<2>(i->t)(subject,os,index);
}
void base_resource::const_instance_iterator::out(ostream& os) const{
	std::get<2>(i->t)(subject,os,index);
}
string base_resource::instance_iterator::str() const{
	ostringstream os;
	out(os);
	return os.str();
}
string base_resource::const_instance_iterator::str() const{
	ostringstream os;
	out(os);
	return os.str();
}
int base_resource::instance_iterator::compare(const base_resource::instance_iterator& j) const{
	if(literalp()&&j.literalp()){
		//for now only compare same literal properties, not all properties define a comparison operator, actually only range matters
		if((get_Property()==j.get_Property())&&std::get<10>(i->t)) return std::get<10>(i->t)(subject,index,j.subject,j.index);
		return str().compare(j.str());//not very efficient and not always correct, eg: time formatting 
	}
	if(!literalp()&&!j.literalp()) return get_const_object()->id.compare(j.get_const_object()->id);	
	return (literalp()&&!j.literalp())? -1 : 1; //some default rule
}
int base_resource::const_instance_iterator::compare(const base_resource::const_instance_iterator& j) const{
	if(literalp()&&j.literalp()){
		//for now only compare same literal properties, not all properties define a comparison operator
		if((get_Property()==j.get_Property())&&std::get<10>(i->t)) return std::get<10>(i->t)(subject,index,j.subject,j.index);
		return str().compare(j.str());//not very efficient and not always correct, eg: time formatting 
	}
	if(!literalp()&&!j.literalp()) return get_const_object()->id.compare(j.get_const_object()->id);	
	return (literalp()&&!j.literalp())? -1 : 1; //some default rule
}
void base_resource::instance_iterator::set_string(string s){
	std::get<0>(i->t)(subject,s,index);
}
RESOURCE_PTR base_resource::instance_iterator::get_object() const{
	return std::get<3>(i->t)(subject,index);
}
CONST_RESOURCE_PTR base_resource::instance_iterator::get_const_object() const{
	return std::get<4>(i->t)(subject,index);
}
CONST_RESOURCE_PTR base_resource::const_instance_iterator::get_const_object() const{
	assert(std::get<4>(i->t));
	return std::get<4>(i->t)(subject,index);
}
void base_resource::instance_iterator::set_object(RESOURCE_PTR r){
	std::get<5>(i->t)(subject,r,index);
}
PROVENANCE base_resource::instance_iterator::get_provenance() const{
	return std::get<9>(i->t)(subject,index);
}
PROVENANCE base_resource::const_instance_iterator::get_provenance() const{
	return std::get<9>(i->t)(subject,index);
}
base_resource::instance_iterator base_resource::type_iterator::add_property(PROVENANCE p){
	/*
 	*	if the add_property is not defined we can just return an iterator to a constant property
 	*/
	#ifdef OBJRDF_VERB
	cerr<<"add_property `"<<get_Property()->id<<"' to resource `"<<subject->id<<"'"<<endl;
	#endif
	//awkward
	assert(std::get<7>(static_cast<V::iterator>(*this)->t));
	//if(!std::get<7>(static_cast<V::iterator>(*this)->t)) return begin(base_resource::nil)->begin();
	std::get<7>(static_cast<V::iterator>(*this)->t)(subject,p);
	return instance_iterator(subject,*this,get_size()-1);
}
void base_resource::_tmp_::operator=(const string& value){
	/*
  	base_resource::type_iterator i=std::find_if(r.begin(),r.end(),namep(key));
	if(i!=r.end()){
		if(i->literalp()){
			istringstream is(value);
			i->add_property()->in(is);
			if(!is) cerr<<"problem with formating\n";
		}else{
			cerr<<"only literal properties supported for now\n";
		}
	}else{
		cerr<<"property not found\n";
	}
	*/
}
int base_resource::p_to_xml_size(const CONST_PROPERTY_PTR p){return 1;}
bool base_resource::is_a(const CONST_CLASS_PTR& c) const{
	return *c<=*get_Class();
}
/*
void base_resource::to_turtle(ostream& os){
	os<<id<<" ";
	base_resource::type_iterator t_begin=begin();
	for(base_resource::type_iterator i=t_begin;i!=end();++i){
		base_resource::instance_iterator begin=i->begin(),end=i->end();
		if(begin!=end){ 
			if(i!=t_begin) os<<";";
			os<<i->get_Property()->id<<" "<<*begin;
			++begin;
			for(;begin<end;++begin)
				os<<","<<*begin;
		}
	}
	os<<" .\n";
}
void base_resource::to_turtle_pretty(ostream& os){
	os<<id<<" ";
	base_resource::type_iterator t_begin=begin();
	for(base_resource::type_iterator i=t_begin;i!=end();++i){
		base_resource::instance_iterator begin=i->begin(),end=i->end();
		if(begin!=end){ 
			if(i!=t_begin) os<<";";
			os<<i->get_Property()->id<<" "<<*begin;
			++begin;
			for(;begin<end;++begin)
				os<<","<<*begin;
		}
	}
	os<<" .\n";
}
void base_resource::to_rdf_xml(ostream& os,const PROVENANCE& p) const{
	os<<"\n<"<<get_Class()->id<<" "<<(id.is_local() ? rdf::ID : rdf::about)<<"='";
	id.to_uri(os);
	os<<"'>";
	for(auto i=cbegin();i!=cend();++i){
		for(auto j=i->cbegin();j!=i->cend();++j){
			if(j->get_provenance()>=p){
				if(i->literalp())
					os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
				else{
					os<<"\n\t<"<<i->get_Property()->id<<" "<<rdf::resource<<"='"<<(j->get_const_object()->id.is_local() ? "#" : "");
					j->get_const_object()->id.to_uri(os);
					os<<"'/>";
				}
			}
		}
	}
	os<<"\n</"<<get_Class()->id<<">";
}
*/
namespace objrdf{
	CONST_CLASS_PTR get_class(CONST_RESOURCE_PTR r){return CONST_CLASS_PTR(r.pool_ptr.index);}
	//shall we start with an offset since the first 2 properties are read-only (rdfs::type and objrdf::self)
	base_resource::type_iterator begin(RESOURCE_PTR r){return std::get<1>(get_class(r)->t)(r);}
	base_resource::type_iterator end(RESOURCE_PTR r){return std::get<2>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return std::get<3>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return std::get<4>(get_class(r)->t)(r);}
	//to start we could define profiles then a full blown access control
	base_resource::type_iterator begin(RESOURCE_PTR r,CONST_USER_PTR){return std::get<1>(get_class(r)->t)(r);}
	base_resource::type_iterator end(RESOURCE_PTR r,CONST_USER_PTR){return std::get<2>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r,CONST_USER_PTR){return std::get<3>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r,CONST_USER_PTR){return std::get<4>(get_class(r)->t)(r);}

	void erase(RESOURCE_PTR r,base_resource::instance_iterator first,base_resource::instance_iterator last){
		std::get<8>(first.i->t)(r,first.index,last.index);
	}
	void erase(RESOURCE_PTR r,base_resource::instance_iterator position){
		std::get<8>(position.i->t)(r,position.index,position.index+1);
	}
	base_resource::const_instance_iterator get_const_self_iterator(CONST_RESOURCE_PTR r){
		return base_resource::const_instance_iterator(r,base_resource::v.cbegin()+1,0);//hard-coded index, careful!
	}
	base_resource::instance_iterator get_self_iterator(RESOURCE_PTR r){
		return base_resource::instance_iterator(r,base_resource::v.begin()+1,0);//hard-coded index, careful!
	}
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os){
		os<<"\n<"<<get_class(r)->id<<" ";
		switch (r->id.index){
			case 0:os<<rdf::ID<<"='"<<r->id.local<<"'>";break;
			case 1:os<<rdf::nodeID<<"='"<<r->id.local<<"'>";break;
			default:os<<rdf::about<<"='";r->id.to_uri(os);os<<"'>";break;
		}
		for(auto i=cbegin(r);i!=cend(r);++i){
			if(i->get_Property()!=objrdf::self::get_property()){//let's skip objrdf::self
				for(base_resource::const_instance_iterator j=i->cbegin();j!=i->cend();++j){
					//should test if constant or not
					if(i->literalp())
						os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
					else{
						os<<"\n\t<"<<i->get_Property()->id<<" ";
						switch(j->get_const_object()->id.index){
							case 0:os<<rdf::resource<<"='#"<<j->get_const_object()->id.local<<"'/>";break;
							case 1:os<<rdf::nodeID<<"='"<<j->get_const_object()->id.local<<"'/>";break;
							default:os<<rdf::resource<<"='";j->get_const_object()->id.to_uri(os);os<<"'/>";break;
						}
					}
				}
			}
		}
		os<<"\n</"<<get_class(r)->id<<">";
	}
}
#ifdef JJJJJJJ
void base_resource::to_rdf_xml_pretty(ostream& os){
	/*
 	*	we can only use rdf:ID if the resource is local, that is if uri::index==0
 	*/ 
	os<<"\n\033[36m<"<<get_Class()->id<<" \033[32m"<<(id.is_local() ? rdf::ID : rdf::about)<<"=\033[31m'";
	id.to_uri(os);
#ifdef OBJRDF_VERB
	//os<<"' ref_count='"<<n;
#endif
	os<<"'\033[36m>";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			if(i->literalp())
				os<<"\n\t\033[36m<"<<i->get_Property()->id<</*"{"<<j->get_provenance()<<"}"<<*/">\033[m"<<*j<<"\033[36m</"<<i->get_Property()->id<<">";
			else{
				os<<"\n\t\033[36m<"<<i->get_Property()->id<</*"{"<<j->get_provenance()<<"}"<<*/" \033[32m"<<rdf::resource<<"=\033[31m'"<<(j->get_object()->id.is_local() ? "#" : "");
				j->get_object()->id.to_uri(os);
				os<<"'\033[36m/>";
			}
		}
	}
	os<<"\n\033[36m</"<<get_Class()->id<<">\033[m";
}
void base_resource::to_xml_leaf(ostream& os){
	os<<"<"<<get_Class()->id<<" "<<rdf::ID<<"='"<<id<<"'>";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		if(i->get_Property()!=rdf::type::get_property()){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					os<<"<"<<i->get_Property()->id<<">";
					if(i->literalp())
						j->out(os);	
					else
						j->get_object()->to_xml_leaf(os);//watch for loops!!!
					os<<"</"<<i->get_Property()->id<<">";
			}
		}
	}
	os<<"</"<<get_Class()->id<<">";
}
void base_resource::to_xml(ostream& os){
	os<<"<"<<get_Class()->id<<" ID='"<<id<<"'\n";
	uri::ns_declaration(os);
	os<<">";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		if(i->get_Property()!=rdf::type::get_property()){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					os<<"<"<<i->get_Property()->id<<">";
					if(i->literalp())
						j->out(os);	
					else
						j->get_object()->to_xml_leaf(os);//watch for loops!!!
					os<<"</"<<i->get_Property()->id<<">";
			}
		}
	}
	os<<"</"<<get_Class()->id<<">";
}
#endif
void objrdf::to_rdf_xml(ostream& os){
	os<<"<?xml version='1.0'?>\n";
	os<<"<"<<rdf::_RDF<<"\n";
	uri::ns_declaration(os);
	//we need xml:base declaration
	//should not be here!!!
	//os<<"xml:base='http://monitor.unicefuganda.org/'"<<endl;
	os<<">";
	rdfs::Class::allocator_type a;
	for(auto i=a.cbegin();i!=a.cend();++i){
		pool::POOL_PTR p(i.index); //there is a mapping between Class and pools
		for(auto j=pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);j!=pool::cend<base_resource::allocator_type::pointer::CELL>(p);++j){
			to_rdf_xml(j,cout);
		}
	}
	os<<"\n</"<<rdf::_RDF<<">\n";
}
//dumb scanner
RESOURCE_PTR objrdf::find(uri u){
	cerr<<"looking up uri `"<<u<<"'...";
	auto i=base_resource::get_index().find(u);
	if(i==base_resource::get_index().end()){
		cerr<<"not found"<<endl;
		return RESOURCE_PTR(0,0);
	}
	cerr<<"found"<<endl;
	return i->second;
	//return i==base_resource::_index_.end() ? RESOURCE_PTR() : i->second;
	/*	
	for(auto i=objrdf::begin();i<objrdf::end();++i){
		for(auto j=i.begin();j<i.end();++j){
			if((*j)->id==u){
				cerr<<"found"<<endl;	
				return *j;
			}
		}
	}	
	cerr<<"not found"<<endl;	
	return RESOURCE_PTR();
	*/
}
RESOURCE_PTR objrdf::create_by_type(CONST_CLASS_PTR c,uri id){
	//POOL_PTR p(c.index);
	/*
	* we have to make sure it points to a valid pool
	* is there anyway we could construct the pool on the fly?
	* 	we only need to know the store:
	*	template<
	*		typename T,
	*		typename STORE=free_store,	//free store or persistent, could also have no store
	*		bool POLYMORPHISM=false,	//does not support derived types
	*		typename _INDEX_=uint16_t	//can address 2^16 objects
	*	> struct pseudo_ptr
	* currently the Class only contains reference to the constructor, no information about allocator
	*/
	//can get rid of that now
	//assert(p->type_id);
	//get a generic pointer
	//should not allocate if constructor not defined???
	//constructor always defined, just does not do anything sometime
	//RESOURCE_PTR rp(p->allocate(),p);
	RESOURCE_PTR rp(std::get<6>(c->t)());
	//invoke constructor
	std::get<0>(c->t)(rp,id);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type(uri type,uri id){
	CONST_CLASS_PTR c=find_t<rdfs::Class>(type);
	return c ? objrdf::create_by_type(c,id) : RESOURCE_PTR(0,0);
}
RESOURCE_PTR objrdf::create_by_type_blank(CONST_CLASS_PTR c){
	//we need to use the object's allocator instead of the pointer...
	//the problem is that we don't know how to access it, we would have to store it in function pointer
	//LOG<<"creating blank instance of `"<<c->id<<"'"<<endl;
	//POOL_PTR p(c.index);
	//RESOURCE_PTR rp(p->allocate(),p);
	RESOURCE_PTR rp(std::get<6>(c->t)());
	ostringstream os;
	rp._print(os);
	uri u(os.str());
	u.index=1;
	std::get<0>(c->t)(rp,u);
	return rp;
}
RESOURCE_PTR objrdf::clone(CONST_RESOURCE_PTR r){
	LOG<<"cloning resource `"<<r->id<<"'"<<endl;
	CONST_CLASS_PTR c=get_class(r);
	//POOL_PTR p(c.index);
	//RESOURCE_PTR rp(p->allocate(),p);
	RESOURCE_PTR rp(std::get<6>(c->t)());
	std::get<5>(c->t)(rp,r);
	return rp;
}
//RESOURCE_PTR objrdf::clone_and_swap(CONST_RESOURCE_PTR r){
RESOURCE_PTR objrdf::clone_and_swap(RESOURCE_PTR r){
	LOG<<"cloning resource `"<<r->id<<"'"<<endl;
	CONST_CLASS_PTR c=get_class(r);
	RESOURCE_PTR rp(std::get<6>(c->t)());
	memcpy(rp,r,c->cget<sizeOf>().t);//this could go very wrong if we don't have the right size
	//void (*)(void*,CONST_RESOURCE_PTR) //copy constructor
	//template<typename T> void copy_constructor(void* p,CONST_RESOURCE_PTR r){new(p)T(static_cast<const T&>(*r));}
	std::get<5>(c->t)(r,rp);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type_blank(uri type){
	CONST_CLASS_PTR c=find_t<rdfs::Class>(type);
	return c ? objrdf::create_by_type_blank(c) : RESOURCE_PTR(0,0);
}

/*
int main(){
rdfs::Class::get_class()->to_turtle(cout);
}*/
