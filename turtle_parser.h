#ifndef TURTLE_PARSER_H
#define TURTLE_PARSER_H
#include "ebnf.h"
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
	typedef char_p<'\''> sq;
	typedef char_p<'\"'> dq;
	typedef or_p<range_p<'a','z'>,range_p<'A','Z'>> pn_char_base;
	//typedef or_p<char_p<' '>,or_p<char_p<'\t'>,or_p<char_p<'\r'>,char_p<'\n'> > > > white_space;
	//dangerous! "<aaaa " will cause it to read forever
	typedef event_1<kleene_p<not_p<or_p<char_p<'>'>,white_space> > >,__COUNTER__> uriref;
	typedef seq<char_p<'<'>,uriref,char_p<'>'> > _uriref_;
	typedef event_1<or_p<seq<pn_char_base,name>,true_p>,__COUNTER__> prefixName;
	//typedef event_1<or_p<name,true_p>,__COUNTER__> prefixName;//to parse blank nodes
	//[99]    PN_PREFIX     ::=       PN_CHARS_BASE ((PN_CHARS|'.')* PN_CHARS)?
	typedef seq<prefixName,char_p<':'> > pname_ns;
	typedef event_1<name,__COUNTER__> pname_local;	
	typedef event_1<seq<pname_ns,pname_local>,__COUNTER__> qname;
	typedef or_p<_uriref_,qname> resource;
	//typedef uriref resource;//for now
	typedef resource predicate;
	typedef seq<char_p<'#'>,kleene_p<not_p<or_p<char_p<'\r'>,char_p<'\n'> > > > > comment;
	typedef event_1<kleene_p<not_p<sq> >,__COUNTER__> string_literal_1;
	typedef event_1<kleene_p<not_p<dq> >,__COUNTER__> string_literal_2;
	/*
 	*	add support for hex values?
 	*/ 
	typedef seq_p<choice<char_p<'-'>,char_p<'+'>,true_p>,plus_p<range_p<'0','9'> > > integer;
	typedef seq<
			choice<char_p<'-'>,char_p<'+'>,true_p>,
			plus_p<range_p<'0','9'> >,
			choice<
				seq<char_p<'.'>,kleene_p<range_p<'0','9'> > >,
				true_p
			>
		> decimal;
	typedef event_1<choice<seq<sq,string_literal_1,sq>,seq<dq,string_literal_2,dq>,event_1<decimal,__COUNTER__> >,__COUNTER__> literal;
	typedef event_1<seq<or_p<char_p<'$'>,char_p<'?'> >,name>,__COUNTER__ > variable;//SPARQL
	typedef event_1<char_p<'a'>,__COUNTER__> is_a;
	typedef event_1<choice<_uriref_,qname,variable,is_a>,__COUNTER__ > verb;
	template<typename OBJECT> 
	struct objectList:seq<OBJECT,or_p<seq<char_p<','>,objectList<OBJECT> >,true_p> >{};
	template<typename OBJECT> struct predicateObjectList:seq<
		verb,
		white_space,
		objectList<OBJECT>,
		or_p<
			seq<
				char_p<';'>,
				predicateObjectList<OBJECT> 
			>,
		true_p> >{};
	//template<typename OBJECT> struct blank:seq<char_p<'['>,predicateObjectList<event<OBJECT> >,char_p<']'> >{};
	typedef event_1<seq<char_p<'_'>,char_p<':'>,pname_local>,__COUNTER__> nodeID;
	template<typename OBJECT> struct blank:or_p<
		seq<
			char_p<'['>,
			or_p<predicateObjectList<OBJECT>,true_p>,
			char_p<']'>
		>,
		nodeID
		>{};
	
	struct object:event_1<
		choice<
			resource,
			event_1<blank<object>,9999>,
			literal,
			variable
		>,
	1111>{};
	typedef event_1<
		choice<
			resource,
			event_1<blank<object>,9999>, //looks a bit suspicious....
			variable
		>,
		__COUNTER__ 
	> subject;
	typedef seq<subject,white_space,predicateObjectList<object>> triples;
	typedef seq<triples,white_space,char_p<'.'>,kleene_p<white_space>> statement;
	typedef plus_p<statement> turtle_doc;
	turtle_parser(istream& is):char_iterator(is){}
	bool go(){return turtle_doc::go(*this);}
	template<typename T> void callback(T,string){}
	bool callback(char_iterator::V& v){return true;}
	bool callback(PARSE_RES_TREE& r){return true;}
};


#endif
