#ifndef SPARQL_PARSER_H
#define SPARQL_PARSER_H
class sparql_parser:public char_iterator{
/*
 *	see http://www.w3.org/TR/rdf-sparql-query/#sparqlGrammar
 */
public:
	static void sort(RESULT& r,vector<string> variables,vector<string> order_variables);
	void to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject&);
	void to_csv(ostream& os,/*const*/ RESULT& r,/*const*/ subject&);
	enum{CASE_INSENSITIVE=true};
	typedef map<string,SUBJECT_PTR> INDEX;
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
	typedef seq_c<'L','I','M','I','T'> LIMIT;
	typedef seq_c<'O','F','F','S','E','T'> OFFSET;
	typedef seq_c<'F','I','L','T','E','R'> FILTER;
	typedef event_1<seqw<BASE,turtle_parser::_uriref_>,__COUNTER__> base_decl;
	typedef event_1<seqw<PREFIX,turtle_parser::pname_ns,turtle_parser::_uriref_>,__COUNTER__> prefix_decl;
	typedef event_1<choice<turtle_parser::decimal,turtle_parser::sparql_variable>,__COUNTER__> NumericExpression;
	typedef event_1<char_p<'='>,__COUNTER__> equal;
	typedef event_1<seq_c<'!','='>,__COUNTER__> different;
	typedef event_1<char_p<'<'>,__COUNTER__> less;
	typedef event_1<char_p<'>'>,__COUNTER__> greater;
	typedef event_1<seq_c<'<','='>,__COUNTER__> less_eq;
	typedef event_1<seq_c<'>','='>,__COUNTER__> greater_eq;
	typedef event_1<
			seqw<
				NumericExpression,
				choice<
					seqw<equal,NumericExpression>,
					seqw<different,NumericExpression>,
					seqw<less,NumericExpression>,
					seqw<greater,NumericExpression>,
					seqw<less_eq,NumericExpression>,
					seqw<greater_eq,NumericExpression>
				>
			>,
			__COUNTER__
	> RelationalExpression;
	typedef event_1<seqw<FILTER,char_p<'('>,RelationalExpression,char_p<')'>>,__COUNTER__> filter;
	//let's use our own document so we can interleave FILTER statements
	//typedef plus_p<statement> turtle_doc;
	typedef plus_p<or_p<turtle_parser::statement,filter>> body;
	typedef	event_1<seqw<WHERE,char_p<'{'>,body,char_p<'}'>>,__COUNTER__> where_statement; 
	typedef	event_1<seqw<DELETE,char_p<'{'>,body,char_p<'}'>>,__COUNTER__> delete_statement; 
	typedef	event_1<seqw<INSERT,char_p<'{'>,body,char_p<'}'>>,__COUNTER__> insert_statement; 
	typedef event_1<seqw<or_p<delete_statement,true_p>,or_p<insert_statement,true_p>,where_statement>,__COUNTER__> update_query;
	typedef	event_1<seqw<DELETE,DATA,char_p<'{'>,body,char_p<'}'>>,__COUNTER__> delete_data_query; 
	typedef	event_1<seqw<INSERT,DATA,char_p<'{'>,body,char_p<'}'>>,__COUNTER__> insert_data_query; 
	//typedef seqw<delete_data_query,char_p<';'>,insert_data_query> update_data_query;
	/*
 	* support for count() statement
 	*	COUNT(?v) AS ?n
 	*
 	*/
	typedef event_1<seqw<ORDER,BY,plus_pw<turtle_parser::sparql_variable>>,__COUNTER__> order_by;
	typedef event_1<plus_p<range_p<'0','9'>>,__COUNTER__> _int_;
	typedef event_1<seqw<LIMIT,_int_>,__COUNTER__> limit;
	size_t _limit=100;//default number of solutions
	typedef event_1<seqw<COUNT,char_p<'('>,turtle_parser::sparql_variable,char_p<')'>,AS,turtle_parser::sparql_variable>,__COUNTER__> counter;
	typedef event_1<seqw<SELECT,or_p<plus_pw<or_p<turtle_parser::sparql_variable,counter>>,char_p<'*'>>,
			where_statement,
			or_p<order_by,true_p>,
			or_p<limit,true_p>
			>,__COUNTER__> select_query;	
	typedef event_1<seqw<DESCRIBE,or_p<turtle_parser::_uriref_,turtle_parser::qname>>,__COUNTER__> simple_describe_query;	
	typedef event_1<seqw<DESCRIBE,or_p<plus_pw<turtle_parser::sparql_variable>,char_p<'*'>>,
			where_statement,
			or_p<order_by,true_p>,
			or_p<limit,true_p> //is this going to blow everything up?
			>,__COUNTER__> describe_query;	
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
	SPARQL_RESOURCE_PTR d_resource,bnode_subject_resource;
	SUBJECT_PTR sbj,current_sbj,where_s,delete_s,insert_s;
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
		if(q==select_q||q==describe_q) return sbj.get();
		return true;
	}
	void out(ostream& os);
	void out_csv(ostream& os);
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
