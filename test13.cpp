#include "df.h"
#include "rdf_xml_parser.h"
#include <fstream>
using namespace objrdf;
using namespace std;
int main(int argc,char* argv[]){
	df::Antenna::get_class(); //does not load if not here
	rdf::RDF doc;
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		rdf_xml_parser p(doc,in);
		p.go();
	}	
	doc.to_rdf_xml_pretty(cout);
}
