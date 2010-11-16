#include "httpd.h"
#include "objrdf.h"

int main(){
	rdf::RDF doc;
	objrdf::httpd h(doc);
	h.run();
};
