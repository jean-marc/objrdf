#include <thread>
#include "httpd.h"
#include "objrdf.h"

int main(){
	objrdf::httpd h(8080);
	auto t=std::thread(&objrdf::httpd::run,&h);
	t.join();
};
