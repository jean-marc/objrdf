#include "objrdf.h"
#include "obj_xml_parser.h"
#include "sigmon.h"
using namespace objrdf;
using namespace sigmon;
int main(){
	sigmon_message r;
	obj_xml_parser p(&r);
	cin>>p;
	r.to_xml(cout);
};
