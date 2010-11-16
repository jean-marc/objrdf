#include "objrdf.h"
using namespace objrdf;

PROPERTY(p,base_resource*);
CLASS1(C,p_array<p>);
int main(){
	C c;
	c.id="a_C";
	p s(new base_resource(uri("qqq")));
	cout<<s->id<<endl;
	p ss=s;
	cout<<ss->id<<endl;
	c.get<p_array<p> >().push_back(s);
	c.get<p_array<p> >().push_back(p(new base_resource(uri("0"))));
	c.get<p_array<p> >().push_back(p(new base_resource(uri("1"))));
	c.get<p_array<p> >().push_back(p(new base_resource(uri("2"))));
	cout<<&c.get<p_array<p> >()<<endl;
	cout<<&c.get<p_array<p> >()[0]<<"\t"<<c.get<p_array<p> >()[0].get()<<"\t"<<c.get<p_array<p> >()[0]->id<<endl;
	cout<<&c.get<p_array<p> >()[1]<<"\t"<<c.get<p_array<p> >()[1].get()<<"\t"<<c.get<p_array<p> >()[1]->id<<endl;
	for(base_resource::type_iterator i=c.begin();i!=c.end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			cout<<i->current()<<"\t"<<j->current()<<"\t"<<j->get_Property()->id<<"\t"<<j->get_object()->id<<endl;
		}
	}
	c.to_turtle_pretty(std::cout);
	c.to_rdf_xml_pretty(std::cout);
	cout<<endl;
}
