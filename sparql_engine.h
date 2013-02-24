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
 * 	we need a garbage collection so then all the properties have been removed from a resource the
 * 	memory should be released, we can decide to do that once the type has been removed
 */
/*
 *	should catch cycles in graph!
 *
 */
using namespace objrdf;
typedef CONST_RESOURCE_PTR SPARQL_RESOURCE_PTR;//won't allow modification (update)
struct subject;
typedef vector<vector<base_resource::const_instance_iterator>> RESULT;
ostream& operator<<(ostream& os,const RESULT& r);
struct verb{
	CONST_PROPERTY_PTR p;//0 -> not bound
	subject* object;
	string name;
	bool is_optional;
	bool is_selected;//will be returned in result set
	bool bound;
	verb(CONST_PROPERTY_PTR p,subject* object,CONST_USER_PTR user);
	verb();
	RESULT run(SPARQL_RESOURCE_PTR r);
	int size();
	vector<string> get_variables() const;
	CONST_USER_PTR user;//who is running the query
};
struct subject{
	SPARQL_RESOURCE_PTR r;//0 -> not bound	
	uri u;//empty not bound
	string s;//empty not bound
	string name;
	bool is_selected;//will be returned in result set
	const bool bound;
	bool is_root;
	bool busy;
	vector<verb> verbs;
	subject(SPARQL_RESOURCE_PTR r=SPARQL_RESOURCE_PTR(0));
	explicit subject(string s);
	subject(uri u);
	RESULT run(base_resource::const_instance_iterator i,CONST_PROPERTY_PTR p);
	RESULT run(size_t n=1000000);
	void del();
	void ins();
	int size();
	vector<string> get_variables();
	/*
 	*	when you run a query you list all the pools used
 	*/ 
	bool is_stale() const;
};



//void to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject&);
//void to_json(ostream& os,const RESULT& r,const subject&){}
class sparql_parser:public char_iterator{
/*
 *	see http://www.w3.org/TR/rdf-sparql-query/#sparqlGrammar
 */
public:
	static void sort(RESULT& r,vector<string> variables,vector<string> order_variables);
	void to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject&);
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
	typedef seq_c<'C','O','U','N','T'> COUNT;
	typedef seq_c<'A','S'> AS;
	typedef seq_c<'O','R','D','E','R'> ORDER;
	typedef seq_c<'B','Y'> BY;
	typedef event_1<seqw<BASE,turtle_parser::_uriref_>,__COUNTER__> base_decl;
	typedef event_1<seqw<PREFIX,turtle_parser::pname_ns,turtle_parser::_uriref_>,__COUNTER__> prefix_decl;
	/*
 	*	how do we add OPTIONAL?
 	*/ 
	typedef	event_1<seqw<WHERE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'>>,__COUNTER__> where_statement; 
	typedef	event_1<seqw<DELETE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'>>,__COUNTER__> delete_statement; 
	typedef	event_1<seqw<INSERT,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'>>,__COUNTER__> insert_statement; 
	typedef event_1<seqw<or_p<delete_statement,true_p>,or_p<insert_statement,true_p>,where_statement>,__COUNTER__> update_query;
	typedef	event_1<seqw<DELETE,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'>>,__COUNTER__> delete_data_query; 
	typedef	event_1<seqw<INSERT,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'>>,__COUNTER__> insert_data_query; 
	//typedef seqw<delete_data_query,char_p<';'>,insert_data_query> update_data_query;
	/*
 	* support for count() statement
 	*	COUNT(?v) AS ?n
 	*
 	*/
	typedef event_1<seqw<ORDER,BY,plus_pw<turtle_parser::variable>>,__COUNTER__> order_by;
	typedef event_1<seqw<COUNT,char_p<'('>,turtle_parser::variable,char_p<')'>,AS,turtle_parser::variable>,__COUNTER__> counter;
	typedef event_1<seqw<SELECT,or_p<plus_pw<or_p<turtle_parser::variable,counter>>,char_p<'*'>>,where_statement,or_p<order_by,true_p>>,__COUNTER__> select_query;	
	typedef event_1<seqw<DESCRIBE,or_p<turtle_parser::_uriref_,turtle_parser::qname>>,__COUNTER__> simple_describe_query;	
	typedef event_1<seqw<DESCRIBE,or_p<plus_pw<turtle_parser::variable>,char_p<'*'>>,where_statement,or_p<order_by,true_p>>,__COUNTER__> describe_query;	
	typedef seqw<
		or_p<base_decl,true_p>,
		kleene_pw<prefix_decl>,
		choice<
			select_query,
			simple_describe_query,
			describe_query,
			insert_data_query,
			seqw<delete_data_query,or_p<seqw<char_p<';'>,insert_data_query>,true_p>>,//see http://www.w3.org/TR/sparql11-update/
  			//update_data_query
			update_query
		>/*,
		char_p<EOF> //to indicate the end
		*/
	> document;
	typedef map<string,base_resource::const_instance_iterator> VARIABLES;
	SPARQL_RESOURCE_PTR d_resource;
	subject *sbj,*current_sbj,*where_s,*delete_s,*insert_s;
	typedef enum{no_q,select_q,simple_describe_q,describe_q,insert_data_q,delete_data_q} query_type;
	query_type q;
	typedef map<string,string> PREFIX_NS;
	typedef set<string> VARIABLE_SET;
	VARIABLE_SET variable_set;
	vector<string> order_by_variables;
	PREFIX_NS prefix_ns;
	//generic_property::PROVENANCE p;
	PROVENANCE p;
	CONST_USER_PTR user;//who is running the query
	sparql_parser(istream& is,PROVENANCE p=2);
	bool go();
	//we can specialize the document
	template<typename T> bool _go(){
		if(!T::go(*this)) return false;
		if(q==select_q||q==describe_q) return sbj;
		return true;
	}
	void out(ostream& os);
	bool callback(PARSE_RES_TREE& r);
	bool parse_where_statement(PARSE_RES_TREE& r);
	bool parse_extra_statement(PARSE_RES_TREE& r);
	CONST_PROPERTY_PTR parse_property(const PARSE_RES_TREE& r);
	//won't parse literal
	SPARQL_RESOURCE_PTR parse_object(const PARSE_RES_TREE& r);
	bool parse_update_data_statement(PARSE_RES_TREE::V::const_iterator begin,PARSE_RES_TREE::V::const_iterator end,bool do_delete=false,VARIABLES v=VARIABLES(),RESOURCE_PTR sub=RESOURCE_PTR());
	bool parse_update_data_statement(const PARSE_RES_TREE& r,bool do_delete=false,VARIABLES v=VARIABLES(),RESOURCE_PTR sub=RESOURCE_PTR());
	template<typename T> void callback(T,string){}
};
#endif
