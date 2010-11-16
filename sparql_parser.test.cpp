#include "sparql_parser.h"


int main(){
	rdf::RDF doc;
	sparql_parser c(doc,cin);
	cout<<c.n_read<<endl;
	cout<<"parsing result: "<<c.go()<<endl;
	cout<<c.n_read<<endl;
	c.out(cout);
	//cout<<*c.get_root()<<endl;
	//rdfs::Class::c.second->_set<rdfs::comment>("###");
	doc.to_rdf_xml(cout);
	cout<<"\n";
}
