#include "objrdf.h"
#include "rdf_xml_parser.h"
using namespace objrdf;
char _p_0[]="p_0";
struct p_0:property<rdfs_namespace,_p_0,int>{};
CLASS1(C,p_0);
namespace objrdf{
	//not very good idea
	/*
	template<> class _property_<C,p_0>:public generic_property{
	public:
		virtual void out(base_resource* subject,void* t,ostream& os){
			os<<"~~~~";
		}
	};
	*/
}
int main(){
	rdf::RDF doc;
	doc.insert(C::get_class());
	doc.insert(p_0::get_property());
	rdf_xml_parser p(doc,std::cin);
	std::cout<<p.go()<<std::endl;
	doc.to_rdf_xml_pretty(std::cout);
	cout<<endl;
}
