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
	std::get<8>(first.i->t)(this,first.index,last.index);
}
void base_resource::erase(instance_iterator position){
	std::get<8>(position.i->t)(this,position.index,position.index+1);
}
void base_resource::get_output(ostream& os){
	//what would be most appropriate HTTP message?	
}
struct cmp_uri{
	uri u;
	cmp_uri(uri u):u(u){}
	bool operator()(CLASS_PTR s)const{return s->id==u;}
};
rdfs::Class::Class(objrdf::uri id,rdfs::subClassOf s,objrdf::base_resource::class_function_table t,string comment_str):SELF(id),t(t)/*,debug(0xdeadbeef)*/{
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
	auto tmp=c.get_const<array<subClassOf> >();//not very efficient
	for(auto i=tmp.begin();i<tmp.end();++i){
		cerr<<this->id<<"\t"<<(*i)->id<<endl;
		//cerr<<this->id<<"\t"<<((subClassOf)(*i))->id<<endl;
		if(*i==this) return true;
	}
	return false;
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
};
CLASS_PTR base_resource::get_class(){
	static CLASS_PTR p=CLASS_PTR::construct_at(
		POOL_PTR::get_type_id<base_resource>(),
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
	//);
	return p;
}
rdf::Property::Property(objrdf::uri id):SELF(id),literalp(true){
}
rdf::Property::Property(objrdf::uri id,rdfs::range r,const bool literalp):rdf::Property::SELF(id),literalp(literalp){
	cerr<<"creating Property "<<id<<" ref count:"<<n<<" range:"<<r->id<<" range ref count:"<<r->n<<endl;
	get<rdfs::range>()=r;
}
base_resource::instance_iterator rdf::Property::get_self_iterator(){
	//should do a static request
	auto i=find_if(begin(),end(),match_property(p_self::get_property()));
	assert(i!=end());
	return i->begin();
}
base_resource::const_instance_iterator rdf::Property::get_const_self_iterator() const{
	auto i=find_if(cbegin(),cend(),match_property(p_self::get_property()));
	assert(i!=cend());
	return i->cbegin();
}
PROPERTY_PTR rdf::Property::nil=PROPERTY_PTR(PROPERTY_PTR::pointer::construct(uri("nil_p")));
V base_resource::v=get_generic_property<base_resource>::go();
CONST_RESOURCE_PTR base_resource::nil=CONST_RESOURCE_PTR::construct(uri("nil"));
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
	CLASS_PTR get_class(CONST_RESOURCE_PTR r){return CLASS_PTR(r.pool_ptr->type_id);}
	base_resource::type_iterator begin(RESOURCE_PTR r){return std::get<1>(get_class(r)->t)(r);}
	base_resource::type_iterator end(RESOURCE_PTR r){return std::get<2>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cbegin(CONST_RESOURCE_PTR r){return std::get<3>(get_class(r)->t)(r);}
	base_resource::const_type_iterator cend(CONST_RESOURCE_PTR r){return std::get<4>(get_class(r)->t)(r);}
	void to_rdf_xml(CONST_RESOURCE_PTR r,ostream& os){
		os<<"\n<"<<get_class(r)->id<<" "<<(r->id.is_local() ? rdf::ID : rdf::about)<<"='";
		r->id.to_uri(os);
		os<<"'>";
		//extract unique id
		size_t _id=r.pool_ptr.index | (r.index<<(sizeof(r.pool_ptr.index)<<3));
		os<<hex<<"{"<<_id<<"}"<<dec;
		auto i=cbegin(r);
		++i;//skip first property rdf:type
		for(;i!=cend(r);++i){
		//for(auto i=cbegin(r)+1;i!=cend(r);++i){//skip first property rdf:type, does not work
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
/*
int main(){
	rdfs::Class::get_class()->to_turtle(cout);
}*/
