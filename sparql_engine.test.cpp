#include "sparql_engine.h"
#include "rdf_xml_parser.h"
#include "objrdf.h"
#include <fstream>

RDFS_NAMESPACE("http://www.example.org/#","ex");
struct C;
PROPERTY(p_0,C*);
PROPERTY(p_1,int);
PROPERTY(p_2,int);
char _C[]="C";
struct C:resource<rdfs_namespace,_C,objrdf::tuple<p_array<p_0>,p_1,p_array<p_2> >::SELF>{
	C(uri u):SELF(u){}
};
int main(int argc,char* argv[]){
	//cout<<typeid(turtle_parser::literal).name()<<endl;
	rdf::RDF doc;
	doc.insert(C::get_class());
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		cerr<<"parsing `"<<argv[i]<<"'";
		rdf_xml_parser p(doc,in);
		cerr<<"\t"<<p.go()<<endl;
	}
	doc.to_rdf_xml_pretty(cout);
	cerr<<"bye"<<endl;
	return 0;
	//sparql parser
 	cin.exceptions(iostream::eofbit);
	sparql_parser sp(doc,std::cin);
	bool r=sp.go();
	if(r/* && sp.sbj*/){
		cerr<<"parsing success"<<endl;
		if(sp.sbj) sp.sbj->run(doc);
	}else{
		cerr<<"parsing failure"<<endl;
	}
	doc.to_rdf_xml_pretty(cout);
	exit(0);	
	cout<<endl;
	//query
	//subject s=subject::unbound("$s");
	//subject s(doc.query(uri("_2")).get());
	subject s(0);
	//subject s=subject::bound(base_resource::nil.get());
	//subject s=subject::bound(rdfs::Class::get_class());
	//subject* o=new subject;
	//s.verbs.push_back(verb(p_1::get_property(),new subject()));
	//s.verbs.push_back(verb(0,new subject()));
	//s.verbs.push_back(verb(0,new subject(rdfs::Class::get_class())));
	//s.verbs.push_back(verb(rdfs::subClassOf::get_property(),new subject()));
	//s.verbs.push_back(verb(objrdf::c_index::get_property(),new subject("2")));
	s.verbs.push_back(verb(p_0::get_property(),new subject()));
	s.verbs.back().object->verbs.push_back(verb(p_2::get_property(),new subject()));
	s.verbs.push_back(verb(p_1::get_property(),new subject()));
	s.verbs.push_back(verb(p_2::get_property(),new subject()));
	//s.verbs.push_back(verb(0,&s));//loop  SELECT $s WHERE{$s $p $s.}
	//subject s_1(0);
	//s_1.verbs.push_back(verb(0,&s));//loop  SELECT $s WHERE{$s $p $s.}
	//s.verbs.push_back(verb(0,&s_1));//loop  SELECT $s WHERE{$s $p $s.}
	//s.run(doc);	
	//handle h(c);
	//RES tmp(true,base_resource::instance_iterator::help());
	//cout<<tmp.second<<endl;
	cout<<"new query:"<<s.run(doc)<<endl;
	//cout<<"\t"<<s.bound<<"\t"<<s.verbs[0].o->result.size()<<"\t"<<s.get_binding_size()<<endl;
	cout<<endl;
	cout<<"size:"<<s.size()<<endl;
	//cout<<s.result.size()<<endl;
	//cout<<s.verbs[0].result.size()<<endl;
	//cout.width(20);
	//s.print();
	/*
	sparql_parser in(cin);
	cout<<in.n_read<<endl;
	cout<<in.go()<<endl;	
	cout<<in.n_read<<endl;
	exit(1);
	*/
	//RESULT r(1);
	//cout<<r<<endl;
}
