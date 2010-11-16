#define TEST_PERF
#include "xml_parser_ns.h"
#include <sstream>
int main(){
	ostringstream os;
	os<<cin.rdbuf();
	cout<<os.str()<<endl;
	//for(;;){
		istringstream is(os.str());
		generic_xml_parser_ns c(is);
		cout<<c.n_read<<endl;
		cout<<c.go()<<endl;
		cout<<c.n_read<<" "<<c.nn<<" "<<*c<<endl;
		cout<<"end of parsing"<<endl;
		cout<<c.ns_list.back().second<<endl;
		cout<<c.depth<<endl;
	//}
}
