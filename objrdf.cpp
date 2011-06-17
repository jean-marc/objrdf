#include "objrdf.h"
#include <sstream>
#include <algorithm>
namespace objrdf{
	//color scheme in bash, could be customized by namespace
	char start_id[]="\033[32m";
	char stop_id[]="\033[m";
}
using namespace objrdf;

namep::namep(uri n):n(n){}
bool namep::operator() (generic_property* p) const{return p->p->id==n;}
bool type_p::operator() (shared_ptr<base_resource> r) const{return *t<=*r->get_Class();}

base_resource::type_iterator base_resource::begin(){return base_resource::type_iterator(this,v.begin());}
base_resource::type_iterator base_resource::end(){return base_resource::type_iterator(this,v.end());}
void base_resource::erase(instance_iterator first,instance_iterator last){
#ifdef WIN32

#else
	first->V::iterator::operator*()->erase(this,first.index,last.index);
#endif
}

generic_property::generic_property(shared_ptr<rdf::Property> p,const bool literalp):p(p),literalp(literalp),offset(0){}
void generic_property::set_string(base_resource*,string s){};
void generic_property::in(base_resource*,istream& is,int){assert(0);}; 
void generic_property::out(base_resource*,ostream& os,int){assert(0);};
void generic_property::add_property(base_resource*,int){};
shared_ptr<base_resource> generic_property::get_object(base_resource*,int){return shared_ptr<base_resource>();};
void generic_property::set_object(base_resource*,shared_ptr<base_resource>,int){};
void generic_property::erase(base_resource* subject,int first,int last){}
void generic_property::print() const{
	cerr<<p.get()<<"\t"<<p->id<<"\t\t"<<literalp<<"\t"<<offset<<endl;
}
pseudo_property::pseudo_property(shared_ptr<rdfs::Class> object):generic_property(rdf::type::get_property(),false),object(object){}
int pseudo_property::get_size(base_resource* subject){return 1;}
void pseudo_property::set_object(base_resource* subject,shared_ptr<base_resource> object,int index){
	cerr<<"the type can not be changed!"<<endl;
}
shared_ptr<base_resource> pseudo_property::get_object(base_resource* subject,int index){return object;}

vector<shared_ptr<rdfs::Class> >& rdfs::Class::get_instances(){
	static vector<shared_ptr<rdfs::Class> > v;
	return v;
}
rdfs::Class::Class(uri u):SELF(u),f(0){
	get<c_index>().t=get_instances().size();
	get_instances().push_back(shared_ptr<rdfs::Class>(this));
}
rdfs::Class::Class(objrdf::uri id,rdfs::subClassOf s,objrdf::fpt f,string comment_str):rdfs::Class::SELF(id),f(f){
	//cerr<<"creating Class "<<id<<" ref count:"<<n<<endl;
	//objrdf::uri::print();
	get<c_index>().t=get_instances().size();
	get_instances().push_back(shared_ptr<rdfs::Class>(this));
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
	return (this==rdf::Literal::get_class().get()) || (get<subClassOf>() && get<subClassOf>()->literalp());
}
shared_ptr<rdfs::Class> base_resource::get_class(){
	static shared_ptr<rdfs::Class> c(new rdfs::Class(objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),shared_ptr<rdfs::Class>(),get_constructor<base_resource>(),get_comment()));
	return c;
}
vector<shared_ptr<rdf::Property> >& rdf::Property::get_instances(){
	static vector<shared_ptr<rdf::Property> > v;
	return v;
}
/*
rdf::Property::Property():literalp(true){
	get<p_index>().t=get_instances().size();
	get<p_self>()=this;
	get_instances().push_back(this);
}
*/
rdf::Property::Property(objrdf::uri id):SELF(id),literalp(true){
	get<p_index>().t=get_instances().size();
	#ifdef OBJRDF_VERB
	cerr<<get<p_self>().use_count()<<endl;
	#endif
	//that does not work with shared_ptr
	get<p_self>()=shared_ptr<rdf::Property>(this);
	#ifdef OBJRDF_VERB
	cerr<<get<p_self>().use_count()<<endl;
	#endif
	get_instances().push_back(shared_ptr<rdf::Property>(this));
	#ifdef OBJRDF_VERB
	cerr<<get<p_self>().use_count()<<endl;
	#endif
}
rdf::Property::Property(objrdf::uri id,rdfs::range r,const bool literalp):rdf::Property::SELF(id),literalp(literalp){
	//cerr<<"creating Property "<<id<<" ref count:"<<n<<" range:"<<r->id<<" range ref count:"<<r->n<<endl;
	get<rdfs::range>()=r;
	get<p_index>().t=get_instances().size();
	get<p_self>()=shared_ptr<rdf::Property>(this);
	get_instances().push_back(shared_ptr<rdf::Property>(this));
}
base_resource::instance_iterator rdf::Property::get_self_iterator(){
	return base_resource::instance_iterator(this,v.begin()+5,0);//should make sure position does not change!
}
shared_ptr<rdf::Property> rdf::Property::nil=shared_ptr<rdf::Property>(new rdf::Property(uri("nil_p")));

V base_resource::v;

shared_ptr<base_resource> base_resource::nil=shared_ptr<base_resource>(new base_resource(uri("nil")));
shared_ptr<rdf::Property> base_resource::iterator::get_Property() const{return BASE::operator*()->p;}
shared_ptr<rdf::Property> base_resource::instance_iterator::get_Property() const{return BASE::operator*()->p;}

int base_resource::iterator::get_size() const{return BASE::operator*()->get_size(subject);}
bool base_resource::iterator::literalp() const{return BASE::operator*()->literalp;}
bool base_resource::instance_iterator::literalp() const{return BASE::operator*()->literalp;}
void base_resource::instance_iterator::in(istream& is){
	BASE::operator*()->in(subject,is,index);
}
void base_resource::instance_iterator::out(ostream& os) const{
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
shared_ptr<base_resource> base_resource::instance_iterator::get_object() const{
	return BASE::operator*()->get_object(subject,index);
}
void base_resource::instance_iterator::set_object(shared_ptr<base_resource> r){
	BASE::operator*()->set_object(subject,r,index);
}
base_resource::instance_iterator base_resource::type_iterator::add_property(){
	BASE::operator*()->add_property(subject,0);//????
	return instance_iterator(*this,get_size()-1);
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
	/*
	string s=start_id+id.local+stop_id+" ";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			//os<<((j==i->begin()) ? s+i->get_Property()->id : ",")<<" "<<*j;
			os<<((j==i->begin()) ? s+i->get_Property()->id.local : ",")<<" "<<*j;
			s="; ";
		}
	}
	os<<" .\n";	
	*/
}
void base_resource::to_rdf_xml(ostream& os){
	os<<"\n<"<<get_Class()->id<<" "<<(id.is_local() ? rdf::ID : rdf::about)<<"='";
	id.to_uri(os);
	os<<"'>";
	for(base_resource::type_iterator i=begin();i!=end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			if(i->literalp())
				os<<"\n\t<"<<i->get_Property()->id<<">"<<*j<<"</"<<i->get_Property()->id<<">";
			else{
				os<<"\n\t<"<<i->get_Property()->id<<" "<<rdf::resource<<"='"<<(j->get_object()->id.is_local() ? "#" : "");
				j->get_object()->id.to_uri(os);
				os<<"'/>";
			}
		}
	}
	os<<"\n</"<<get_Class()->id<<">";
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
				os<<"\n\t\033[36m<"<<i->get_Property()->id<<">\033[m"<<*j<<"\033[36m</"<<i->get_Property()->id<<">";
			else{
				os<<"\n\t\033[36m<"<<i->get_Property()->id<<" \033[32m"<<rdf::resource<<"=\033[31m'"<<(j->get_object()->id.is_local() ? "#" : "");
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
void rdf::RDF::insert(shared_ptr<objrdf::base_resource> r){
	if(r->id.empty()){
		get<V>().push_back(r);
	}else{
		MAP::iterator i=m.find(r->id);
		if(i==m.end()){
			get<V>().push_back(r);
			m[r->id]=r;
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
void rdf::RDF::to_rdf_xml(ostream& os){
	os<<"<"<<_RDF<<"\n";
	uri::ns_declaration(os);
	os<<">";
	for(V::iterator i=get<V>().begin();i!=get<V>().end();++i) (*i)->to_rdf_xml(os);
	os<<"\n</"<<_RDF<<">\n";
}
void rdf::RDF::to_rdf_xml_pretty(ostream& os){
	#ifdef __MINGW32__ 
	return to_rdf_xml(os);
	#endif
	os<<"\033[36m<"<<_RDF<<"\n";
	uri::ns_declaration(os);
	os<<">";
	for(V::iterator i=get<V>().begin();i!=get<V>().end();++i) (*i)->to_rdf_xml_pretty(os);
	os<<"\n\033[36m</"<<_RDF<<">\033[m\n";
}
shared_ptr<objrdf::base_resource> rdf::RDF::query(uri _i){
	MAP::iterator i=m.find(_i);	
	return (i!=m.end()) ? shared_ptr<base_resource>(i->second) : shared_ptr<base_resource>();
}
rdf::RDF::RDF(uri u):SELF(u){
	#ifdef __GXX_EXPERIMENTAL_CXX0X__
	for(auto i=rdf::Property::get_instances().begin();i<rdf::Property::get_instances().end();++i) insert(*i);
	for(auto i=rdfs::Class::get_instances().begin();i<rdfs::Class::get_instances().end();++i) insert(*i);
	#else
	for(vector<shared_ptr<rdf::Property> >::iterator i=rdf::Property::get_instances().begin();i<rdf::Property::get_instances().end();++i) insert(*i);
	for(vector<shared_ptr<rdfs::Class> >::iterator i=rdfs::Class::get_instances().begin();i<rdfs::Class::get_instances().end();++i) insert(*i);
	#endif
	insert(base_resource::nil);
}
rdf::RDF::~RDF(){
	/*
 	*	before deleting resources all the links need to be severed
 	*/ 
	#ifdef __GXX_EXPERIMENTAL_CXX0X__
	for(auto i=get<V>().begin();i!=get<V>().end();++i){
		for(auto j=(*i)->begin();j<(*i)->end();++j){
			(*i)->erase(j->begin(),j->end());
		}
	}
	#else
	for(V::iterator i=get<V>().begin();i!=get<V>().end();++i){
		for(base_resource::type_iterator j=(*i)->begin();j<(*i)->end();++j){
			(*i)->erase(j->begin(),j->end());
		}
	}
	#endif
}
/*
int main(){
	rdfs::Class::get_class()->to_turtle(cout);
}*/
