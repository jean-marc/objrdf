/*
 *	code generation, given a RDFS schema this application generates
 *	the C++ code, along with random document generator it could be
 *	very handy for testing and benchmarking
 *	maybe we could had Classes at run time by loading object file
 *	the trick is to output Classes and Properties in the right order
 *	to make compiler happy
 *
 */
#include "objrdf.h"
#include "rdf_xml_parser.h"

using namespace objrdf;
typedef vector<rdfs::Class*>::iterator C_ITERATOR;
typedef vector<rdf::Property*>::iterator P_ITERATOR;
struct has_not_range{
	rdfs::Class* c;
	has_not_range(rdfs::Class* _c){c=_c;}
	bool operator()(rdf::Property* p) const{return p->get<rdfs::domain>().get()!=c;}
};
int main(){
	objrdf::Int::get_class();
	int n_c=rdfs::Class::get_v().size();
	int n_p=rdf::Property::get_v().size();
	rdf::RDF doc;
	rdf_xml_parser p(doc,cin);
	p.go();
	cout<<"/*"<<endl;
	doc.to_turtle(cout);
	cout<<"*/"<<endl;
	map<string,string> m;
	m[_Double]="double";
	m[_Float]="float";
	m[_Int]="int";
	m[_Unsigned_Int]="unsigned int";
	m[_Short]="short";
	m[_Unsigned_Short]="unsigned int";
	m[_Char]="char";
	m[_String]="string";
	cout<<"#include \"objrdf.h\""<<endl;
	cout<<"using namespace objrdf;"<<endl;
	for(P_ITERATOR i=rdf::Property::get_v().begin()+n_p;i<rdf::Property::get_v().end();++i){
		if((*i)->get<rdfs::range>().get()){
			if((*i)->get<rdfs::range>()->literalp()){ 
				cout<<"PROPERTY("<<(*i)->id<<","<<m[(*i)->get<rdfs::range>()->id]<<");"<<endl;
			}else{
				//cout<<"class "<<(*i)->get<rdfs::range>()->id<<";"<<endl;
				cout<<"PROPERTY("<<(*i)->id<<","<<(*i)->get<rdfs::range>()->id<<"*);"<<endl;
			}
		}
	}	
	for(C_ITERATOR i=rdfs::Class::get_v().begin()+n_c;i<rdfs::Class::get_v().end();++i){
		vector<rdf::Property*> my_properties;
		remove_copy_if(rdf::Property::get_v().begin()+n_p,rdf::Property::get_v().end(),back_inserter(my_properties),has_not_range(*i));
		cout<<"CLASS"<<my_properties.size()<<"("<<(*i)->id;
		for(P_ITERATOR i=my_properties.begin();i<my_properties.end();++i) cout<<","<<(*i)->id;
		cout<<");"<<endl;
	}	
	cout<<"int main(){}"<<endl;
}
