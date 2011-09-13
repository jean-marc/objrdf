#ifndef SPARQL_ENGINE_H
#define SPARQL_ENGINE_H
#include <set>
#include <sstream>
#include "turtle_parser.h"
#include "uri.h"
#include "objrdf.h"
#define LOG cerr
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
 */
/*
 *	should catch cycles in graph!
 *
 */
using namespace objrdf;
struct match_property{
	const rdf::Property* p;
	match_property(const rdf::Property* _p):p(_p){};
	bool operator()(generic_property* g)const{return g->p.get()==p;}
};
struct subject;
typedef vector<vector<base_resource::instance_iterator> > RESULT;
ostream& operator<<(ostream& os,const RESULT& r){
	for(auto i=r.begin();i<r.end();++i){
		for(auto j=i->begin();j<i->end();++j){
			if(j->get_Property()->get<rdfs::range>()=rdfs::XML_Literal::get_class())
				os<<"XML_Literal ...\t|";
			else
				os<<*j<<"\t|";
		}
	}
	return os;
}
struct verb{
	shared_ptr<rdf::Property> p;//0 -> not bound
	subject* object;
	string name;
	bool is_optional;
	bool is_selected;//will be returned in result set
	bool bound;
	verb(shared_ptr<rdf::Property> p,subject* object);
	verb();
	RESULT run(base_resource* r);
	int size();
	vector<string> get_variables() const;
};
struct subject{
	base_resource* r;//0 -> not bound	
	string s;//empty not bound
	string name;
	bool is_selected;//will be returned in result set
	const bool bound;
	bool is_root;
	bool busy;
	vector<verb> verbs;
	subject(base_resource* r=0);
	subject(string s);
	RESULT run(base_resource::instance_iterator i);
	RESULT run(rdf::RDF& doc,size_t n=1000);
	void del();
	void ins();
	int size();
	vector<string> get_variables() const;
};
void to_xml(ostream& os,const RESULT& r,const subject&);
void to_json(ostream& os,const RESULT& r,const subject&){}
class sparql_parser:public char_iterator{
/*
 *	see http://www.w3.org/TR/rdf-sparql-query/#sparqlGrammar
 */
public:
	enum{CASE_INSENSITIVE=true};
	typedef map<string,subject*> INDEX;
	INDEX index;
	typedef seq_c<'P','R','E','F','I','X'> PREFIX;
	typedef seq_c<'S','E','L','E','C','T'> SELECT;
	typedef seq_c<'I','N','S','E','R','T'> INSERT;	
	typedef seq_c<'D','E','L','E','T','E'> DELETE;	
	typedef seq_c<'D','A','T','A'> DATA;
	typedef seq_c<'W','H','E','R','E'> WHERE;		
	typedef seq_c<'B','A','S','E'> BASE;		
	typedef seq_c<'D','E','S','C','R','I','B','E'> DESCRIBE;
	typedef event_1<seqw<BASE,turtle_parser::_uriref_>,__COUNTER__> base_decl;
	typedef event_1<seqw<PREFIX,turtle_parser::pname_ns,turtle_parser::_uriref_>,__COUNTER__> prefix_decl;
	typedef	event_1<seqw<WHERE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> where_statement; 
	//typedef	event_1<seqw<DELETE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> delete_statement; 
	//typedef	event_1<seqw<INSERT,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> insert_statement; 
	typedef	event_1<seqw<DELETE,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> delete_data_query; 
	typedef	event_1<seqw<INSERT,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> insert_data_query; 
	typedef event_1<seqw<SELECT,or_p<plus_pw<turtle_parser::variable>,char_p<'*'> >,where_statement>,__COUNTER__> select_query;	
	typedef event_1<seqw<DESCRIBE,or_p<turtle_parser::_uriref_,turtle_parser::qname> >,__COUNTER__> simple_describe_query;	
	typedef event_1<seqw<DESCRIBE,or_p<plus_pw<turtle_parser::variable>,char_p<'*'> >,where_statement>,__COUNTER__> describe_query;	
	//typedef seqw<or_p<delete_statement,true_p>,or_p<insert_statement,true_p>,where_statement> update_query;
	typedef seqw<
		or_p<base_decl,true_p>,
		kleene_pw<prefix_decl>,
		choice<
			select_query,
			simple_describe_query,
			describe_query,
			insert_data_query,
			delete_data_query
			//,update_query,
  			//update_data_query
		> 
	> document;
	rdf::RDF& doc;
	shared_ptr<base_resource> d_resource;
	subject *sbj,*current_sbj,*where_s,*delete_s,*insert_s;
	typedef enum{no_q,select_q,simple_describe_q,describe_q,insert_data_q,delete_data_q} query_type;
	query_type q;
	typedef map<string,string> PREFIX_NS;
	typedef set<string> VARIABLE_SET;
	VARIABLE_SET variable_set;
	PREFIX_NS prefix_ns;
	generic_property::PROVENANCE p;
	sparql_parser(rdf::RDF& doc,istream& is,generic_property::PROVENANCE p=2);
	bool go();
	void out(ostream& os);
	bool callback(PARSE_RES_TREE& r);
	bool parse_where_statement(PARSE_RES_TREE& r);
	shared_ptr<rdf::Property> parse_property(PARSE_RES_TREE& r);
	//won't parse literal
	shared_ptr<base_resource> parse_object(PARSE_RES_TREE& r);
	bool parse_update_data_statement(PARSE_RES_TREE& r,bool do_delete=false);
	template<typename T> void callback(T,string){}
};
#endif
