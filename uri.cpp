#include "uri.h"
#include <sstream>
#include <string.h>
#include <iomanip>
#include <parser/url_decoder.h>
using namespace objrdf;
using namespace std;
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
#ifdef __GNUG__
	static vector<ns_prefix> *v=new vector<ns_prefix>({ns_prefix(),ns_prefix("blank#","_:")});
#else
	static vector<ns_prefix> *v=new vector<ns_prefix>(2);
#endif
	return *v;
}
struct match_ns{
	const string ns;
	match_ns(const string ns):ns(ns){}
	bool operator()(uri::ns_prefix& n)const{return ns==n.first;}
};
uri::uri():index(0){
	local[0]=0;//empty string
	cerr<<"new uri `"<<local<<"'"<<endl;
}
//should not use this!, use blank nodes instead
string get(void* p){
	ostringstream os;
	os<<p;
	return os.str();
}
uri::uri(string _local):index(0){
	local[uri::STR_SIZE-1]=0;
	if(_local.empty()) 
		strncpy(local,get(this).c_str(),uri::STR_SIZE-1);
	else
		strncpy(local,_local.c_str(),uri::STR_SIZE-1);
	cerr<<"new uri `"<<local<<"'"<<endl;
}
uri::uri(string ns,string _local){
	//does not use ns!
	local[uri::STR_SIZE-1]=0;
	strncpy(local,_local.c_str(),uri::STR_SIZE-1);
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
uri::uri(string ns,string prefix,string _local){
	local[uri::STR_SIZE-1]=0;
	strncpy(local,_local.c_str(),uri::STR_SIZE-1);
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
}
//look up prefix
uri uri::qname_uri(std::string s){
	size_t i=s.find_first_of(':');
	if(i==string::npos) return uri();
	auto prefix=s.substr(0,i+1);//`kkk:'
	auto j=find_if(ns_v().cbegin(),ns_v().cend(),[&](const ns_prefix& n){return n.second==prefix;});
	if(j==ns_v().cend()) return uri();
	uri u;
	u.index=j-ns_v().cbegin();
	strncpy(u.local,s.substr(i+1).c_str(),uri::STR_SIZE-1);
	return u;
}
string uri::ns() const{
	return ns_v()[index].first;
}
bool uri::empty() const{
	return strlen(local)==0;
}
bool uri::is_local() const{
	return index==0;
}
bool uri::operator==(const uri& u) const{
	return index==u.index && strcmp(local,u.local)==0;
}
bool uri::operator!=(const uri& u) const{
	return index!=u.index || strcmp(local,u.local);
}
int uri::compare(const uri& u) const{
	return index==u.index ? (strcmp(local,u.local)) : index-u.index;
}
bool uri::operator<(const uri& u) const{
	return compare(u)<0;
}
uri::uri(const uri& u):index(u.index){
	strcpy(local,u.local);
}
uri& uri::operator=(const uri& u){
	strcpy(local,u.local);
	index=u.index;
	return *this;
}
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
	static vector<ns_prefix>& _ns_v_=ns_v();
	os<<_ns_v_[index].first<<local;
}
string _url_encode(const string& value){
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;
    for(auto c:value){
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			escaped << c;
        else
			// Any other characters are percent-encoded
			escaped << '%' << setw(2) << int((unsigned char) c);
	}
	return escaped.str();
}
//let's only work on local part
uri uri::url_encode(std::string s){
	return uri(_url_encode(s));	
}
string uri::url_decode() const{
	//we could also set a flag when it's encoded to save processing
	//we could also store non valid URI'S (there are shorter) and only encode when needed
	//would be nice if it was transparent to user
	url_decoder::my_handler<const char*> h;
	if(!url_decoder::word::go(local,local+strlen(local),h).first)
		throw std::runtime_error(string("failed URL decode `")+local+"'");
	return h.decoded;
}
namespace objrdf{
	ostream& operator<<(ostream& os,const uri& u){
		static vector<uri::ns_prefix>& _ns_v_=uri::ns_v();
		return os<<_ns_v_[u.index].second<<u.local;
	}
}
