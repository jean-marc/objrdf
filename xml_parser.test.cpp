#include "xml_parser.h"
using namespace std;
int main(){
	/*
	generic_xml_parser<xml_parser> in(cin);
	cout<<in.n_read<<endl;
	cout<<generic_xml_parser<xml_parser>::element::go(in)<<endl;	
	cout<<in.n_read<<endl;
	*/
	//xml_xml<xml_parser> x(cin);
	pretty_xml<xml_parser> x(cin);
	cerr<<x.n_read<<endl;
	cerr<<x.go()<<endl;
	cerr<<x.n_read<<endl;
}

