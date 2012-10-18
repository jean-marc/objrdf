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

template<> void f_ptr::constructor<rdfs::Class>(void* p,uri u){assert(0);}
template<> void f_ptr::constructor<rdf::Property>(void* p,uri u){assert(0);}
template<> base_resource::type_iterator f_ptr::end<rdfs::Class>(ITERATOR_RESOURCE_PTR r){return base_resource::type_iterator(r,rdfs::Class::v.begin());}
template<> base_resource::type_iterator f_ptr::end<rdf::Property>(ITERATOR_RESOURCE_PTR r){return base_resource::type_iterator(r,rdf::Property::v.begin());}

name_p::name_p(uri n):n(n){}
bool name_p::operator()(const property_info& p) const{return p.p->id==n;}
bool type_p::operator()(RESOURCE_PTR r) const{return *t<=*r->get_Class();}

base_resource::type_iterator base_resource::begin(){return base_resource::type_iterator(this,v.begin());}
base_resource::type_iterator base_resource::end(){return base_resource::type_iterator(this,v.end());}
PROPERTY_PTR base_resource::type_iterator::get_Property() const{return static_cast<V::iterator>(*this)->p;}
size_t base_resource::type_iterator::get_size() const{return std::get<6>(static_cast<V::iterator>(*this)->t)(subject);}//very confusing notation
bool base_resource::type_iterator::literalp() const{return static_cast<V::iterator>(*this)->literalp;}

base_resource::const_type_iterator base_resource::cbegin() const{return base_resource::const_type_iterator(this,v.cbegin());}
base_resource::const_type_iterator base_resource::cend() const{return base_resource::const_type_iterator(this,v.cend());}
PROPERTY_PTR base_resource::const_type_iterator::get_Property() const{return static_cast<V::const_iterator>(*this)->p;}
size_t base_resource::const_type_iterator::get_size() const{return std::get<6>(static_cast<V::const_iterator>(*this)->t)(subject);}//very confusing notation
bool base_resource::const_type_iterator::literalp() const{return static_cast<V::const_iterator>(*this)->literalp;}

property_info::property_info(PROPERTY_PTR p,function_table t):p(p),t(t),literalp(p->literalp){}
void base_resource::erase(instance_iterator first,instance_iterator last){
	std::get<8>(first.i->t)(this,first.index,last.index);
}
void base_resource::erase(instance_iterator position){
	std::get<8>(position.i->t)(this,position.index,position.index+1);
}
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
	bool operator()(CLASS_PTR s)const{return s->id==u;}
};
rdfs::Class::Class(objrdf::uri id,rdfs::subClassOf s,objrdf::base_resource::class_function_table t,string comment_str):SELF(id),t(t){
	#ifdef OBJRDF_VERB
	cerr<<"create rdfs::Class `"<<id<<"'\t"<<this<</*"\t"<<t<<*/endl;
	#endif
	get<comment>().t=comment_str;
	if(s){
		get<array<subClassOf>>().push_back(s);
		get<array<subClassOf>>().insert(
			get<array<subClassOf>>().end(),
			s->get_const<array<subClassOf>>().cbegin(),
			s->get_const<array<subClassOf>>().cend()
		);
	}
	n=1;//otherwise crash when deleting
}
bool rdfs::Class::operator==(const rdfs::Class& c) const{
	return this==&c;	
}
bool rdfs::Class::operator<(const rdfs::Class& c) const{//is c subClass of this?
	auto i=find(c.get_const<array<subClassOf>>().cbegin(),c.get_const<array<subClassOf>>().cend(),this);
	return i!=c.get_const<array<subClassOf>>().cend();
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
CLASS_PTR base_resource::get_class(){
	static CLASS_PTR p=CLASS_PTR::construct_at(
		POOL_PTR::help<base_resource>().index,
		objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),
		rdfs::subClassOf(),
		objrdf::base_resource::class_function_table(
			f_ptr::constructor<base_resource>,
			f_ptr::begin<base_resource>,
			f_ptr::end<base_resource>,
			f_ptr::cbegin<base_resource>,
			f_ptr::cend<base_resource>
		)			
		,get_comment()
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

CONST_RESOURCE_PTR base_resource::nil=CONST_RESOURCE_PTR::construct(uri("nil"));
V base_resource::v=get_generic_property<base_resource>::go();
PROPERTY_PTR base_resource::instance_iterator::get_Property() const{return i->p;}
PROPERTY_PTR base_resource::const_instance_iterator::get_Property() const{return i->p;}

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
string base_resource::instance_iterator::str(){
	ostringstream os;
	out(os);
	return os.str();
}
string base_resource::const_instance_iterator::str(){
	ostringstream os;
	out(os);
	return os.str();
}
void base_resource::instance_iterator::set_string(string s){
	std::get<0>(i->t)(subject,s,index);
}
RESOURCE_PTR base_resource::instance_iterator::get_object() const{
	return std::get<3>(i->t)(subject,index);
}
CONST_RESOURCE_PTR base_resource::instance_iterator::get_const_object() const{
	return std::get<4>(i->t)(subject,CONST_RESOURCE_PTR(),index);
}
CONST_RESOURCE_PTR base_resource::const_instance_iterator::get_const_object() const{
	assert(std::get<4>(i->t));
	return std::get<4>(i->t)(subject,alt_subject,index);
}
void base_resource::instance_iterator::set_object(RESOURCE_PTR r){
	/*
 	*	if r=base_resource::nil it should be converted to RESOURCE_PTR(0)
 	*/ 
	//std::get<5>(i->t)(subject,r,index);
	std::get<5>(i->t)(subject,base_resource::nil==r ? RESOURCE_PTR(0) : r,index);
}
PROVENANCE base_resource::instance_iterator::get_provenance() const{
	return std::get<9>(i->t)(subject,index);
}
PROVENANCE base_resource::const_instance_iterator::get_provenance() const{
	return std::get<9>(i->t)(subject,index);
}
base_resource::instance_iterator base_resource::type_iterator::add_property(PROVENANCE p){
	#ifdef OBJRDF_VERB
	cerr<<"add_property with provenance "<<(int)p<<" to resource `"<<subject->id<<"'"<<endl;
	#endif
	//awkward
	assert(std::get<7>(static_cast<V::iterator>(*this)->t));
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
int base_resource::p_to_xml_size(const PROPERTY_PTR p){return 1;}
bool base_resource::is_a(const CLASS_PTR& c) const{
	return *c<=*get_Class();
}
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
namespace objrdf{
	CLASS_PTR get_class(CONST_RESOURCE_PTR r){return CLASS_PTR(r.pool_ptr.index);}
	base_resource::type_iterator begin(RESOURCE_PTR r){return std::get<1>(get_class(r)->t)(r);}
	base_resource::type_iterator end(RESOURCE_PTR r){return std::get<2>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return std::get<3>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return std::get<4>(get_class(r)->t)(r);}
	base_resource::const_instance_iterator get_const_self_iterator(CONST_RESOURCE_PTR r){
		return base_resource::const_instance_iterator(r,r,base_resource::v.cbegin()+2,0);
	}
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os){
		//os<<"\n<"<<get_class(r)->id<<" "<<(r->id.is_local() ? rdf::ID : rdf::about)<<"='";
		os<<"\n<"<<get_class(r)->id<<" ";
		switch (r->id.index){
			case 0:os<<rdf::ID;break;
			case 1:os<<rdf::nodeID;break;
			default:os<<rdf::about;break;
		}
		os<<"='";
		r->id.to_uri(os);
		os<<"'>";
		for(auto i=cbegin(r);i!=cend(r);++i){
			for(base_resource::const_instance_iterator j=i->cbegin();j!=i->cend();++j){
				//should test if constant or not
				if(i->literalp())
					os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
				else{
					//os<<"\n\t<"<<i->get_Property()->id<<" "<<rdf::resource<<"='"<<(j->get_const_object()->id.is_local() ? "#" : "");
					os<<"\n\t<"<<i->get_Property()->id<<" ";//<<rdf::resource<<"='"<<(j->get_const_object()->id.is_local() ? "#" : "");
					switch(j->get_const_object()->id.index){
						case 0:os<<rdf::resource<<"='#";break;
						case 1:os<<rdf::nodeID<<"='";break;
						default:os<<rdf::resource<<"='";break;
					}
					j->get_const_object()->id.to_uri(os);
					os<<"'/>";
				}
			}
		}
		os<<"\n</"<<get_class(r)->id<<">";
	}
}
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
void objrdf::to_rdf_xml(ostream& os){
	os<<"<?xml version='1.0'?>\n";
	os<<"<"<<rdf::_RDF<<"\n";
	uri::ns_declaration(os);
	//we need xml:base declaration
	os<<"xml:base='http://inventory.unicefuganda.org/'"<<endl;
	os<<">";
	for(auto i=objrdf::begin();i<objrdf::end();++i){
		/*
		*	should throw if the class is not defined
		*/ 
		cerr<<"reading pool `"<<CLASS_PTR(i.index)->id<<"'"<<endl;
		assert(CLASS_PTR(i.index)->id.local[0]);
		for(auto j=i.begin();j<i.end();++j){
			to_rdf_xml(*j,os);
		}
		os<<endl;
	}
	os<<"\n</"<<rdf::_RDF<<">\n";
}
//dumb scanner
CONST_RESOURCE_PTR objrdf::find(uri u){
	cerr<<"looking up uri `"<<u<<"'...";
	for(auto i=objrdf::begin();i<objrdf::end();++i){
		for(auto j=i.begin();j<i.end();++j){
			if((*j)->id==u){
				cerr<<"found"<<endl;	
				return *j;
			}
		}
	}	
	cerr<<"not found"<<endl;	
	return CONST_RESOURCE_PTR();
}
RESOURCE_PTR objrdf::create_by_type(CLASS_PTR c,uri id){
	POOL_PTR p(c.index);
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
	assert(p->type_id);
	//get a generic pointer
	//should not allocate if constructor not defined???
	//constructor always defined, just does not do anything sometime
	RESOURCE_PTR rp(p->allocate(),p);
	//invoke constructor
	std::get<0>(c->t)(rp,id);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type(uri type,uri id){
	CLASS_PTR c=find_t<CLASS_PTR>(type);
	return c ? objrdf::create_by_type(c,id) : RESOURCE_PTR();
}
RESOURCE_PTR objrdf::create_by_type_blank(CLASS_PTR c){
	POOL_PTR p(c.index);
	RESOURCE_PTR rp(p->allocate(),p);
	ostringstream os;
	rp._print(os);
	uri u(os.str());
	u.index=1;
	std::get<0>(c->t)(rp,u);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type_blank(uri type){
	CLASS_PTR c=find_t<CLASS_PTR>(type);
	return c ? objrdf::create_by_type_blank(c) : RESOURCE_PTR();
}
pool_iterator objrdf::begin(){return pool_iterator(::begin<POOL_PTR>());}
pool_iterator objrdf::end(){return pool_iterator(::end<POOL_PTR>());}



/*
int main(){
rdfs::Class::get_class()->to_turtle(cout);
}*/
