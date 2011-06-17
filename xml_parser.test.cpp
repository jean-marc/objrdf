#include "xml_parser.h"
using namespace std;
int main(){
	//generic_xml_parser<xml_parser> in(cin);
	//quiet<xml_parser> in(cin);
	pretty_xml<xml_parser> in(cin);
	cout<<in.n_read<<" depth:"<<in.depth<<endl;
	cout<<in.go()<<endl;	
	//cout<<pretty_xml<xml_parser>::doctype::go(in)<<endl;
	cout<<in.n_read<<" depth:"<<in.depth<<endl;
}

