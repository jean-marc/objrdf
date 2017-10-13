#include "test.h"
namespace test{
	void go(){
		cerr<<introspect<Testd>::get_class()->id<<"\n";
	}
}
