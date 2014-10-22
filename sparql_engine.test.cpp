#include "sparql_engine.h"
#include "objrdf.h"
#include <fstream>
namespace test{
	

}
int main(int argc,char* argv[]){
	//to_rdf_xml(cout);
 	//cin.exceptions(iostream::eofbit);
	sparql_parser sp(std::cin);
	bool r=sp.go();
	if(r){
		cerr<<"parsing success"<<endl;
		sp.out(cout);
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
	//doc.to_rdf_xml_pretty(cout);
	return 0;
}
