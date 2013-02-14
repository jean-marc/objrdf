#ifndef TURTLE_PARSER_H
#define TURTLE_PARSER_H
#include "ebnf.h"
#include <iostream>
using namespace std;
//template<typename T=int> //different version of the parser
//see http://www.w3.org/TeamSubmission/turtle
//pretty conservative, no extra white space allowed
struct turtle_parser{
	typedef plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>>> name;
	typedef char_p<'\''> sq;
	typedef char_p<'\"'> dq;
	typedef or_p<range_p<'a','z'>,range_p<'A','Z'>> pn_char_base;
	typedef event<kleene_p<not_p<or_p<char_p<'>'>,white_space>>>> uriref;
	typedef seq<char_p<'<'>,uriref,char_p<'>'> > _uriref_;
	typedef event<or_p<seq<pn_char_base,name>,true_p>> prefixName;
	//[99]    PN_PREFIX     ::=       PN_CHARS_BASE ((PN_CHARS|'.')* PN_CHARS)?
	typedef seq<prefixName,char_p<':'>> pname_ns;
	typedef event<name> pname_local;	
	typedef event<seq<pname_ns,pname_local>> qname;
	typedef or_p<_uriref_,qname> resource;
	//typedef uriref resource;//for now
	typedef resource predicate;
	typedef seq<char_p<'#'>,kleene_p<not_p<choice_c<'\r','\n'>>>> comment;
	typedef event<kleene_p<not_p<sq>>> string_literal_1;
	typedef event<kleene_p<not_p<dq>>> string_literal_2;
	typedef seq<choice<char_p<'-'>,char_p<'+'>,true_p>,plus_p<range_p<'0','9'>>> integer;
	typedef seq<choice<char_p<'-'>,char_p<'+'>,true_p>,plus_p<range_p<'0','9'>>,choice<seq<char_p<'.'>,kleene_p<range_p<'0','9'>>>,true_p>> decimal;
	typedef event<choice<seq<sq,string_literal_1,sq>,seq<dq,string_literal_2,dq>,event<decimal>>> literal;
	typedef event<seq<choice_c<'$','?'>,name>> variable;//SPARQL
	typedef event<char_p<'a'>> is_a;
	typedef event<choice<_uriref_,qname,variable,is_a>> verb;
	template<typename OBJECT> struct objectList:plus_pt<OBJECT,char_p<','>>{};
	template<typename OBJECT> struct predicateObjectList:plus_pt<seq_t<verb,objectList<OBJECT>>,char_p<';'>>{};
	template<typename OBJECT> struct blank:seq<char_p<'['>,predicateObjectList<OBJECT>,char_p<']'>>{};
	struct object:event<choice<resource,event<blank<object>>,literal,variable>>{};
	typedef event<choice<resource,variable>> subject;
	typedef seq_t<subject,predicateObjectList<object>> triples;
	typedef seq_t<triples,char_p<'.'>> statement;
	typedef plus_p<statement> doc;
};


#endif
