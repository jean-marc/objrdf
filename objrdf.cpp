#include "objrdf.h"
#include <sstream>
#include <algorithm>
namespace objrdf{
	//color scheme in bash, could be customized by namespace
	char start_id[]="\033[32m";
	char stop_id[]="\033[m";
}
using namespace objrdf;

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
#ifdef WIN32

#else
	std::get<8>(first.i->t)(this,first.index,last.index);
#endif
}
void base_resource::erase(instance_iterator position){
#ifdef WIN32

#else
	std::get<8>(position.i->t)(this,position.index,position.index+1);
#endif
}
void base_resource::get_output(ostream& os){
	//what would be most appropriate HTTP message?	
}
#ifndef PERSISTENT
vector<shared_ptr<rdfs::Class> >& rdfs::Class::get_instances(){
	static vector<shared_ptr<rdfs::Class> > v;
	return v;
}
#endif
struct cmp_uri{
	uri u;
	cmp_uri(uri u):u(u){}
	bool operator()(CLASS_PTR s)const{return s->id==u;}
};
rdfs::Class::Class(objrdf::uri id,rdfs::subClassOf s,objrdf::base_resource::class_function_table t,string comment_str):SELF(id),t(t)/*,debug(0xdeadbeef)*/{
	#ifdef OBJRDF_VERB
	cerr<<"create rdfs::Class `"<<id<<"'\t"<<this<</*"\t"<<t<<*/endl;
	#endif
	
	#ifndef PERSISTENT
	vector<shared_ptr<rdfs::Class>>::iterator i=find_if(get_instances().begin(),get_instances().end(),cmp_uri(id));
	if(i!=get_instances().end()) throw runtime_error("duplicate class "+string(id.local));
	get<c_index>().t=get_instances().size();
	get_instances().push_back(shared_ptr<rdfs::Class>(this));
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
#ifndef PERSISTENT
shared_ptr<rdfs::Class> rdfs::Class::create_Class(objrdf::uri id,subClassOf s,objrdf::base_resource::class_function_table t,string comment){
	//need to get rid of this
	vector<shared_ptr<rdfs::Class> >::iterator i=find_if(get_instances().begin(),get_instances().end(),cmp_uri(id));
	return (i!=get_instances().end())? i->get() : new Class(id,s,t,comment);
}
#endif
bool rdfs::Class::operator==(const rdfs::Class& c) const{
	return this==&c;	
}
bool rdfs::Class::operator<(const rdfs::Class& c) const{//is c subClass of this?
	//can still use this with slight modification
	//#ifdef PERSISTENT
	//void* pointer comparison
	//return (c.get_const<rdfs::subClassOf>().get()==this)||(c.get_const<rdfs::subClassOf>().get()&&(*this<*c.get_const<rdfs::subClassOf>()));
	//#else
	auto tmp=c.get_const<array<subClassOf> >();//not very efficient
	for(auto i=tmp.begin();i<tmp.end();++i){
		cerr<<this->id<<"\t"<<(*i)->id<<endl;
		//cerr<<this->id<<"\t"<<((subClassOf)(*i))->id<<endl;
		if(*i==this) return true;
	}
	return false;
	//#endif
}
bool rdfs::Class::operator<=(const rdfs::Class& c) const{
	return (*this==c)||(*this<c);
}
bool rdfs::Class::is_subclass_of(const Class& c) const{
	return c<=*this;
}
bool rdfs::Class::literalp() const{
	//return (this==rdf::Literal::get_class().get()) || (get<subClassOf>() && get<subClassOf>()->literalp());
	return *rdf::Literal::get_class()<*this;
}
void rdfs::Class::analyze(){
	#ifdef PERSISTENT

	#else
	static bool first=true;
	if(!first) return;
	for(auto i=get_instances().begin();i<get_instances().end();++i){
		for(auto j=(*i)->get<array<subClassOf> >().begin();j<(*i)->get<array<subClassOf> >().end();++j){
			((subClassOf)(*j))->get<array<superClassOf> >().push_back(superClassOf(*i));
		}
	}
	first=false;
	#endif
};
CLASS_PTR base_resource::get_class(){
	//POOL_PTR i=shared_ptr<base_resource>::pointer::get_pool();
	//cerr<<"class `base_resource' uses pool number "<<(unsigned int)i.index<<endl;
	//static shared_ptr<rdfs::Class> p(shared_ptr<rdfs::Class>::pointer::construct_at(
	static CLASS_PTR p=CLASS_PTR::construct_at(
		//i.index,
		//shared_ptr<base_resource>::pointer::get_pool()->type_id,
		POOL_PTR::get_type_id<base_resource>(),
		objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),
		rdfs::subClassOf(),
		objrdf::base_resource::class_function_table(
			f_ptr::constructor<base_resource>,
			f_ptr::begin<base_resource>,
			f_ptr::end<base_resource>,
			f_ptr::cbegin<base_resource>,
			f_ptr::cend<base_resource>
			/*
			,&base_resource::fbegin<base_resource>
			,&base_resource::fend<base_resource>
			*/
		)			
		,get_comment()
		);
	//);
	return p;
}
#ifndef PERSISTENT
vector<PROPERTY_PTR >& rdf::Property::get_instances(){
	static vector<PROPERTY_PTR > v;
	return v;
}
#endif
/*
rdf::Property::Property():literalp(true){
	get<p_index>().t=get_instances().size();
	get<p_self>()=this;
	get_instances().push_back(this);
}
*/
rdf::Property::Property(objrdf::uri id):SELF(id),literalp(true){
	#ifdef OBJRDF_VERB
	//cerr<<get<p_self>().use_count()<<endl;
	#endif
	//that does not work with shared_ptr
	//problem here: we can not use this pointer, we need a pseudo_ptr to this instance, should be done at construction
	#ifndef PERSISTENT
	get<p_index>().t=get_instances().size();
	get<p_self>()=p_self(p_self::PTR(this));
	#endif
	//get<p_self>()=p_self(this);
	#ifdef OBJRDF_VERB
	//cerr<<get<p_self>().use_count()<<endl;
	#endif
	#ifndef PERSISTENT
	get_instances().push_back(p_self::PTR(this));
	#endif
	#ifdef OBJRDF_VERB
	//cerr<<get<p_self>().use_count()<<endl;
	#endif
}
rdf::Property::Property(objrdf::uri id,rdfs::range r,const bool literalp):rdf::Property::SELF(id),literalp(literalp){
	cerr<<"creating Property "<<id<<" ref count:"<<n<<" range:"<<r->id<<" range ref count:"<<r->n<<endl;
	get<rdfs::range>()=r;
	#ifdef PERSISTENT
	//n=1;
	#else
	get<p_index>().t=get_instances().size();
	get<p_self>()=p_self(p_self::PTR(this));
	get_instances().push_back(p_self::PTR(this));
	#endif
}
base_resource::instance_iterator rdf::Property::get_self_iterator(){
	//should do a static request
	auto i=find_if(begin(),end(),match_property(p_self::get_property()));
	assert(i!=end());
	return i->begin();
	//return base_resource::instance_iterator(this,v.begin()+2+tuple_index<objrdf::p_self,PROPERTIES>::value,0);
}
base_resource::const_instance_iterator rdf::Property::get_const_self_iterator() const{
	auto i=find_if(cbegin(),cend(),match_property(p_self::get_property()));
	assert(i!=cend());
	return i->cbegin();
	//return base_resource::const_instance_iterator(this,v.begin()+2+tuple_index<objrdf::p_self,PROPERTIES>::value,0);
}
#ifdef PERSISTENT
PROPERTY_PTR rdf::Property::nil=PROPERTY_PTR(PROPERTY_PTR::pointer::construct(uri("nil_p")));
#else
PROPERTY_PTR rdf::Property::nil=PROPERTY_PTR(new rdf::Property(uri("nil_p")));
#endif

V base_resource::v=get_generic_property<base_resource>::go();
#ifdef PERSISTENT
CONST_RESOURCE_PTR base_resource::nil=CONST_RESOURCE_PTR::construct(uri("nil"));
#else
shared_ptr<base_resource> base_resource::nil=shared_ptr<base_resource>(new base_resource(uri("nil")));
#endif
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
	return std::get<4>(i->t)(subject,index);
}
CONST_RESOURCE_PTR base_resource::const_instance_iterator::get_const_object() const{
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
	#ifdef OBJRDF_VERB
	cerr<<"add_property:"<<p<<endl;
	#endif
	//awkward
	std::get<7>(static_cast<V::iterator>(*this)->t)(subject,p);
	//return instance_iterator(*this,get_size()-1);
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
	//problem if we don't use virtual functions
	//apply<begin>()
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
	CLASS_PTR get_class(CONST_RESOURCE_PTR r){return CLASS_PTR(r.pool_ptr->type_id);}
	base_resource::type_iterator begin(RESOURCE_PTR r){return std::get<1>(get_class(r)->t)(r);}
	base_resource::type_iterator end(RESOURCE_PTR r){return std::get<2>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return std::get<3>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return std::get<4>(get_class(r)->t)(r);}
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os){
		os<<"\n<"<<r->get_Class()->id<<" "<<(r->id.is_local() ? rdf::ID : rdf::about)<<"='";
		r->id.to_uri(os);
		os<<"'>";
		//extract unique id
		size_t _id=r.pool_ptr.index | (r.index<<(sizeof(r.pool_ptr.index)<<3));
		os<<hex<<"{"<<_id<<"}"<<dec;
		for(auto i=cbegin(r);i!=cend(r);++i){
			for(base_resource::const_instance_iterator j=i->cbegin();j!=i->cend();++j){
				//should test if constant or not
				if(i->literalp())
					os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
				else{
					os<<"\n\t<"<<i->get_Property()->id<<" "<<rdf::resource<<"='"<<(j->get_const_object()->id.is_local() ? "#" : "");
					j->get_const_object()->id.to_uri(os);
					//os<<j->get_const_object()->id.local;
					os<<"'/>";
				}
			}
		}
		os<<"\n</"<<r->get_Class()->id<<">";
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
#ifndef PERSISTENT
void rdf::RDF::insert(shared_ptr<objrdf::base_resource> r){
	if(r->id.empty()){//that should not be allowed!
		get<V>().push_back(pp(r));
	}else{
		MAP::iterator i=m.find(r->id);
		if(i==m.end()){
			get<V>().push_back(pp(r));
			m[r->id]=r;
			//mm.insert(MULTIMAP::value_type(r->get_Class()->get_const<objrdf::c_index>().t,r));
		}else{
			#ifdef OBJRDF_VERB
			cerr<<"duplicate resource :'"<<r->id<<"'"<<endl;
			#endif
			//delete r when goes out of scope!
		}
	}
};	
shared_ptr<base_resource> rdf::RDF::find(uri s){
	/*
 	*		
 	*/
	MAP::iterator i=m.find(s);
	return (i!=m.end()) ? shared_ptr<base_resource>(i->second) : shared_ptr<base_resource>();
};
void rdf::RDF::to_turtle(ostream& os){
	//for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_turtle(os);
}
void rdf::RDF::to_turtle_pretty(ostream& os){
	//for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_turtle_pretty(os);
}
void rdf::RDF::to_rdf_xml(ostream& os,const PROVENANCE& p){
	os<<"<"<<_RDF<<"\n";
	uri::ns_declaration(os);
	os<<">";
	//should test provenance here
	for(V::iterator i=get<V>().begin();i!=get<V>().end();++i) (*i)->to_rdf_xml(os,p);
	os<<"\n</"<<_RDF<<">\n";
}
void rdf::RDF::to_rdf_xml_pretty(ostream& os){
	#ifdef WIN32 
	return to_rdf_xml(os);
	#endif
	os<<"\033[36m<"<<_RDF<<"\n";
	uri::ns_declaration(os);
	os<<">";
	for(V::iterator i=get<V>().begin();i!=get<V>().end();++i) (*i)->to_rdf_xml_pretty(os);
	//for(auto i=m.begin();i!=m.end();++i) i->second->to_rdf_xml_pretty(os);//maybe we can get rid of vector<shared_ptr<base_resource> > and use solely map<>
	os<<"\n\033[36m</"<<_RDF<<">\033[m\n";
}
shared_ptr<objrdf::base_resource> rdf::RDF::query(uri _i){
	/*
 	*	optimization for rdf::Property and rdfs::Class, they are the first resources
 	*
 	*/
	MAP::iterator i=m.find(_i);	
	return (i!=m.end()) ? shared_ptr<base_resource>(i->second) : shared_ptr<base_resource>();
}
rdf::RDF::RDF(uri u):SELF(u){
	#ifndef PERSISTENT
	for(auto i=rdf::Property::get_instances().begin();i<rdf::Property::get_instances().end();++i) insert(*i);
	for(auto i=rdfs::Class::get_instances().begin();i<rdfs::Class::get_instances().end();++i) insert(*i);
	#endif
	//insert(base_resource::nil);
	rdfs::Class::analyze();
}
rdf::RDF::~RDF(){
	/*
 	*	before deleting resources all the links need to be severed
 	*/ 
	for(auto i=get<V>().begin();i!=get<V>().end();++i){
		for(auto j=((pp&)(*i))->begin();j<((pp&)(*i))->end();++j){
			((pp&)(*i))->erase(j->begin(),j->end());
		}
	}
}
#endif
/*
int main(){
	rdfs::Class::get_class()->to_turtle(cout);
}*/
