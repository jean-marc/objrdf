#include "uri.h"
#include <sstream>
#include <string.h>
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
 	*	v[1] is for blank nodes
 	*/ 
	//static vector<ns_prefix> *v=new vector<ns_prefix>(2);
	static vector<ns_prefix> *v=new vector<ns_prefix>({ns_prefix(),ns_prefix("","_:")});
	return *v;
}
struct match_ns{
	const string ns;
	match_ns(const string ns):ns(ns){}
	bool operator()(uri::ns_prefix& n)const{return ns==n.first;}
};
uri::uri():index(0){
	#ifdef PERSISTENT
	local[0]=0;//empty string
	#endif
	cerr<<"new uri `"<<local<<"'"<<endl;
}
string get(void* p){
	ostringstream os;
	os<<p;
	return os.str();
}
#ifdef PERSISTENT
uri::uri(string _local):index(0){
	local[uri::STR_SIZE-1]=0;
	if(_local.empty()) 
		strncpy(local,get(this).c_str(),uri::STR_SIZE-1);
	else
		strncpy(local,_local.c_str(),uri::STR_SIZE-1);
	cerr<<"new uri `"<<local<<"'"<<endl;
}
#else
uri::uri(string local):local(local.empty()?get(this):local),index(0){}
#endif

#ifdef PERSISTENT
uri::uri(string ns,string _local){
	//does not use ns!
	local[uri::STR_SIZE-1]=0;
	strncpy(local,_local.c_str(),uri::STR_SIZE-1);
#else
uri::uri(string ns,string local):local(local){
#endif
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
#ifdef PERSISTENT
uri::uri(string ns,string prefix,string _local){
	local[uri::STR_SIZE-1]=0;
	strncpy(local,_local.c_str(),uri::STR_SIZE-1);
#else
uri::uri(string ns,string prefix,string local):local(local){
#endif
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
uri uri::bnode_uri(string s){
	uri u(s);
	u.index=1;
	return u;
}
uri uri::hash_uri(string s){
	/*
 	*	
 	*/	 
	size_t i=s.find_first_of('#');
#ifdef WIN32
	if(i==0) return uri(s.substr(i+1));
	if(i==string::npos) return uri(s);
	return uri(s.substr(0,i+1),s.substr(i+1));
#else
	switch(i){
		case 0:return uri(s.substr(i+1));
		case string::npos:return uri(s); //not sure what to do here?
		default:return uri(s.substr(0,i+1),s.substr(i+1));
	}
#endif
	//return (i!=string::npos) ? uri(s.substr(0,i+1),s.substr(i+1)) : uri(s);
}
string uri::ns() const{
	return ns_v()[index].first;
}
bool uri::empty() const{
#ifdef PERSISTENT
	return strlen(local)==0;
#else
	return local.empty();
#endif
}
bool uri::is_local() const{
	return index==0;
}
bool uri::operator==(const uri& u) const{
#ifdef PERSISTENT
	//cerr<<"`"<<local<<"'("<<index<<")==`"<<u.local<<"'("<<u.index<<")"<<endl;
	return index==u.index && strcmp(local,u.local)==0;
#else
	return index==u.index && local==u.local;
#endif
}
bool uri::operator!=(const uri& u) const{
#ifdef PERSISTENT
	return index!=u.index || strcmp(local,u.local);
#else
	return index!=u.index || local!=u.local;
#endif
}
int uri::compare(const uri& u) const{
#ifdef PERSISTENT
	return index==u.index ? (strcmp(local,u.local)) : index-u.index;
#else
	return index==u.index ? local.compare(u.local) : index-u.index;
#endif
}
bool uri::operator<(const uri& u) const{
	return compare(u)<0;
}
#ifdef PERSISTENT
uri::uri(const uri& u):index(u.index){
	strcpy(local,u.local);
}
uri& uri::operator=(const uri& u){
	strcpy(local,u.local);
	index=u.index;
}
#endif
void uri::print(){
	for(vector<ns_prefix>::iterator i=ns_v().begin();i<ns_v().end();++i)
		cout<<(i-ns_v().begin())<<"\t"<<i->second<<"\t"<<i->first<<endl;
}
bool comp(const uri::ns_prefix& a,const uri::ns_prefix& b){return a.second<b.second;}
struct pred{
	const string s;
	pred(const string s):s(s){}
	bool operator()(const uri::ns_prefix& u){return u.second==s;}
};
void uri::check(){
	int index=0;
	//print();
	/*
 	*	make sure all prefixes are unique
 	*	it could also be nice t
 	*/ 
	for(vector<ns_prefix>::iterator i=ns_v().begin()+1;i<ns_v().end();++i){
		if(count_if(ns_v().begin()+1,ns_v().end(),pred(i->second))>1){
			ostringstream os;
			os<<"_"<<index<<":";
			i->second=os.str();
			++index;
		}
	}
}
void uri::ns_declaration(ostream& os){
	check();
	for(auto i=ns_v().cbegin()+2;i<ns_v().cend();++i){
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
void uri::to_uri(ostream& os) const{
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

