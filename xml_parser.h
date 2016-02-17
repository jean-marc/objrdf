#ifndef XML_PARSER_H
#define XML_PARSER_H
#include "ebnf.h"
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <math.h>
#include <vector>
#include <list>
#include <algorithm>
#include "uri.h"
//using namespace std;
typedef std::map<objrdf::uri,std::string> ATTRIBUTES;

template<typename START,typename STOP> struct _range_:seq<START,kleene_p<not_p<STOP> >,STOP>{};
//CRTP curiously recursive template pattern
template<typename SAX_HANDLER> struct xml_parser:char_iterator{
	//<?xml version="1.0" encoding="UTF-8"?>
	typedef seq_c<'<','?'> start_xml_declaration;
	typedef seq_c<'?','>'> end_xml_declaration;
	typedef _range_<start_xml_declaration,end_xml_declaration> xml_declaration;
	typedef seq_c<'<','!','-','-'> start_comment;
	typedef seq_c<'-','-','>' > end_comment;
	typedef _range_<start_comment,end_comment> comment;
	/*
	<!DOCTYPE rdf:RDF [
	    <!ENTITY rdfns 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>
	    <!ENTITY rdfsns 'http://www.w3.org/2000/01/rdf-schema#'>
	    <!ENTITY skosns 'http://www.w3.org/2004/02/skos/core#'>
	]>
	*/
	typedef seq_c<'<','!'> start_doctype;
	typedef seq_c<']','>'> end_doctype;
	typedef _range_<start_doctype,end_doctype> doctype;
	typedef choice<char_p<' '>,char_p<'\n'>,char_p<'\r'>,char_p<'\t'>> white_space;
	//namespace support
	//typedef event<plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>,char_p<'-'> > > > nname;
	typedef plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>,char_p<'-'>>> nname;
	//typedef event<char_p<':'> > colon;
	//typedef seq<nname,or_p<seq<colon,nname>,true_p> > name;
	typedef event<nname> prefix;
	typedef event<nname,int> suffix;
	typedef seq<prefix,or_p<seq<char_p<':'>,suffix>,true_p>> name;
	//typedef seq<nname,colon,nname> name;
	//typedef plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>,char_p<'-'> > > name;
	//struct _prefix;typedef event<name,_prefix> prefix;
	//struct _suffix;typedef event<name,_suffix> suffix;
	//typedef name prefix;
	//typedef name suffix;
	//typedef seq<prefix,char_p<':'>,suffix> qname;
	//typedef plus_p<not_p<char_p<' '> > > name;
	//typedef choice<event<name>,event<qname> > element_name;
	typedef event<name> element_name;
	typedef event<name,int> attribute_name;
	//typedef cb<plus_p<not_p<char_p<'='> > > > attribute_name;
	typedef event<kleene_p<not_p<char_p<'\''>>>> attribute_value_0;
	typedef seq<char_p<'\''>,attribute_value_0,char_p<'\''>> attribute_0;
	typedef event<kleene_p<not_p<char_p<'\"'>>>> attribute_value_1;
	typedef seq<char_p<'"'>,attribute_value_1,char_p<'"'>> attribute_1;
	typedef kleene_p<seq<plus_p<white_space>,seq<attribute_name,char_p<'='>,or_p<attribute_0,attribute_1>>>> attributes;
	//typedef event<seq<char_p<'<'>,element_name,attributes,kleene_p<white_space>,char_p<'>'> > > start_tag;
	typedef event<seq<char_p<'<'>,element_name,attributes,kleene_p<white_space>>> start_tag;
	typedef event<seq<char_p<'<'>,char_p<'/'>,element_name,kleene_p<white_space>,char_p<'>'>>> end_tag;
	typedef event<seq_c<'/','>'>> empty_end_tag;
	//typedef event<seq<char_p<'<'>,element_name,attributes,kleene_p<white_space>,char_p<'/'>,char_p<'>'> > > empty_element;
	//will fill up the buffer quickly
	typedef event<plus_p<not_p<char_p<'<'>>>> text;//could add support for escaping 
	struct element:choice<
		seq<start_tag,or_p<
			seq<char_p<'>'>,kleene_p<or_p<text,element> >,end_tag>, //<abc ...>...</abc>
			empty_end_tag>						//<abc .../> 
		>,
		comment,
		seq<white_space,element>
	>{};
	typedef seq<
		kleene_p<white_space>,
		or_p<xml_declaration,true_p>,
		kleene_p<white_space>,
		or_p<doctype,true_p>,
		element
	> document;
		
	//struct element:or_p<empty_element,seq<start_tag,kleene_p<or_p<text,element> >,end_tag> >{};
	/*struct element:choice<
		seq<start_tag,kleene_p<or_p<text,element> >,end_tag>,
		empty_element,
		comment,
		seq<white_space,element>
	>{};*/
	bool go(){return document::go(*this);}
	string element_name_p,element_name_s,attribute_name_s,prefix_s,suffix_s;
	int depth;
	/*
 	*	an array of all the namespaces present in the document, the array can only
 	*	grow, replicate are not inserted, the index is used as a reference 
 	*
 	*/ 
	vector<string> namespaces;
	struct ns{
		string p;
		string s;
		int depth;
		ns(string p,string s,int d):p(p),s(s),depth(d){}
	};
	list<ns> ns_v;
	xml_parser(istream& is):char_iterator(is){
		depth=0;
		ns_v.push_front(ns("","",depth));//default namespace
	}
	//before put in the map
	struct att_stage{
		string prefix;
		string name;
		string val;
		att_stage(string p,string n,string v):prefix(p),name(n),val(v){}
	};
	vector<att_stage> att_v;
	ATTRIBUTES att_list;
	bool callback(prefix,string s){
		//prefix_s=s;
		prefix_s.clear();
		suffix_s=s;
		return true;
	}
	bool callback(suffix,string s){
		prefix_s=suffix_s;
		suffix_s=s;
		return true;
	}
	bool callback(element_name,string s){
		element_name_p=prefix_s;
		element_name_s=suffix_s;	
		att_list.clear();
		//suffix_s.clear();
		return true;
	}
	bool callback(attribute_name,string s){
		attribute_name_s=s;
		return true;
	}
	bool attribute_value(string s){
		/*
 		*	xmlns='...' default namespace
 		*/	 
		if(prefix_s=="xmlns")
			ns_v.push_front(ns(suffix_s,s,depth));		
		else if(suffix_s=="xmlns")	//default namespace
			ns_v.push_front(ns("",s,depth));		
		else		
			att_v.push_back(att_stage(prefix_s,suffix_s,s));
		/*
		if(suffix_s.size()){
			if(prefix_s=="xmlns")
				ns_v.push_back(ns(suffix_s,s,depth));		
			else
				att_v.push_back(att_stage(prefix_s,suffix_s,s));
		}else
			att_list[attribute_name_s]=s;//duplicate attribute is an error per XML, here the last instance overrides previous ones
		*/
		//suffix_s.clear();
		return true;
	}
	bool callback(attribute_value_0,string s){return attribute_value(s);}
	bool callback(attribute_value_1,string s){return attribute_value(s);}
	struct cmp{
		const string prefix;
		cmp(const string _p):prefix(_p){/*prefix=_p;*/}
		bool operator()(ns& s) const{return prefix==s.p;}
	};
	void print_ns(){
		for(typename list<ns>::iterator i=ns_v.begin();i!=ns_v.end();++i)
			LOG_DEBUG<<"\t"<<i->p<<"\t"<<i->s<<"\t"<<i->depth<<endl;
	}
	bool callback(start_tag,string s){
		//need to retrieve all the namespaces
		//print_ns();
		for(typename vector<att_stage>::iterator i=att_v.begin();i<att_v.end();++i){
			//LOG<<i->prefix<<" "<<i->name<<" "<<i->val<<endl;
			typename list<ns>::iterator j=find_if(ns_v.begin(),ns_v.end(),cmp(i->prefix));	
			if(j!=ns_v.end()){
				att_list[objrdf::uri(j->s,i->name)]=i->val;
			}else{
				LOG_ERROR<<"Namespace prefix:`"<<i->prefix<<"' no defined"<<endl;
			}	
		}
		att_v.clear();
		//if(element_name_s.size()){
			typename list<ns>::iterator j=find_if(ns_v.begin(),ns_v.end(),cmp(element_name_p));	
			if(j!=ns_v.end()){
				static_cast<SAX_HANDLER*>(this)->start_element(objrdf::uri(j->s,element_name_s),att_list);
			}else{
				LOG_ERROR<<"Namespace prefix:`"<<element_name_p<<"' no defined"<<endl;
			}	
		//}else
		//	static_cast<SAX_HANDLER*>(this)->start_element(element_name_p,att_list);
		depth++;
		return true;
	}
	struct deeper{
		const int d;
		deeper(int _d):d(_d){}
		bool operator()(ns& s) const{
			//LOG<<"deeper? "<<d<<" "<<s.p<<" "<<s.s<<" "<<s.depth<<endl;
			return d<s.depth;
		}
	};
	bool callback(end_tag,string s){
		static_cast<SAX_HANDLER*>(this)->end_element(objrdf::uri(element_name_s));
		//remove all namespaces deeper than current depth
		depth--;
		ns_v.remove_if(deeper(depth-1));	
		return true;
	}
	bool callback(empty_end_tag,string s){
		static_cast<SAX_HANDLER*>(this)->end_element(objrdf::uri(element_name_s));
		depth--;
		ns_v.remove_if(deeper(depth-1));	
		return true;
	}
	bool callback(text,string s){
		static_cast<SAX_HANDLER*>(this)->characters(s);
		return true;
	}
};
//why is it so convoluted?
template<template<typename SAX_HANDLER> class PARSER> struct quiet:PARSER<quiet<PARSER> >{
	quiet(istream& is):PARSER<quiet<PARSER> >(is){}
	bool start_element(objrdf::uri name,ATTRIBUTES att){return true;}
	bool end_element(objrdf::uri name){return true;}
	bool characters(string s){return true;}

};
template<template<typename SAX_HANDLER> class PARSER> struct generic_xml_parser:PARSER<generic_xml_parser<PARSER> >{
	typedef PARSER<generic_xml_parser<PARSER> > BASE;
	generic_xml_parser(istream& is):BASE(is){};
	bool start_element(objrdf::uri name,ATTRIBUTES att){
		cout<<"start element `"<<name<<"'\t"<<(char)BASE::is.peek()<<endl;
		for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
			cout<<"\t"<<i->first<<"->"<<i->second<<"\n";
		return true;
	}
	bool end_element(objrdf::uri name){
		cout<<"end element `"<<name<<"'"<<endl;
		return true;
	}
	bool characters(string s){
		cout<<s.size()<<" character(s) "<<s<<endl;
		return true;
	}
};
/*
template<template<typename SAX_HANDLER> class PARSER> struct xml_xml:PARSER<xml_xml<PARSER> >{
	typedef PARSER<xml_xml<PARSER> > BASE;
	//to be used in bash 
	int depth;
	xml_xml(istream& is):BASE(is){
		depth=0;
	}
	bool start_element(string name,ATTRIBUTES att){
		if(name=="path"&&att["sodipodi:type"]=="arc"){
			//att["jm"]="hello";
			float cx=0,cy=0,rx=0,ry=0,start=0,end=0;
			{istringstream i(att["sodipodi:cx"]);i>>cx;}
			{istringstream i(att["sodipodi:cy"]);i>>cy;}
			{istringstream i(att["sodipodi:rx"]);i>>rx;}
			{istringstream i(att["sodipodi:ry"]);i>>ry;}
			{istringstream i(att["sodipodi:start"]);i>>start;}
			{istringstream i(att["sodipodi:end"]);i>>end;}
			LOG<<cx<<" "<<cy<<" "<<rx<<" "<<ry<<" "<<start<<" "<<end<<endl;
			ostringstream o;
			float x_axis_rotation=0;
			int large_arc_flag=0;
			int sweep_flag=(start<end);
			o<<"M "<<cx+rx*cos(start)<<" "<<cy+ry*sin(start)<<" A "<<rx<<" "<<ry<<" "<<x_axis_rotation<<" "<<large_arc_flag<<" "<<sweep_flag<<" "<<cx+rx*cos(end)<<" "<<cy+ry*sin(end);
			LOG<<o.str()<<endl;
			LOG<<att["d"]<<endl<<endl;
			att["d"]=o.str();
		}
		cout<<"<"<<name; 
		for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
			cout<<" "<<i->first<<"='"<<i->second<<"'";
		cout<<">";//\n";
		return true;
	}
	bool end_element(string name){
		cout<<"</"<<name<<">";//\n";
		return true;
	}
	bool characters(string s){
		cout<<s;
		return true;
	}
};
*/
template<template<typename SAX_HANDLER> class PARSER> struct pretty_xml:PARSER<pretty_xml<PARSER> >{
	typedef PARSER<pretty_xml<PARSER> > BASE;
	//to be used in bash 
	int depth;
	pretty_xml(istream& is):BASE(is){
		depth=0;
	}
	bool start_element(objrdf::uri name,ATTRIBUTES att){
		cout<<"\033[36m<"<<name; 
		for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
			cout<<" \033[32m"<<i->first<<"\033[m=\033[31m'"<<i->second<<"'";
		cout<<"\033[36m>\033[m";//\n";
		return true;
	}
	bool end_element(objrdf::uri name){
		cout<<"\033[36m</"<<name<<">\033[m";//\n";
		return true;
	}
	bool characters(string s){
		cout<<s;
		return true;
	}
};
#endif
