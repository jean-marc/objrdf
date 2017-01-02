#include <thread>
#include "httpd.h"
#include "objrdf.h"

namespace rdfs{
	int verbosity=7;
	const char _context_[]="rdfs";
}
namespace objrdf{
	int verbosity=7;
	const char _context_[]="objrdf";
}
namespace pool_allocator{
	int verbosity=7;
	const char _context_[]="pool";
}

int main(){
	objrdf::generate_index();
	objrdf::httpd h(8080);
	auto t=std::thread(&objrdf::httpd::run,&h);
	t.join();
};
