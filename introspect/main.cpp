/*
	we are trying to isolate the introspection step so
	that the symbol for get_class() and get_property() are in their
	own object file
*/
#include "test.h"
#include "objrdf.h"
//#include "introspect.h"
/*
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<>> Test;
*/
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
using namespace objrdf;
using namespace test;
int main(){
	go();
	//are those function defined
	cerr<<introspect<Test>::get_class()->id<<endl;
	cerr<<introspect<Testd>::get_class()->id<<endl;
	//not symbol available with gcc, not sure why but maybe not big deal
	//cerr<<introspect<p_0>::get_property()->id<<endl;
	//cerr<<introspect<p_1>::get_property()->id<<endl;
	cerr<<introspect<xsd::Double>::get_class()->id<<endl;
	//who is responsible for populating vtable?
	//cerr<<Test::v.size()<<endl;
	to_rdf_xml(introspect<Test>::get_class(),cerr);
	//to_rdf_xml(introspect<p_0>::get_property()->cget<rdfs::range>(),cerr);
	//to_rdf_xml(introspect<p_1>::get_property(),cerr);
	Test::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,uri{"hi"});
	to_rdf_xml(p,cerr);
	p->go();

}

