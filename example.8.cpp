#include "objrdf.h"
using namespace objrdf;

PROPERTY(p,base_resource*);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,p_array<p>,C>{
	void set(p _p){
		get<p_array<p> >().back()=_p;	
	}
};

int main(){
	C c;
	c.get<p_array<p> >().push_back(p(new base_resource(uri("0"))));
	c.get<p_array<p> >().push_back(p(new base_resource(uri("1"))));
	cout<<&c.get<p_array<p> >()<<endl;
	cout<<&c.get<p_array<p> >()[0]<<"\t"<<c.get<p_array<p> >()[0].get()<<"\t"<<c.get<p_array<p> >()[0]->id<<endl;
	cout<<&c.get<p_array<p> >()[1]<<"\t"<<c.get<p_array<p> >()[1].get()<<"\t"<<c.get<p_array<p> >()[1]->id<<endl;
	for(base_resource::type_iterator i=c.begin();i!=c.end();++i){
		for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
			//os<<((j==i->begin()) ? s+i->get_Property()->id.name : ",")<<" "<<*j;
			cout<<i->current()<<"\t"<<j->current()<<"\t"<<j->get_Property()->id<<"\t"<<j->get_object()->id<<endl;
		}
	}
	c.to_turtle(std::cout);
}
