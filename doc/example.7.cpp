#include <objrdf.h>	/* doc/example.7.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
struct s_0{
	int a;
};
struct s_0_w:s_0,resource<rdfs_namespace,objrdf::str<'s','_','0','_','w'>,std::tuple<>,s_0_w>{
	s_0_w(uri u):SELF(u){}
	s_0_w(s_0 s):s_0(s),SELF(uri("")){}
	static void patch(V& _v){
		_v.push_back(property_info::go<property<rdfs_namespace,objrdf::str<'a'>,decltype(a)>>(offsetof(s_0_w,a)));
	}
};
struct s_1{
	int a;
	double b;
	std::string c;
	vector<int> d;
	s_0 e;//nested
};
//define a property for nested structure
typedef property<rdfs_namespace,str<'e'>,local<s_0_w>> e;
struct s_1_w:s_1,resource<rdfs_namespace,objrdf::str<'s','_','1','_','w'>,std::tuple<e>,s_1_w>{
	s_1_w(uri u):SELF(u){};
	s_1_w(s_1 s,uri u):s_1(s),SELF(u){
		//need to copy nested structure to property `e'
		get<::e>().t=s.e;
	};
	static void patch(V& _v){
		_v.push_back(property_info::go<property<rdfs_namespace,objrdf::str<'a'>,decltype(a)>>(offsetof(s_1_w,a)));
		_v.push_back(property_info::go<property<rdfs_namespace,objrdf::str<'b'>,decltype(b)>>(offsetof(s_1_w,b)));
		_v.push_back(property_info::go<property<rdfs_namespace,objrdf::str<'c'>,decltype(c)>>(offsetof(s_1_w,c)));
		//a bit more complicated with vector
		_v.push_back(property_info::go<objrdf::array<property<rdfs_namespace,objrdf::str<'d'>,decltype(d)::value_type>>>(offsetof(s_1_w,d)));
	}
};
int main(){
	s_1 r;
	r.a=12345;
	r.b=.12345;
	r.c="hello from RDF";
	r.d.push_back(1);
	r.d.push_back(1);
	r.d.push_back(1);
	r.e.a=67890;
	//now wrap inside s_1_w
	s_1_w rr(r,uri("rr"));
	to_rdf_xml(&rr,cout);
	to_rdf_xml(&rr.cget<e>().t,cout);
}
