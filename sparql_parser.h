#ifndef SPARQL_PARSER_H
#define SPARQL_PARSER_H
#include "ebnf_template.h"
#include "turtle_parser.h"
#include "shared_ptr.h"
#include "objrdf.h"
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include "jm.h"
#include <algorithm>
#include <sstream>
using namespace std;
/*	basic SPARQL + some support for update: insert, delete, modify
 *	SELECT * WHERE { $x p $v .}
 *	create a graph structure based on the pattern
 */
using namespace objrdf;
//generic graph, it could be interesting to use the objrdf framework for that but it would require changes to the parsers
bool is_variable(string s);
struct binding:bi<binding>{ //CRTP
	string name,s;
	shared_ptr<base_resource> r;
	friend ostream& operator<<(ostream& os,binding& b){
		return os<<b.name<<":"<<((b.r.get())? b.r->id.name : b.s);
	}
	void binding_to_xml(ostream& os){//it would be nice if we could define functions within function
		//need to add support to blank node
		os<<"\t\t\t<binding name='"<<name<<"'>";		
		if(r.get()) 
			os<<"<uri>"<<r->id.name<<"</uri>";
		else
			os<<"<literal>"<<s<<"</literal>";//note we could provide the datatype attribute
		os<<"</binding>\n";
		if(right) right->binding_to_xml(os);
	}
	void result_to_xml(ostream& os){
		os<<"\t\t<result>\n";
		binding_to_xml(os);
		os<<"\t\t</result>\n";
		if(down) down->result_to_xml(os);
	}
	void to_xml(ostream& os){
		os<</*"<?xml version=\"1.0\"?>\n*/"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">\n\t<head></head>\n\t<results>\n";
		result_to_xml(os);
    		os<<"\t</results>\n</sparql>\n";
	}
	//json serialization	
};
typedef pair<bool,binding*> ret;
struct predicate;
struct _resource{
	int n;
	string name;
	vector<predicate> p;	
	bool is_root;
	_resource():n(0),is_root(false){}
	_resource(string s):n(0),name(s),is_root(false){}
	friend ostream& operator<<(ostream& os,_resource& r);
	ret _bind(shared_ptr<base_resource> );
	ret _bind(string);
	ret _bind(base_resource::instance_iterator);
};
struct predicate{
	string name;
	shared_ptr<_resource> object;
	predicate():object(0){}
	predicate(string s,shared_ptr<_resource> _object):object(_object),name(s){}
	friend ostream& operator<<(ostream& os,predicate& p);
	ret _bind(base_resource::instance_iterator);
};
struct sparql_parser:char_iterator{
	rdf::RDF& doc;
	//typedef turtle_parser<>::white_space white_space;
	typedef turtle_parser::white_space white_space;
	typedef seq_c<'S','E','L','E','C','T'> SELECT;	
	typedef seq_c<'I','N','S','E','R','T'> INSERT;	
	typedef seq_c<'D','E','L','E','T','E'> _DELETE_;	
	typedef seq_c<'W','H','E','R','E'> WHERE;
	typedef seq_c<'D','A','T','A'> DATA;
	typedef	seq<WHERE,white_space,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> > where_statement; 
	typedef seq<event<SELECT>,white_space,char_p<'*'>,white_space,where_statement> select_query;	
	typedef seq<event<INSERT>,white_space,DATA,white_space,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> > insert_data_query;
	typedef seq<event<_DELETE_>,white_space,DATA,white_space,char_p<'{'>,turtle_parser::turtle_doc,char_p<'}'> > delete_data_query;
	typedef choice<select_query,insert_data_query,delete_data_query> query;
	shared_ptr<_resource> root,current_subject;
	string current_predicate;
	typedef map<string,shared_ptr<_resource> > M;
	M m;
	//update
	shared_ptr<base_resource> subject;
	base_resource::type_iterator current_property;
	string current_literal_object;
	//
	sparql_parser(rdf::RDF& _doc,istream& is):char_iterator(is),doc(_doc),current_property(base_resource::nil->end()){
		//select
		root=0;
		current_subject=0;
		//update
		subject=base_resource::nil;
	}
	//bool go(){return select_query::go(*this);}
	bool go(){
		bool r=query::go(*this);
		if((q==SELECT_QUERY)&&get_root().get()){
			for(rdf::RDF::V::iterator i=doc.v.begin();i<doc.v.end();++i){
				ret rr=get_root()->_bind(*i);
				if(rr.first) b.floor()->down=rr.second;
				
			}
			//b.serialize(cout);
			b.normalize();
			//b.serialize(cout);
		}
		return r;

	}
	void out(ostream& os){
		//if((q==SELECT_QUERY)&&b.down) b.down->to_xml(os);
		if(q==SELECT_QUERY){
			if(b.down)
				b.down->to_xml(os);
			else{
				os<</*"<?xml version=\"1.0\"?>\n*/"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">\n\t<head></head>\n\t<results>\n";
				os<<"\t</results>\n</sparql>\n";
			}
		}
	}
	//bool is_variable(string s){return !s.empty()&&(s[0]=='$' || s[0]=='@');}
	typedef enum{SELECT_QUERY,INSERT_QUERY,DELETE_QUERY} QUERY_TYPE;
	QUERY_TYPE q;
	binding b;
	void callback(event<turtle_parser::scharacter>,string& s){
		current_literal_object=s;
		//cerr<<"string:\""<<s<<"\""<<endl;
	}
	void callback(event<SELECT>,string& s){q=SELECT_QUERY;}
	void callback(event<INSERT>,string& s){q=INSERT_QUERY;}
	void callback(event<_DELETE_>,string& s){q=DELETE_QUERY;}
	void callback(turtle_parser::subject,string s){
		cerr<<s<<endl;
		if(q==SELECT_QUERY){
			if(is_variable(s)){
				M::iterator i=m.find(s);
				if(i!=m.end()){
					current_subject=i->second;
				}else{
					current_subject=new _resource(s);
					current_subject->is_root=true;
					m[s]=current_subject;
				}
			}else{
				string uri=s.substr(1,s.size()-2);
				//current_subject=new _resource(s);
				current_subject=new _resource(uri);
				root=current_subject;
			}
		}else if(q==INSERT_QUERY){
			string uri=s.substr(1,s.size()-2);
			subject=doc.find(uri);
			if(!subject.get()) {
				cerr<<"resource `"<<s<<"' not found"<<endl;
				subject=base_resource::nil;
			}
		}else if(q==DELETE_QUERY){

		}
	}
	void callback(turtle_parser::verb,string s){
		/*
 		*	could be a uriref or qname or variable
 		*/
		//reset current_literal_object, a bit awkward	
		//current_literal_object
		cerr<<"\t"<<s<<endl;
		if(q==SELECT_QUERY){
			if(is_variable(s))
				current_predicate=s;
			else{
				string uri=s.substr(1,s.size()-2);
				current_predicate=uri;
			}
		}else if(q==INSERT_QUERY){
			string uri=s.substr(1,s.size()-2);
			current_property=std::find_if(subject->begin(),subject->end(),namep(uri));
			if(current_property==subject->end())
				cerr<<"property `"<<s<<"' not found"<<endl;
		}else if(q==DELETE_QUERY){
	
		}

	}
	void callback(event<turtle_parser::object>,string s){
		cerr<<"\t\t"<<s<<endl;
		if(q==SELECT_QUERY){
			shared_ptr<_resource> current_object=0;
			if(is_variable(s)){
				M::iterator i=m.find(s);
				if(i!=m.end()){
					current_object=i->second;
					i->second->is_root=false;
				}else{
					current_object=new _resource(s);
					m[s]=current_object;
				}
			}else{
				//current_object=new _resource(s);
				string uri=s.substr(1,s.size()-2);
				current_object=new _resource(uri);
			}
			current_subject->p.push_back(predicate(current_predicate,current_object));
		}else if(q==INSERT_QUERY){
			if(current_property!=subject->end()){
				//could be a url or literal or blank node	
				if(current_property->literalp()){
					//could be quoted
					if(s[0]=='"'){		
						istringstream is(current_literal_object);
						current_property->add_property()->in(is);
					}else{
						istringstream is(s);
						current_property->add_property()->in(is);
					}
				}else{
					string uri=s.substr(1,s.size()-2);
					shared_ptr<base_resource> object=doc.find(uri);//<uri>
					if(object.get())
						current_property->add_property()->set_object(object.get());//NEED TO CHECK THE TYPE!!!!
					else
						cerr<<"object "<<s<<" not found"<<endl;
				}

			}
		}else if(q==DELETE_QUERY){
	
		}
	}
	shared_ptr<_resource> get_root(){
		//there could be more than one
		if(root.get()) return root;
		M::iterator i=m.begin();
		while(i!=m.end() && !i->second->is_root) ++i;
		return i->second;
	}
	/*
	void callback(turtle_parser<INSERT>::subject,string s){}
	void callback(turtle_parser<INSERT>::predicate,string s){}
	void callback(::callback<turtle_parser<INSERT>::object>,string s){}
	void callback(turtle_parser<_DELETE_>::subject,string s){}
	void callback(turtle_parser<_DELETE_>::predicate,string s){}
	void callback(::callback<turtle_parser<_DELETE_>::object>,string s){}
	*/

};



#endif
