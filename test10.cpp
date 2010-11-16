#include "wbdf_message.h"
#include <iostream>
#include <sstream>
using namespace sigmon_wbdf;
using namespace std;

int main(){
	sigmon_wbdf_message request;
	request.get<p_df_bearing_request>()=new df_bearing_request;
	request.get<p_df_bearing_return>()=new df_bearing_return;
	request.to_xml(cout);
	rdf::RDF doc;
	doc.to_rdf_xml(cout);
	ostringstream os;
	os<<cin.rdbuf();
	cout<<os.str()<<endl;
	{
	//for(;;){
		sigmon_wbdf_message request;
		istringstream is(os.str());
		obj_xml_parser p(&request,is);
		p.go();
		request.to_xml(cout);
		//sigmon_wbdf_message reply;
		//reply.get<p_df_bearing_return>().set(new df_bearing_return);
		//reply.to_xml(cout);
		
	}
	//cin>>s;
	//s.get<p_df_bearing_request>().set(new df_bearing_request);
	//s.to_xml(cout);
}
