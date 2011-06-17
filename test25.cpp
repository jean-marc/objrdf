/*
 *	a simple schema to handle license information
 */
#include "objrdf.h"
#include "hex_format.h"
#include "rdf_xml_parser.h"
#include <fstream>
using namespace objrdf;
//.h
CLASS0(Product);
PROPERTY(product,Product*);
PROPERTY(key,string);
PROPERTY(issue_date,string);
PROPERTY(expiry_date,string);
PROPERTY(feature_code,hex_format<unsigned int>);//32 bit?
CLASS0(Feature);
PROPERTY(feature,Feature*);
CLASS6(License,product,p_array<key>,issue_date,expiry_date,feature_code,p_array<feature>);

int main(int argc,char* argv[]){
	cout<<Product::v.size()<<endl;
	Product::get_class()->to_rdf_xml_pretty(cout);
	Product p;
	p.to_turtle_pretty(cout);
	rdf::RDF doc;
	//doc.insert(License::get_class());
	//doc.insert(Product::get_class());
	//doc.insert(key::get_property());
	//doc.insert(issue_date::get_property());
	//doc.insert(expiry_date::get_property());
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		if(in.good()){
			rdf_xml_parser p(doc,in);
			cout<<"parsing "<<argv[i]<<" :"<<p.go()<<endl;
		}else{
			cerr<<"can't read file"<<endl;
			exit(1);
		}
	}	
	//query by type
	shared_ptr<License> l=doc.query_by_type<License>();
	if(l.get()){
		cout<<"the license is valid until: "<<l->get<expiry_date>().t<<endl;	
		//we can modify the license eg: add a new feature
		l->get<p_array<feature> >().push_back(new Feature);
		//serialize only this part of the document
		cout<<endl;
		l->to_xml(cout);
		cout<<endl;
	}
	doc.to_rdf_xml_pretty(cout);
	//doc.to_xml(cout);
	cout<<endl;
}
