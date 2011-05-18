#ifndef SPARQL_ENGINE_H
#define SPARQL_ENGINE_H
#include <set>
#include "turtle_parser.h"
#include "uri.h"
#include "objrdf.h"
//#include "result.h"
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
using namespace objrdf;
struct match_property{
	const rdf::Property* p;
	match_property(const rdf::Property* _p):p(_p){};
	bool operator()(generic_property* g)const{return g->p.get()==p;}
};
struct subject;
typedef vector<vector<base_resource::instance_iterator> > RESULT;
ostream& operator<<(ostream& os,RESULT& r){
	for(auto i=r.begin();i<r.end();++i){
		for(auto j=i->begin();j<i->end();++j){
			os<<*j<<"\t|";
		}
		os<<endl;
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
	bool run(rdf::RDF& doc,int n=1000);
	void del();
	void ins();
	int size();
};

class sparql_parser:public char_iterator{
/*
 *	see http://www.w3.org/TR/rdf-sparql-query/#sparqlGrammar
 *
 */
public:
	typedef map<string,subject*> INDEX;
	INDEX index;
	typedef seq_c<'P','R','E','F','I','X'> PREFIX;
	typedef seq_c<'S','E','L','E','C','T'> SELECT;
	typedef seq_c<'I','N','S','E','R','T'> INSERT;	
	typedef seq_c<'D','E','L','E','T','E'> DELETE;	
	typedef seq_c<'D','A','T','A'> DATA;
	typedef seq_c<'W','H','E','R','E'> WHERE;		
	typedef seq_c<'B','A','S','E'> BASE;		
	typedef event_1<seqw<BASE,turtle_parser::_uriref_>,__COUNTER__> base_decl;
	typedef event_1<seqw<PREFIX,turtle_parser::pname_ns,turtle_parser::_uriref_>,__COUNTER__> prefix_decl;
	typedef	event_1<seqw<WHERE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> where_statement; 
	typedef	event_1<seqw<DELETE,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> delete_statement; 
	typedef	event_1<seqw<INSERT,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> insert_statement; 
	typedef	event_1<seqw<DELETE,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> delete_data_statement; 
	typedef	event_1<seqw<INSERT,DATA,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> >,__COUNTER__> insert_data_statement; 
	typedef seqw<SELECT,or_p<plus_pw<turtle_parser::variable>,char_p<'*'> >,where_statement> select_query;	
	typedef seqw<or_p<delete_statement,true_p>,or_p<insert_statement,true_p>,where_statement> update_query;
	typedef seqw<or_p<delete_data_statement,true_p>,or_p<insert_data_statement,true_p> > update_data_query;
	//construct_query, describe_query, ask_query
	typedef seqw<or_p<base_decl,true_p>,kleene_pw<prefix_decl>,choice<select_query,update_query,update_data_query> > document;
	rdf::RDF& doc;
	subject *sbj,*current_sbj,*where_s,*delete_s,*insert_s;
	sparql_parser(rdf::RDF& doc,istream& is):char_iterator(is),doc(doc),sbj(0),current_sbj(0){}
	bool go(){
		if(document::go(*this))
			return true;
		sbj=0;
		return false;
	;}
	template<typename T> void callback(T,string){}
	typedef map<string,string> PREFIX_NS;
	typedef set<string> VARIABLE_SET;
	VARIABLE_SET variable_set;
	PREFIX_NS prefix_ns;
	bool callback(PARSE_RES_TREE& r){
		cout<<r<<endl;
		switch(r.t.first){
			case prefix_decl::id:
				prefix_ns[r.v[0].t.second]=r.v[1].t.second;	
			break;
			case turtle_parser::variable::id:
				variable_set.insert(r.t.second);
			break;
			case delete_data_statement::id://need to process where statement first
			break;
			case insert_data_statement::id://need to process where statement first
				return parse_insert_data_statement(r);
			break;
			case delete_statement::id://need to process where statement first
			break;
			case insert_statement::id://need to process where statement first
			break;
			case where_statement::id:
				return parse_where_statement(r);
			break;
		}
		return true;
	}
	bool parse_where_statement(PARSE_RES_TREE& r){
		for(auto i=r.v.begin();i<r.v.end();++i){
			//cerr<<"current:\n"<<*i<<endl;
			switch(i->t.first){
				case turtle_parser::subject::id:{
					switch(i->v[0].t.first){
						case turtle_parser::variable::id:{
							INDEX::iterator j=index.find(i->v[0].t.second);
							if(j!=index.end()){
								current_sbj=j->second;
							}else{
								sbj=new subject();
								sbj->name=i->v[0].t.second;
								sbj->is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
								index[i->v[0].t.second]=sbj;
								current_sbj=sbj;
							}	
						}
						break;
						case turtle_parser::uriref::id:{
							shared_ptr<base_resource> r=doc.query(uri::hash_uri(i->v[0].t.second));
							sbj=new subject(r.get() ? r.get() : base_resource::nil.get());
							index[i->v[0].t.second]=sbj;
							current_sbj=sbj;
						}
						break;
						case turtle_parser::qname::id:{
							PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
							if(j!=prefix_ns.end()){
								uri u(j->second,i->v[0].v[1].t.second);
								shared_ptr<base_resource> r=doc.query(u);
								sbj=new subject(r.get() ? r.get() : base_resource::nil.get());
								index[i->v[0].v[0].t.second]=sbj;
								current_sbj=sbj;
							}else{
								cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
								return false;
							}
							return false;
						}
						break;		
						default:
							cerr<<"??"<<endl;
						break;
					}
				}
				break;
				case turtle_parser::verb::id:{
					switch(i->v[0].t.first){
						case turtle_parser::variable::id:{
							current_sbj->verbs.push_back(verb(shared_ptr<rdf::Property>(),0));
							current_sbj->verbs.back().name=i->v[0].t.second;		
							current_sbj->verbs.back().is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
						}
						break;
						case turtle_parser::uriref::id:{
							shared_ptr<rdf::Property> r=doc.query_t<rdf::Property>(uri::hash_uri(i->v[0].t.second));
							current_sbj->verbs.push_back(verb(r ? r : rdf::Property::nil,0));
						}
						break;
						case turtle_parser::qname::id:{
							PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
							if(j!=prefix_ns.end()){
								uri u(j->second,i->v[0].v[1].t.second);
								shared_ptr<rdf::Property> r=doc.query_t<rdf::Property>(u);
								current_sbj->verbs.push_back(verb(r ? r : rdf::Property::nil,0));
							}else{
								cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
								return false;
							}
						}
						break;
						case turtle_parser::is_a::id:{
							current_sbj->verbs.push_back(verb(rdf::type::get_property(),0));
						}
						break;
					}
				}
				break;
				case turtle_parser::object::id:{
					switch(i->v[0].t.first){
						case turtle_parser::variable::id:{
							INDEX::iterator j=index.find(i->v[0].t.second);
							if(j!=index.end()){
								current_sbj->verbs.back().object=j->second;
							}else{
								subject* object=new subject();
								object->name=i->v[0].t.second;
								object->is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
								index[i->v[0].t.second]=object;
								current_sbj->verbs.back().object=object;
							}	
						}
						break;
						case turtle_parser::uriref::id:{
							shared_ptr<base_resource> r=doc.query(uri::hash_uri(i->v[0].t.second));
							subject* object=new subject(r.get() ? r.get() : base_resource::nil.get() );
							index[i->v[0].t.second]=object;
							current_sbj->verbs.back().object=object;
						}
						break;
						case turtle_parser::qname::id:{
							PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
							if(j!=prefix_ns.end()){
								uri u(j->second,i->v[0].v[1].t.second);
								shared_ptr<base_resource> r=doc.query(u);
								subject* object=new subject(r.get() ? r.get() : base_resource::nil.get() );
								index[i->v[0].v[0].t.second]=object;
								current_sbj->verbs.back().object=object;
							}else{
								cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
								return false;
							}
						}
						break;
						case turtle_parser::literal::id:{
							current_sbj->verbs.back().object=new subject(i->v[1].t.second);
						}
						break;
					}
				}
				break;
			}
		}
		return true;
	}
	bool parse_insert_data_statement(PARSE_RES_TREE& r){
	/*
 	*	it is actually a plain turtle parser, it might not be practical to store thr document in a PARSE_RES_TREE,
 	*	we need a generic resource that can store literals and pointers, could use it for rdf_xml_parser as well
 	*	pair<uri,string> and 
 	*	pair<uri,shared_ptr<base_resource> >
 	*	it could be build 
 	*	we could break it down in triples and put on stack
 	*/
		shared_ptr<base_resource> sub;//subject
		shared_ptr<base_resource> substitute;
		for(auto i=r.v.begin();i<r.v.end();++i){
			cerr<<"!!!current:\n"<<*i<<endl;
			switch(i->t.first){
				case turtle_parser::subject::id:{
					switch(i->v[0].t.first){
						case turtle_parser::uriref::id:{
							uri u=uri::hash_uri(i->v[0].t.second);
							sub=doc.query(u);
							if(!sub.get()){//we have to create the resource but we need to know its type, we have access to the whole document
								substitute=RES_PTR(new base_resource(u));
								doc.insert(substitute);
								sub=substitute;
								//we assume that the types will be given in this statement
								auto j=i+1;
								while(j<r.v.end()&&j->t.first==turtle_parser::verb::id){
									cout<<"verb:\n"<<*j<<endl;
									++j;
								}
								return false;
							}
						}
						break;
						case turtle_parser::qname::id:{
							PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
							if(j!=prefix_ns.end()){
								uri u(j->second,i->v[0].v[1].t.second);
								sub=doc.query(u);
								if(!sub.get()){//we have to create the resource but we need to know its type, we have access to the whole document
									substitute=RES_PTR(new base_resource(u));
									doc.insert(substitute);
									sub=substitute;
									//cerr<<"resource `";u.to_uri(cerr);cerr<<"' not found"<<endl;
									//return false;

								}
							}else{
								cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
								return false;
							}

						}
						break;
						case turtle_parser::variable::id:
							return false;
						break;
					}
				}
				break;
				case turtle_parser::verb::id:{
					switch(i->v[0].t.first){
						case turtle_parser::uriref::id:{

						}
						break;
						case turtle_parser::qname::id:{

				
						}
						break;
						case turtle_parser::variable::id:
							return false;
						break;

					}

				}
				break;
				default:
					cerr<<"????"<<endl;
				break;

			}	
		}
		return false;
	}
	bool callback(char_iterator::V& v){
		for(auto i=v.rbegin();i<v.rend();++i){
			cout<<get<0>(*i)<<"\t"<<get<1>(*i)<<"\t"<<get<2>(*i)<<endl;
		}
		cout<<endl;
		return true;
	}
	void callback(ID id,string s){
		cout<<id<<"\t"<<s<<endl;
	}
};
#endif
