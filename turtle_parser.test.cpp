#include "turtle_parser.h"
#include <string.h>


int main(int argc,char* argv[]){
	if(argc==1) exit(1);
	cout<<argv[1]<<endl;
	auto r=get_range(argv[1],argv[1]+strlen(argv[1]));
	cout<<event<turtle_parser::doc>::go(r)<<endl;
	//cout<<event<turtle_parser::_uriref_>::go(r)<<endl;
}
