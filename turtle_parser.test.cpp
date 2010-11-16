#include "turtle_parser.h"


int main(){
	turtle_parser c(cin);
	cout<<c.n_read<<endl;
	cout<<"parsing result: "<<c.go()<<endl;
	cout<<c.n_read<<endl;
}
