#ifndef TURTLE_PARSER_H
#define TURTLE_PARSER_H
#include "ebnf_template.h"
#include <iostream>
using namespace std;
//template<typename T=int> //different version of the parser
//see http://www.w3.org/TeamSubmission/turtle
struct turtle_parser:char_iterator{
	typedef plus_p<
			choice<
				range_p<'a','z'>,
				range_p<'A','Z'>,
				range_p<'0','9'>,
				char_p<'_'>
			 > 
		> name;
	typedef or_p<char_p<' '>,or_p<char_p<'\t'>,or_p<char_p<'\r'>,char_p<'\n'> > > > white_space;
	typedef kleene_p<not_p<char_p<'>'> > > relativeURI;
	typedef seq<char_p<'<'>,relativeURI,char_p<'>'> > uriref;
	typedef name prefixName;
	typedef seq<or_p<prefixName,true_p>,char_p<':'>,name> qname;
	//typedef or_p<uriref,qname> resource;
	typedef uriref resource;//for now
	typedef resource predicate;
	typedef seq<char_p<'#'>,kleene_p<not_p<or_p<char_p<'\r'>,char_p<'\n'> > > > > comment;
	typedef kleene_p<not_p<char_p<'\"'> > > scharacter;
	typedef seq<char_p<'\"'>,event<scharacter>,char_p<'\"'> > quotedString;
	typedef seq_p<choice<char_p<'-'>,char_p<'+'>,true_p>,plus_p<range_p<'0','9'> > > integer;
	typedef seq<
			choice<char_p<'-'>,char_p<'+'>,true_p>,
			plus_p<range_p<'0','9'> >,
			choice<
				seq<char_p<'.'>,kleene_p<range_p<'0','9'> > >,
				true_p
			>
		> decimal;
	typedef choice<quotedString/*,integer*/,decimal> literal;
	typedef seq<or_p<char_p<'$'>,char_p<'?'> >,name> variable;//SPARQL
	typedef event<or_p<predicate,variable> > verb;
	template<typename OBJECT> 
	struct objectList:seq<OBJECT,or_p<seq<char_p<','>,objectList<OBJECT> >,true_p> >{};
	template<typename OBJECT> 
	struct predicateObjectList:seq<verb,white_space,objectList<OBJECT>,or_p<seq<char_p<';'>,predicateObjectList<OBJECT> >,true_p> >{};
	template<typename OBJECT> 
	struct blank:seq<char_p<'['>,predicateObjectList<event<OBJECT> >,char_p<']'> >{};
	//struct object:choice<resource,blank<object>,variable,literal>{};
	struct object:choice<resource,blank<object>,literal,variable>{};
	typedef event<choice<resource,blank<object>,variable> > subject;
	typedef seq<subject,white_space,predicateObjectList<event<object> > > triples;
	typedef seq<triples,white_space,char_p<'.'>,kleene_p<white_space> > statement;
	typedef plus_p<statement> turtle_doc;
	turtle_parser(istream& is):char_iterator(is){}
	bool go(){return turtle_doc::go(*this);}
	void callback(subject,string s){/*cerr<<s<<endl;*/}
	void callback(verb,string s){/*cerr<<"\t"<<s<<endl;*/}
	void callback(event<object>,string s){/*cerr<<"\t\t"<<s<<endl;*/}
	void callback(event<scharacter>,string s){/*cerr<<"string:\""<<s<<"\""<<endl;*/}
};


#endif
