#ifndef SPARQL_ENGINE_H
#define SPARQL_ENGINE_H
#include "turtle_parser.h"
#include "uri.h"
#include "objrdf.h"
#include "result.h"
/*
 * 	http://www.w3.org/TR/rdf-sparql-query/
 *
 *	PREFIX foaf:   <http://xmlns.com/foaf/0.1/>
 *	SELECT ?name ?mbox
 *	WHERE
 *	  { ?x foaf:name ?name .
 *	      ?x foaf:mbox ?mbox }
 *
 *	all variables should have same number of bindings, what if one of the statement is optional?
 *	we need to know where the root is
 *
 * 	    
 *	http://www.w3.org/TR/sparql11-update/
 *
 *	INSERT DATA { graph_triples }
 *	DELETE DATA { graph_triples }
 *
 *	PREFIX foaf:  <http://xmlns.com/foaf/0.1/>
 *	WITH <http://example/addresses>
 *	DELETE { ?person foaf:firstName 'Bill' }
 *	INSERT { ?person foaf:firstName 'William' }
 *	WHERE { 
 *		?person a foaf:Person .
 *	        ?person foaf:firstName 'Bill'
 *	  } 
 *
 *	how to represent the different subgraphs:
 *
 *	s v o
 *	(o_0 (p_0 (o_1 ...) (p_1 (o_2 ..)) ...))	
 *
 *	we could have result base_resource*  
 *
 */
using namespace objrdf;
struct match_property{
	const rdf::Property* p;
	match_property(const rdf::Property* _p):p(_p){};
	bool operator()(generic_property* g)const{return g->p.get()==p;}
};
//we can use empty set for false but what about true?
//we could use a special set: (nil)
//enum binding to enum{
typedef std::pair<base_resource::instance_iterator,bool> binding;
ostream& operator<<(ostream& os,binding& b){
	if(b.first==base_resource::nil->begin()->begin()) return os<<"@";
	return b.second ? os<<b.first : os<<b.first->get_Property()->id;
}

typedef vector<vector<base_resource::instance_iterator> > RESULT;
struct subject;
//typedef std::pair<bool,result<base_resource::instance_iterator> > RES;
typedef std::pair<bool,result<binding> > RES;
void print_subject(result<binding>&);
void print_verb(result<binding>&,bool);
//vector<vector<binding> > to_v(result<binding>&,int n);
result<binding> normalize(result<binding>&);
typedef vector<vector<base_resource::instance_iterator> > RES_1;
ostream& operator<<(ostream& os,RES_1& r){
	for(auto i=r.begin();i<r.end();++i){
		for(auto j=i->begin();j<i->end();++j){
			os<<*j<<"\t";
		}
		os<<endl;
	}
	return os;
}
struct verb{
	bool is_optional;
	bool is_selected;//will be returned in result set
	string name;
	//we need to use an iterator instead
	//we can get it from RDF::doc but a bit awkward
	//but we know which one is a property
	vector<rdf::Property*> result;
	rdf::Property* p;//0 -> not bound
	bool bound;
	subject* o;
	verb(rdf::Property* p,subject* o);
	verb();
	bool run(base_resource* r);
	RESULT _run(base_resource* r);	
	RES run_0(base_resource* r);		
	RES_1 run_1(base_resource* r);
	void print(unsigned int index);
	void _print();
	int size();
	static int last_binding;
	int get_binding_size();
};
struct subject{
	/*
 	*	can be bound 
 	*/ 
	bool is_root;
	bool is_selected;//will be returned in result set
	string name;
	vector<base_resource::instance_iterator> result;
	base_resource* r;//0 -> not bound	
	string s;//empty not bound
	bool busy;
	bool root;
	const bool bound;
	vector<verb> verbs;
	//static subject bound(base_resource*);
	//static subject bound(string);
	//static subject unbound(string);
	subject(base_resource* r=0);
	subject(string s);
	bool run(base_resource::instance_iterator i);
	RESULT _run(base_resource::instance_iterator i);
	RES run_0(base_resource::instance_iterator i);		
	RES_1 run_1(base_resource::instance_iterator i);
	bool run(rdf::RDF& doc,int n=1000);
	void print(unsigned int index);
	void print();
	void _print();
	void normalize();//ensure all results are same size
	int size();
	int get_binding_size();
};

/*
struct sparql_parser:char_iterator{
	typedef seq_c<'P','R','E','F','I','X'> prefix_s;
	typedef seq_c<'S','E','L','E','C','T'> select_s;
	typedef seq_c<'I','N','S','E','R','T'> insert_s;	
	typedef seq_c<'D','E','L','E','T','E'> delete_s;	
	typedef seq_c<'D','A','T','A'> data_s;
	typedef seq_c<'W','H','E','R','E'> where_s;		
	typedef	seq<where_s,white_space,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> > where_statement; 
	typedef seq<event<select_s>,white_space,char_p<'*'>,white_space,where_statement> select_query;	
	subject sbj,*current_sbj;
	sparql_parser(istream& is):char_iterator(is),current_sbj(0){}
	bool go(){return select_query::go(*this);}
	typedef map<string,subject*> INDEX;
	INDEX index;
	void callback(turtle_parser::subject,string s){
		cerr<<s<<endl;
		if(s[0]=='$'||s[0]=='?'){
			INDEX::iterator i=index.find(s);
			if(i!=index.end()){
				current_sbj=i->second;
			}else{
				sbj=subject();
				sbj.name=s;
				index[s]=sbj;
			}	
		}else{//look up

		}
	}
	void callback(turtle_parser::verb,string s){cerr<<"\t"<<s<<endl;}
	void callback(event<turtle_parser::object>,string s){cerr<<"\t\t"<<s<<endl;}
	void callback(event<turtle_parser::scharacter>,string s){cerr<<"string:\""<<s<<"\""<<endl;}
	void callback(event<select_s>,string s){}
	

};
*/
#endif
