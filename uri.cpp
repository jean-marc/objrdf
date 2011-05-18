#include "uri.h"
#include <sstream>
using namespace objrdf;
/*
 *	preload the vector
 *	v[0]=empty namespace
 */
/*
 * Construct On First Use Idiom 
 * http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.17
 */
vector<uri::ns_prefix>& uri::ns_v(){
	/*
 	*	v[0] is the default/local namespace it's empty, it will be set by xml:base
 	*	if index==0 then we can serialize with rdf:ID otherwise it is rdf:about
 	*/ 
	static vector<ns_prefix> *v=new vector<ns_prefix>(1);
	return *v;
}
struct match_ns{
	const string ns;
	match_ns(const string ns):ns(ns){}
	bool operator()(uri::ns_prefix& n)const{return ns==n.first;}
};
uri::uri():index(0){}
uri::uri(string local):local(local),index(0){}
uri::uri(string ns,string local):local(local){
	//for performance
	static vector<ns_prefix>& _ns_v_=ns_v();
	vector<ns_prefix>::iterator i=find_if(_ns_v_.begin(),_ns_v_.end(),match_ns(ns));
	if(i==_ns_v_.end()){
		_ns_v_.push_back(ns_prefix(ns,""));
		index=_ns_v_.size()-1;
	}else{	
		index=i-_ns_v_.begin();
	}
}	
uri::uri(string ns,string prefix,string local):local(local){
	//for performance
	static vector<ns_prefix>& _ns_v_=ns_v();
	vector<ns_prefix>::iterator i=find_if(_ns_v_.begin(),_ns_v_.end(),match_ns(ns));
	if(i==_ns_v_.end()){
		_ns_v_.push_back(ns_prefix(ns,prefix+":"));
		index=_ns_v_.size()-1;
	}else{	
		index=i-_ns_v_.begin();
	}
}
//uri uri::_uri_(string ns_and_prefix,string local){
	/*
 	*	looks for white space that separates URI from prefix 
 	*/
	//size_t i=ns_and_prefix.find_first_of(' ');
	//return (i!=string::npos) ? uri(ns_and_prefix.substr(0,i),ns_and_prefix.substr(i+1),local) : uri(ns_and_prefix,local);
//}
uri uri::hash_uri(string s){
	/*
 	*	
 	*/	 
	size_t i=s.find_first_of('#');
	switch(i){
		case 0:return uri(s.substr(i+1));
		case string::npos:return s; //not sure what to do here?
		default:return uri(s.substr(0,i+1),s.substr(i+1));
	}
	//return (i!=string::npos) ? uri(s.substr(0,i+1),s.substr(i+1)) : uri(s);
}
bool uri::empty() const{
	return local.empty();
}
bool uri::is_local() const{
	return index==0;
}
bool uri::operator==(const uri& u) const{
	return index==u.index && local==u.local;
}
bool uri::operator!=(const uri& u) const{
	return index!=u.index || local!=u.local;
}
bool uri::operator<(const uri& u) const{
	return index==u.index ? local<u.local : index<u.index;
}
void uri::print(){
	for(vector<ns_prefix>::iterator i=ns_v().begin();i<ns_v().end();++i)
		cout<<(i-ns_v().begin())<<"\t"<<i->second<<"\t"<<i->first<<endl;
}
void uri::check(){
	int index=0;
	for(vector<ns_prefix>::iterator i=ns_v().begin()+1;i<ns_v().end();++i){
		if(i->second.empty()){
			ostringstream os;
			os<<"_"<<index<<":";
			i->second=os.str();
			++index;
		}
	}
	
}
void uri::ns_declaration(ostream& os){
	check();
	for(vector<ns_prefix>::iterator i=ns_v().begin()+1;i<ns_v().end();++i){
		if(i->second.empty())
			os<<"xmlns='"<<i->first<<"'"<<endl;
		else
			os<<"xmlns:"<<i->second.substr(0,i->second.length()-1)<<"='"<<i->first<<"'"<<endl;
	}
}
void uri::ns_declaration_pretty(ostream& os){
	check();
	for(vector<ns_prefix>::iterator i=ns_v().begin();i<ns_v().end();++i){
		if(i->second.empty())
			os<<"xmlns='"<<i->first<<"'"<<endl;
		else
			os<<"xmlns:"<<i->second.substr(0,i->second.length()-1)<<"='"<<i->first<<"'"<<endl;
	}
}
void uri::to_uri(ostream& os){
	//for performance
	static vector<ns_prefix>& _ns_v_=ns_v();
	os<<_ns_v_[index].first<<local;
}
namespace objrdf{
	ostream& operator<<(ostream& os,const uri& u){
		static vector<uri::ns_prefix>& _ns_v_=uri::ns_v();
		return os<<_ns_v_[u.index].second<<u.local;
	}
}

