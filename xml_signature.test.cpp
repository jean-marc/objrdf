#include "xml_signature.h"



int main(){
	pretty_xml<xml_signature> x(cin);
	//cout<<x.n_read<<endl;
	cout<<x.go()<<endl;
	//cout<<x.n_read<<endl;

}
