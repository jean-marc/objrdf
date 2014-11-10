#include "objrdf.h"
#include <sstream>
#include <algorithm>
#include <fstream>
namespace objrdf{
	//color scheme in bash, could be customized by namespace
	char start_id[]="\033[32m";
	char stop_id[]="\033[m";
	ostream& operator<<(ostream& os,const property_info& i){
		return os<<"\n"<<i.p->id<<"\t("<<i.offset<<")\n******************\n"<<i.t;
	}
}
using namespace objrdf;
using namespace std;

//template<> void f_ptr::constructor<rdfs::Class>(RESOURCE_PTR p,uri u){assert(0);}//we might actually need that for sparql
template<> void f_ptr::constructor<rdf::Property>(RESOURCE_PTR p,uri u){assert(0);}
template<> base_resource::type_iterator f_ptr::end<rdfs::Class>(RESOURCE_PTR r){return base_resource::type_iterator(r,rdfs::Class::v.begin());}
template<> base_resource::type_iterator f_ptr::end<rdf::Property>(RESOURCE_PTR r){return base_resource::type_iterator(r,rdf::Property::v.begin());}

name_p::name_p(uri n):n(n){}

bool name_p::operator()(const property_info& p) const{return p.p->id==n;}
bool type_p::operator()(RESOURCE_PTR r) const{
	return is_a(get_class(r),t);
}
base_resource::base_resource(uri id):id(id){
	//note: not indexed by default, we couldn't anyway because we only have `this' pointer
	#ifdef OBJRDF_VERB
	cerr<<"create base_resource `"<<id<<"' "<<this<<endl;
	#endif
}
base_resource::~base_resource(){
	#ifdef OBJRDF_VERB
	cerr<<"delete base_resource `"<<id<<"' "<<this<<endl;
	#endif
}
CONST_PROPERTY_PTR base_resource::type_iterator::get_Property() const{return static_cast<V::const_iterator>(*this)->p;}//strange syntax
CONST_PROPERTY_PTR base_resource::const_type_iterator::get_Property() const{return static_cast<V::const_iterator>(*this)->p;}
//size_t base_resource::type_iterator::get_size() const{return static_cast<V::const_iterator>(*this)->t.get_size(subject);}
//size_t base_resource::const_type_iterator::get_size() const{return static_cast<V::const_iterator>(*this)->t.get_size(subject);}
size_t base_resource::type_iterator::get_size() const{
	auto tmp=*static_cast<V::const_iterator>(*this);
	assert(tmp.t.get_size_generic);
	return tmp.t.get_size_generic(subject,tmp.offset);
	//return static_cast<V::const_iterator>(*this)->t.get_size_generic(subject,(*this)->offset);
}
size_t base_resource::const_type_iterator::get_size() const{
	auto tmp=*static_cast<V::const_iterator>(*this);
	assert(tmp.t.get_size_generic);
	return tmp.t.get_size_generic(subject,tmp.offset);
	//return static_cast<V::const_iterator>(*this)->t.get_size_generic(subject,(*this)->offset);
}
bool base_resource::type_iterator::literalp() const{return static_cast<V::const_iterator>(*this)->literalp;}
bool base_resource::const_type_iterator::literalp() const{return static_cast<V::const_iterator>(*this)->literalp;}
bool base_resource::type_iterator::constp() const{return !static_cast<V::const_iterator>(*this)->t.add_property_generic;}
bool base_resource::const_type_iterator::constp() const{return !static_cast<V::const_iterator>(*this)->t.add_property_generic;}
void base_resource::patch(V& v){}

map<uri,RESOURCE_PTR>& base_resource::get_index(){
	static map<uri,RESOURCE_PTR> *m=new map<uri,RESOURCE_PTR>();
	return *m;
}
void base_resource::do_index(RESOURCE_PTR p){
	LOG<<"indexing resource `"<<p->id<<"'"<<endl;
	get_index()[p->id]=p;
}
property_info::property_info(CONST_PROPERTY_PTR p,function_table t,ptrdiff_t offset):p(p),t(t),literalp(p->literalp),offset(offset){}
void base_resource::get_output(ostream& os) const{
	os<<"HTTP/1.1 200 OK"<<"\r\n";
	ostringstream out;
	out<<"empty"<<endl;
	os<<"Content-Type: "<<"text/plain"<<"\r\n";
	os<<"Content-Length:"<<out.str().size()<<"\r\n";
	os<<"\r\n";
	os<<out.str();
	os.flush();
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
rdfs::Class::Class(objrdf::uri id):SELF(id){}
rdfs::Class::Class(
	objrdf::uri id,
	rdfs::subClassOf s,
	objrdf::base_resource::class_function_table t,
	string comment_str,
	objrdf::sizeOf size
#ifndef NATIVE
	,objrdf::hashOf h
#endif
):SELF(id),t(t){
	#ifdef OBJRDF_VERB
	cerr<<"create rdfs::Class `"<<id<<"'\t"<<this<</*"\t"<<t<<*/endl;
	#endif
	get<comment>().set_string(comment_str);
	get<objrdf::sizeOf>()=size;
	#ifdef NATIVE
	do_index(this);
	if(s.t){
	#else
	get<objrdf::hashOf>()=h;
	if(s){
	#endif
		get<array_subClassOf>().push_back(s);
		get<array_subClassOf>().insert(
			get<array_subClassOf>().end(),
			#ifdef NATIVE
			s.t->cget<array_subClassOf>().cbegin(),
			s.t->cget<array_subClassOf>().cend()
			#else
			s->cget<array_subClassOf>().cbegin(),
			s->cget<array_subClassOf>().cend()
			#endif
		);
	}
}
CONST_CLASS_PTR base_resource::get_class(){
	#ifdef NATIVE
	static CONST_CLASS_PTR p=new rdfs::Class(
	#else
	static CONST_CLASS_PTR p=rdfs::Class::allocator_type::construct_allocate_at(
		allocator_type::get_pool().index,
	#endif
		objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),
		rdfs::subClassOf(),
		objrdf::base_resource::class_function_table(
			f_ptr::constructor<base_resource>,
			f_ptr::destructor<base_resource>,
			f_ptr::copy_constructor<base_resource>,
			f_ptr::begin<base_resource>,
			f_ptr::end<base_resource>,
			f_ptr::cbegin<base_resource>,
			f_ptr::cend<base_resource>,
			f_ptr::allocate<base_resource>,
			f_ptr::deallocate<base_resource>,
			f_ptr::get_output<base_resource>
		)			
		,get_comment()
		,objrdf::sizeOf(sizeof(base_resource))
		#ifndef NATIVE
		,objrdf::hashOf(pool_allocator::pool::get_hash<base_resource>())
		#endif
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
#ifdef NATIVE
RESOURCE_PTR base_resource::nil=new base_resource(uri("nil"));
#else
RESOURCE_PTR base_resource::nil=base_resource::allocator_type::construct_allocate(uri("nil"));
#endif
V base_resource::v=get_generic_property<base_resource>::go();
CONST_PROPERTY_PTR base_resource::instance_iterator::get_Property() const{return i->p;}
CONST_PROPERTY_PTR base_resource::const_instance_iterator::get_Property() const{return i->p;}

bool base_resource::instance_iterator::literalp() const{return i->literalp;}
bool base_resource::const_instance_iterator::literalp() const{return i->literalp;}
void base_resource::instance_iterator::in(istream& is){
	//i->t.in(subject,is,index);
	i->t.in_generic(subject,i->offset,is,index);
}
void base_resource::instance_iterator::out(ostream& os) const{
	//i->t.out(subject,os,index);
	//cerr<<"out"<<i->t.out_generic<<endl;
	i->t.out_generic(subject,i->offset,os,index);
}
void base_resource::const_instance_iterator::out(ostream& os) const{
	//i->t.out(subject,os,index);
	//cerr<<"out"<<i->t.out_generic<<endl;
	i->t.out_generic(subject,i->offset,os,index);
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
		if((get_Property()==j.get_Property())&&i->t.compare) return i->t.compare(subject,index,j.subject,j.index);
		return str().compare(j.str());//not very efficient and not always correct, eg: time formatting 
	}
	if(!literalp()&&!j.literalp()) return get_const_object()->id.compare(j.get_const_object()->id);	
	return (literalp()&&!j.literalp())? -1 : 1; //some default rule
}
int base_resource::const_instance_iterator::compare(const base_resource::const_instance_iterator& j) const{
	if(literalp()&&j.literalp()){
		//for now only compare same literal properties, not all properties define a comparison operator
		if((get_Property()==j.get_Property())&&i->t.compare) return i->t.compare(subject,index,j.subject,j.index);
		return str().compare(j.str());//not very efficient and not always correct, eg: time formatting 
	}
	if(!literalp()&&!j.literalp()) return get_const_object()->id.compare(j.get_const_object()->id);	
	return (literalp()&&!j.literalp())? -1 : 1; //some default rule
}
void base_resource::instance_iterator::set_string(string s){
	i->t.set_string_generic(subject,s,i->offset,index);
}
RESOURCE_PTR base_resource::instance_iterator::get_object() const{
	//return i->t.get_object(subject,index);
	//assert(i->t.get_object_generic);
	return i->t.get_object_generic(subject,i->offset,index);
}
CONST_RESOURCE_PTR base_resource::instance_iterator::get_const_object() const{
	//return i->t.cget_object(subject,index);
	//assert(i->t.cget_object_generic);
	return i->t.cget_object_generic(subject,i->offset,index);
}
CONST_RESOURCE_PTR base_resource::const_instance_iterator::get_const_object() const{
	//assert(i->t.cget_object);
	//return i->t.cget_object(subject,index);
	//assert(i->t.cget_object_generic);
	return i->t.cget_object_generic(subject,i->offset,index);
}
void base_resource::instance_iterator::set_object(RESOURCE_PTR r){
	//i->t.set_object(subject,r,index);
	i->t.set_object_generic(subject,r,i->offset,index);
}
RESOURCE_PTR base_resource::instance_iterator::get_statement() const{
	return i->t.get_statement(subject,index);
}
RESOURCE_PTR base_resource::const_instance_iterator::get_statement() const{
	return i->t.get_statement(subject,index);
}
PROVENANCE base_resource::instance_iterator::get_provenance() const{
	return i->t.get_provenance(subject,index);
}
PROVENANCE base_resource::const_instance_iterator::get_provenance() const{
	return i->t.get_provenance(subject,index);
}
base_resource::instance_iterator base_resource::type_iterator::add_property(PROVENANCE p){
	/*
 	*	if the add_property is not defined we can just return an iterator to a constant property
 	*/
	#ifdef OBJRDF_VERB
	cerr<<"add_property `"<<get_Property()->id<<"' to resource `"<<subject->id<<"'"<<endl;
	#endif
	//awkward
	auto tmp=*static_cast<V::const_iterator>(*this);
	assert(tmp.t.add_property_generic);
	tmp.t.add_property_generic(subject,tmp.offset);
	return instance_iterator(subject,*this,get_size()-1);
}
int base_resource::p_to_xml_size(const CONST_PROPERTY_PTR p){return 1;}
namespace objrdf{
	CONST_CLASS_PTR get_class(CONST_RESOURCE_PTR r){
		#ifdef NATIVE
		return r->_get_class();
		#else
		CONST_CLASS_PTR p(r.pool_ptr.index);
		try{
			auto r=p.operator->();
		}catch(std::out_of_range& e){
			cerr<<"rdfs::Class not defined yet!"<<endl;
			exit(1);
		}
		return p;
		#endif
	}
	bool is_subclass(CONST_CLASS_PTR a,CONST_CLASS_PTR b){
		//a is derived from b
		#ifdef NATIVE
		auto i=find_if(
			a->cget<rdfs::Class::array_subClassOf>().cbegin(),
			a->cget<rdfs::Class::array_subClassOf>().cend(),
			[&](const rdfs::subClassOf& p){return p.t==b;}
		);
		#else
		auto i=find(a->cget<rdfs::Class::array_subClassOf>().cbegin(),a->cget<rdfs::Class::array_subClassOf>().cend(),b);
		#endif
		return i!=a->cget<rdfs::Class::array_subClassOf>().cend();
	}
	bool is_a(CONST_CLASS_PTR a,CONST_CLASS_PTR b){
		return a==b||is_subclass(a,b);
	}
	//shall we start with an offset since the first 2 properties are read-only (rdfs::type and objrdf::self)
	base_resource::type_iterator begin(RESOURCE_PTR r){return get_class(r)->t.begin(r);}
	base_resource::type_iterator end(RESOURCE_PTR r){return get_class(r)->t.end(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return get_class(r)->t.cbegin(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return get_class(r)->t.cend(r);}
	//to start we could define profiles then a full blown access control
	#ifndef NATIVE
	base_resource::type_iterator begin(RESOURCE_PTR r,CONST_USER_PTR){return get_class(r)->t.begin(r);}
	base_resource::type_iterator end(RESOURCE_PTR r,CONST_USER_PTR){return get_class(r)->t.end(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r,CONST_USER_PTR){return get_class(r)->t.cbegin(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r,CONST_USER_PTR){return get_class(r)->t.cend(r);}
	#endif

	void erase(RESOURCE_PTR r,base_resource::instance_iterator first,base_resource::instance_iterator last){
		first.i->t.erase(r,first.index,last.index);
	}
	void erase(RESOURCE_PTR r,base_resource::instance_iterator position){
		position.i->t.erase(r,position.index,position.index+1);
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
			//if(i->get_Property()!=objrdf::self::get_property()){//let's skip objrdf::self, can we just increase index?
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
			//}
		}
		os<<"\n</"<<get_class(r)->id<<">";
	}
	void get_output(CONST_RESOURCE_PTR r,ostream& os){
		get_class(r)->t.get_output(r,os);
	}
}
void objrdf::to_rdf_xml(ostream& os){
	os<<"<?xml version='1.0'?>\n";
	os<<"<"<<rdf::_RDF<<"\n";
	uri::ns_declaration(os);
	//we need xml:base declaration
	os<<">";
	#ifdef NATIVE//use the index
#ifdef __GNUG__
	for(auto i:base_resource::get_index()) to_rdf_xml(i.second,os);
#else
	for(auto i=base_resource::get_index().cbegin();i!=base_resource::get_index().cend();++i) to_rdf_xml(i->second,os);
#endif
	#else
	rdfs::Class::allocator_type a;
	for(auto i=a.cbegin();i!=a.cend();++i){
		pool_allocator::pool::POOL_PTR p(i.get_cell_index()); //there is a mapping between Class and pools
		if(p->iterable){
			for(auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);++j){
				to_rdf_xml(j,cout);
			}
		}else{
			cerr<<"pool `"<<i->id<<"' not iterable "<<p->payload_offset<<endl;
		}
	}
	#endif
	os<<"\n</"<<rdf::_RDF<<">\n";
}
#ifndef NATIVE
void objrdf::generate_index(){
	rdfs::Class::allocator_type a;
	for(auto i=a.cbegin();i!=a.cend();++i){
		pool_allocator::pool::POOL_PTR p(i.get_cell_index()); //there is a mapping between Class and pools
		for(auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);++j){
			base_resource::get_index()[j->id]=j;
		}
	}
}
#endif
RESOURCE_PTR objrdf::find(uri u){
	cerr<<"looking up uri `"<<u<<"'...";
	auto i=base_resource::get_index().find(u);
	if(i==base_resource::get_index().end()){
		cerr<<"not found"<<endl;
		#ifdef NATIVE
		return RESOURCE_PTR(0);
		#else
		return RESOURCE_PTR(0,0);
		#endif
	}
	cerr<<"found"<<endl;
	return i->second;
}
RESOURCE_PTR objrdf::create_by_type(CONST_CLASS_PTR c,uri id){
	RESOURCE_PTR rp(c->t.allocate());
	c->t.ctor(rp,id);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type(uri type,uri id){
	CONST_CLASS_PTR c=find_t<rdfs::Class>(type);
	#ifdef NATIVE
	return c ? objrdf::create_by_type(c,id) : RESOURCE_PTR(0);
	#else
	return c ? objrdf::create_by_type(c,id) : RESOURCE_PTR(0,0);
	#endif
}
RESOURCE_PTR objrdf::create_by_type_blank(CONST_CLASS_PTR c){
	RESOURCE_PTR rp(c->t.allocate());
	ostringstream os;
	//rp._print(os);
	//uri u(os.str());
	//u.index=1;//local 
	uri u=get_uri(rp);
	c->t.ctor(rp,u);
	return rp;
}
RESOURCE_PTR objrdf::clone(CONST_RESOURCE_PTR r){
	LOG<<"cloning resource `"<<r->id<<"'"<<endl;
	CONST_CLASS_PTR c=get_class(r);
	RESOURCE_PTR rp(c->t.allocate());
	c->t.cctor(rp,r);
	return rp;
}
RESOURCE_PTR objrdf::clone_and_swap(RESOURCE_PTR r){
	LOG<<"cloning resource `"<<r->id<<"'"<<endl;
	CONST_CLASS_PTR c=get_class(r);
	RESOURCE_PTR rp(c->t.allocate());
	memcpy(rp,r,c->cget<sizeOf>().t);//this could go very wrong if we don't have the right size
	c->t.cctor(r,rp);
	return rp;
}
RESOURCE_PTR objrdf::create_by_type_blank(uri type){
	CONST_CLASS_PTR c=find_t<rdfs::Class>(type);
	#ifdef NATIVE
	return c ? objrdf::create_by_type_blank(c) : RESOURCE_PTR(0);
	#else
	return c ? objrdf::create_by_type_blank(c) : RESOURCE_PTR(0,0);
	#endif
}

/*
int main(){
rdfs::Class::get_class()->to_turtle(cout);
}*/
