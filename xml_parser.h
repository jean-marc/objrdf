#ifndef XML_PARSER_H
#define XML_PARSER_H
#include "ebnf_template.h"
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <math.h>
using namespace std;
typedef map<string,string> ATTRIBUTES;
//CRTP curiously recursive template pattern
template<typename SAX_HANDLER> struct xml_parser:char_iterator{
	//<?xml version="1.0"?>
	//typedef seq_c<'<','?','x','m','l'
	typedef seq_c<'<','!','-','-'> start_comment;
	typedef seq_c<'-','-','>' > end_comment;
	//typedef seq<start_comment,or_p<end_comment,true_p> > comment;
	typedef seq<start_comment,kleene_p<not_p<end_comment> >,end_comment> comment;
	typedef choice<char_p<' '>,char_p<'\n'>,char_p<'\r'>,char_p<'\t'> > white_space;
	//typedef plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>,char_p<'-'>,char_p<':'> > > name;
	typedef plus_p<choice<range_p<'a','z'>,range_p<'A','Z'>,range_p<'0','9'>,char_p<'_'>,char_p<'-'> > > name;
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
	typedef event<kleene_p<not_p<char_p<'\''> > > > attribute_value_0;
	typedef seq<attribute_name,char_p<'='>,char_p<'\''>,attribute_value_0,char_p<'\''>  > attribute_0;
	typedef event<kleene_p<not_p<char_p<'\"'> > > > attribute_value_1;
	typedef seq<attribute_name,char_p<'='>,char_p<'"'>,attribute_value_1,char_p<'"'>  > attribute_1;
	typedef kleene_p<seq<plus_p<white_space>,or_p<attribute_0,attribute_1> > > attributes;
	typedef event<seq<char_p<'<'>,element_name,attributes,kleene_p<white_space>,char_p<'>'> > > start_tag;
	typedef event<seq<char_p<'<'>,char_p<'/'>,element_name,kleene_p<white_space>,char_p<'>'> > > end_tag;
	typedef event<seq<char_p<'<'>,element_name,attributes,kleene_p<white_space>,char_p<'/'>,char_p<'>'> > > empty_element;
	typedef event<plus_p<not_p<char_p<'<'> > > > text;
	//struct element:or_p<empty_element,seq<start_tag,kleene_p<or_p<text,element> >,end_tag> >{};
	struct element:choice<
		seq<start_tag,kleene_p<or_p<text,element> >,end_tag>,
		empty_element,
		comment,
		seq<white_space,element>
	>{};
	xml_parser(istream& is):char_iterator(is){}
	bool go(){return element::go(*this);}
	string element_name_s,attribute_name_s;
	ATTRIBUTES att_list;
	/*
	bool callback(prefix,string s){
		return true;
	}
	bool callback(suffix,string s){
		return true;
	}
	*/
	//bool callback(name,string s){
	bool callback(element_name,string s){
		element_name_s=s;	
		att_list.clear();
		return true;
	}
	bool callback(attribute_name,string s){
		attribute_name_s=s;
		return true;
	}
	bool callback(attribute_value_0,string s){
		att_list[attribute_name_s]=s;//duplicate attribute is an error per XML, here the last instance overrides previous ones
		return true;
	}
	bool callback(attribute_value_1,string s){
		att_list[attribute_name_s]=s;//duplicate attribute is an error per XML, here the last instance overrides previous ones
		return true;
	}
	bool callback(start_tag,string s){
		static_cast<SAX_HANDLER*>(this)->start_element(element_name_s,att_list);
		return true;
	}
	bool callback(end_tag,string s){
		static_cast<SAX_HANDLER*>(this)->end_element(element_name_s);
		return true;
	}
	bool callback(empty_element,string s){
		static_cast<SAX_HANDLER*>(this)->start_element(element_name_s,att_list);
		static_cast<SAX_HANDLER*>(this)->end_element(element_name_s);
		return true;
	}
	bool callback(text,string s){
		static_cast<SAX_HANDLER*>(this)->characters(s);
		return true;
	}
	
};
template<template<typename SAX_HANDLER> class PARSER> struct generic_xml_parser:PARSER<generic_xml_parser<PARSER> >{
	typedef PARSER<generic_xml_parser<PARSER> > BASE;
	generic_xml_parser(istream& is):BASE(is){};
	bool start_element(string name,ATTRIBUTES att){
		cout<<"start element `"<<name<<"'\t"<<(char)BASE::is.peek()<<endl;
		for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
			cout<<"\t"<<i->first<<"->"<<i->second<<"\n";
		return true;
	}
	bool end_element(string name){
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
			cerr<<cx<<" "<<cy<<" "<<rx<<" "<<ry<<" "<<start<<" "<<end<<endl;
			ostringstream o;
			float x_axis_rotation=0;
			int large_arc_flag=0;
			int sweep_flag=(start<end);
			o<<"M "<<cx+rx*cos(start)<<" "<<cy+ry*sin(start)<<" A "<<rx<<" "<<ry<<" "<<x_axis_rotation<<" "<<large_arc_flag<<" "<<sweep_flag<<" "<<cx+rx*cos(end)<<" "<<cy+ry*sin(end);
			cerr<<o.str()<<endl;
			cerr<<att["d"]<<endl<<endl;
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
	bool start_element(string name,ATTRIBUTES att){
		cout<<"\033[36m<"<<name; 
		for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
			cout<<" \033[32m"<<i->first<<"\033[m=\033[31m'"<<i->second<<"'";
		cout<<"\033[36m>\033[m";//\n";
		return true;
	}
	bool end_element(string name){
		cout<<"\033[36m</"<<name<<">\033[m";//\n";
		return true;
	}
	bool characters(string s){
		cout<<s;
		return true;
	}
};
#endif
