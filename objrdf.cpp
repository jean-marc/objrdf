#include "objrdf.h"
#include <sstream>
#include <algorithm>
namespace objrdf{
	//color scheme in bash, can be customized by namespace
	char start_id[]="\033[32m";
	char stop_id[]="\033[m";

}
using namespace objrdf;

namep::namep(string _n):n(_n){}
bool namep::operator() (generic_property* p) const{return p->p->id==n;}
bool type_p::operator() (shared_ptr<base_resource> r) const{return *t<=*r->get_Class();}

void generic_property::set_string(base_resource*,string s){};
void generic_property::in(base_resource*,istream& is,int){assert(0);}; 
void generic_property::out(base_resource*,ostream& os,int){assert(0);};
void generic_property::add_property(base_resource*,int){};
shared_ptr<base_resource> generic_property::get_object(base_resource*,int){return 0;};
void generic_property::set_object(base_resource*,base_resource*,int){};
generic_property::generic_property(rdf::Property* p,const bool _literalp):literalp(_literalp){
	offset=0;
	this->p=p;
}
void generic_property::print() const{
	//cerr<<p<<"\t"<<p->id<<"\t\t"<<literalp<<"\t"<<offset<<endl;
	cerr<<p.get()<<"\t"<<p->id<<"\t\t"<<literalp<<"\t"<<offset<<endl;
}
pseudo_property::pseudo_property(rdfs::Class* _object):generic_property(rdfs::type::get_property(),false){object=_object;}
int pseudo_property::get_size(base_resource* subject){return 1;}
void pseudo_property::set_object(base_resource* subject,base_resource* object,int index){
	cerr<<"the type can not be changed!"<<endl;
}
/*
void pseudo_property::in(base_resource*,istream& is,int){} 
void pseudo_property::out(base_resource*,ostream& os,int){}
*/
shared_ptr<base_resource> pseudo_property::get_object(base_resource* subject,int index){return object;}

vector<rdfs::Class*>& rdfs::Class::get_instances(){
	static vector<Class*> v;
	return v;
}
rdfs::Class::Class(){
	get<c_index>().t=get_instances().size();
	get_instances().push_back(this);
	this->f=0;
}
rdfs::Class::Class(string id){
	get<c_index>().t=get_instances().size();
	get_instances().push_back(this);
	this->id=id;
	this->f=0;
}
rdfs::Class::Class(string id,rdfs::subClassOf s,objrdf::fpt f,string comment_str){
	//cerr<<"creating Class "<<id<<" ref count:"<<n<<endl;
	get<c_index>().t=get_instances().size();
	get_instances().push_back(this);
	this->id=id;
	this->f=f;
	get<comment>().t=comment_str;
	get<subClassOf>()=s;
}
bool rdfs::Class::operator==(rdfs::Class& c) const{
	return this==&c;	
}
bool rdfs::Class::operator<(rdfs::Class& c) const{//is c subClass of this?
	return (c.get<rdfs::subClassOf>().get()==this)||(c.get<rdfs::subClassOf>().get()&&(*this<*c.get<rdfs::subClassOf>().get()));
}
bool rdfs::Class::operator<=(rdfs::Class& c) const{
	return (*this==c)||(*this<c);
}
bool rdfs::Class::literalp(){
	//return *rdf::Literal::get_class()<=(*this);
	return (this==rdf::Literal::get_class()) || (get<subClassOf>().get() && get<subClassOf>()->literalp());
}
rdfs::Class* base_resource::get_class(){
	static rdfs::Class* c=new rdfs::Class("Resource",shared_ptr<rdfs::Class>(0),get_constructor<base_resource>(),get_comment());
	//c->get<rdfs::comment>().t=get_comment();
	return c;
}
vector<rdf::Property*>& rdf::Property::get_instances(){
	static vector<rdf::Property*> v;
	return v;
}
rdf::Property::Property():literalp(true){
	get<p_index>().t=get_instances().size();
	get_instances().push_back(this);
}
rdf::Property::Property(string id,rdfs::range r,const bool _literalp):literalp(_literalp){
	//cerr<<"creating Property "<<id<<" ref count:"<<n<<" range:"<<r->id<<" range ref count:"<<r->n<<endl;
	this->id=id;
	get<rdfs::range>()=r;
	get<p_index>().t=get_instances().size();
	get_instances().push_back(this);
}
V base_resource::v;

shared_ptr<base_resource> base_resource::nil=shared_ptr<base_resource>(new base_resource(uri("nil")));
//rdf::Property* base_resource::iterator::get_Property() const{return BASE::operator*()->p;}
shared_ptr<rdf::Property> base_resource::iterator::get_Property() const{return BASE::operator*()->p;}

int base_resource::iterator::get_size() const{return BASE::operator*()->get_size(subject);}
bool base_resource::iterator::literalp() const{return BASE::operator*()->literalp;}
void base_resource::instance_iterator::in(istream& is){
	BASE::operator*()->in(subject,is,index);
}
void base_resource::instance_iterator::out(ostream& os){
	BASE::operator*()->out(subject,os,index);
}
string base_resource::instance_iterator::str(){
	ostringstream os;
	out(os);
	return os.str();
}
void base_resource::instance_iterator::set_string(string s){
	BASE::operator*()->set_string(subject,s);
}
shared_ptr<base_resource> base_resource::instance_iterator::get_object(){
	return BASE::operator*()->get_object(subject,index);
}
void base_resource::instance_iterator::set_object(base_resource* r){
	BASE::operator*()->set_object(subject,r,index);
}
base_resource::instance_iterator base_resource::type_iterator::add_property(){
	BASE::operator*()->add_property(subject,0);//????
	return instance_iterator(*this,get_size()-1);
}
void base_resource::_tmp_::operator=(const string& value){
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
	//string s=id+" ";
	string s=start_id+id.name+stop_id+" ";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			//os<<((j==i->begin()) ? s+i->get_Property()->id : ",")<<" "<<*j;
			os<<((j==i->begin()) ? s+i->get_Property()->id.name : ",")<<" "<<*j;
			s="; ";
		}
	}
	os<<" .\n";	
}
void base_resource::to_rdf_xml(ostream& os){
	os<<"<"<<get_Class()->id<<" id='";//<<id<<"'>";
	if(id.empty())
		os<<this;
	else	
		os<<id;
	os<<"'>";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			if(i->literalp())
				os<<"<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
			else
				os<<"<"<<i->get_Property()->id<<" resource='"<<*j<<"'/>";
		}
	}
	os<<"</"<<get_Class()->id<<">";
}
void base_resource::to_rdf_xml_pretty(ostream& os){
#ifdef WIN32
	os<<"\n<"<<get_Class()->id<<" id='";
#else
	os<<"\n\033[36m<"<<get_Class()->id<<" \033[32mid=\033[31m'";//<<id<<"'>";
#endif
	if(id.empty())
		os<<this;
	else	
		os<<id;
#ifdef VERBOSE
	os<<"' ref_count='"<<n;
#endif
#ifdef WIN32
	os<<"'>";
#else
	os<<"'\033[36m>";
#endif
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			if(i->literalp())
#ifdef WIN32
				os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
#else
				os<<"\n\t\033[36m<"<<i->get_Property()->id<<">\033[m"<<*j<<"\033[36m</"<<i->get_Property()->id<<">";
#endif
			else
#ifdef WIN32
				os<<"\n\t<"<<i->get_Property()->id<<" resource='"<<*j<<"'/>";
#else
				os<<"\n\t\033[36m<"<<i->get_Property()->id<<" \033[32mresource=\033[31m'"<<*j<<"'\033[36m/>";
#endif
		}
	}
#ifdef WIN32
	os<<"\n</"<<get_Class()->id<<">";
#else
	os<<"\n\033[36m</"<<get_Class()->id<<">\033[m";
#endif
}
void base_resource::to_xml(ostream& os){
	os<<"<"<<get_Class()->id<<" id='"<<id<<"'>";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		if(i->get_Property()!=rdfs::type::get_property()){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					os<<"<"<<i->get_Property()->id<<">";
					if(i->literalp())
						j->out(os);	
					else
						j->get_object()->to_xml(os);//watch for loops!!!
					os<<"</"<<i->get_Property()->id<<">";
			}
		}
	}
	os<<"</"<<get_Class()->id<<">";
}
void rdf::RDF::insert(objrdf::shared_ptr<objrdf::base_resource> r){
	if(r->id.empty()){
		v.push_back(r);//maybe should be given id
	}else{
		MAP::iterator i=m.find(r->id);
		if(i==m.end()){
			v.push_back(r);
			m[r->id]=r.get();
		}else{
			//duplicate resource
			//delete r when goes out of scope!
		}
	}
};	
objrdf::base_resource* rdf::RDF::find(string s){
	MAP::iterator i=m.find(s);
	return (i!=m.end()) ? i->second : 0;
};
void rdf::RDF::to_rdf_xml(ostream& os){
	os<<"<RDF>";
	for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_rdf_xml(os);
	os<<"</RDF>";
}
void rdf::RDF::to_turtle(ostream& os){
	for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_turtle(os);
}
void rdf::RDF::to_turtle_pretty(ostream& os){
	for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_turtle_pretty(os);
}
void rdf::RDF::to_rdf_xml_pretty(ostream& os){
//#ifdef WIN32
//	os<<"<RDF>";
//	for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_rdf_xml_pretty(os);
//	os<<"\n</RDF>";
//#else
	os<<"\033[36m<RDF>";
	for(V::iterator i=v.begin();i!=v.end();++i) (*i)->to_rdf_xml_pretty(os);
	os<<"\n\033[36m</RDF>\033[m";
//#endif
	os<<endl;
}
objrdf::shared_ptr<objrdf::base_resource> rdf::RDF::query(string _i){
	MAP::iterator i=m.find(_i);	
	return (i!=m.end()) ? i->second : 0;
}
rdf::RDF::RDF(){
	for(vector<rdfs::Class*>::iterator i=rdfs::Class::get_instances().begin();i<rdfs::Class::get_instances().end();++i){
		//cerr<<(*i)->id<<" ref count:"<<(*i)->n<<endl;
		insert(*i);
		//cerr<<(*i)->id<<" ref count:"<<(*i)->n<<endl;
	}
	cerr<<rdf::Property::get_instances().size()<<endl;
	for(vector<rdf::Property*>::iterator i=rdf::Property::get_instances().begin();i<rdf::Property::get_instances().end();++i){
		//cerr<<(*i)->id<<" ref count:"<<(*i)->n<<endl;
		insert(*i);
		//cerr<<(*i)->id<<" ref count:"<<(*i)->n<<endl;
		//(*i)->to_rdf_xml(cerr);
	}
}
void rdf::RDF::introspect(){
}
/*
int main(){
	rdfs::Class::get_class()->to_turtle(cout);
}*/
