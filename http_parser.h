/*
 *	parses http request and exposes parameters
 *
 *	GET|POST|PUT|HEAD  path?a=b&c=d&e=f	HTTP/1.1
 *	key:value\r\n
 *	key:value\r\n
 *
 *	message ...
 *
 */
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H
#include "ebnf.h"
#include "log_info.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;
//template<typename HANDLER> struct http_parser:char_iterator/*<128>*/{
namespace objrdf{
	extern int verbosity;
	extern const char _context_[];
	struct http_parser:char_iterator/*<128>*/{
		typedef seq_c<'\r','\n'> crlf;
		//typedef seq<char_p<'G'>,char_p<'E'>,char_p<'T'>> get;
		typedef event<plus_p<not_p<char_p<' '>>>> method;//if eof it will loop forever
		typedef plus_p<not_p<or_p<char_p<' '>,or_p<char_p<'\t'>,or_p<char_p<'\r'>,char_p<'\n'>>>>>> str;//a string
		typedef event<plus_p<not_p<or_p<char_p<'='>,char_p<' '>>>>> _key;
		typedef event<plus_p<not_p<or_p<char_p<'&'>,char_p<' '>>>>> _value;
		typedef or_p<seq<_key,char_p<'='>,_value>,true_p> _pair_;//accept empty pairs
		struct _pairs_:seq<_pair_,or_p<seq<char_p<'&'>,_pairs_>,true_p>>{}; //recursive
		typedef	seq<char_p<'?'>,or_p<_pairs_,true_p>> arguments; 
		typedef event<plus_p<not_p<or_p<char_p<' '>,char_p<'?'>>>>> path;
		typedef seq<path,or_p<arguments,true_p>> url;
		typedef event<str> _version;
		typedef seq<method,char_p<' '>,url,char_p<' '>,_version,crlf> head;
		typedef event<plus_p<not_p<or_p<char_p<':'>,char_p<'\r'>>>>> key;
		typedef event<plus_p<not_p<char_p<'\r'>>>> value;	
		typedef seq<key,seq_p<char_p<':'>,char_p<' '>>,value,crlf> header;
		typedef seq<head,kleene_p<header>,crlf> request;
		http_parser(istream& is):char_iterator/*<128>*/(is){}
		//bool go(){return request::go(*static_cast<HANDLER*>(this));}
		bool go(){return request::go(*this);}
		typedef map<string,string> M;
		M headers,url_arguments;
		string current_key,current_path,current_method,version;
		bool callback(_version,string s){
			version=s;
			return true;
		}
		bool callback(method,string s){
			current_method=s;
			LOG_DEBUG<<"method:"<<s<<endl;
			return true;
		}
		bool callback(key,string s){
			current_key=s;
			return true;
		}
		bool callback(value,string s){
			headers[current_key]=s;
			LOG_DEBUG<<current_key<<" : "<<s<<endl;
			return true;
		}
		bool callback(path,string s){
			current_path=s;
			LOG_DEBUG<<"path:"<<s<<endl;
			return true;
		}
		bool callback(_key,string s){
			current_key=s;
			return true;
		}
		bool callback(_value,string s){
			url_arguments[current_key]=s;
			LOG_DEBUG<<"\t"<<current_key<<" : "<<s<<endl;
			return true;
		}
	};
	/*
	struct generic_http_parser:http_parser<generic_http_parser>{
		string current_key,current_path,current_method;
		typedef map<string,string> M;
		M headers,url_arguments;
		generic_http_parser(istream& is):http_parser<generic_http_parser>(is){}
		bool callback(method,string s){
			current_method=s;
			cerr<<"method:"<<s<<endl;
		}
		bool callback(key,string s){
			current_key=s;
			cerr<<"key:"<<s<<endl;
		}
		bool callback(value,string s){
			headers[current_key]=s;
			cerr<<"value:"<<s<<endl;
		}
		bool callback(path,string s){
			current_path=s;
			cerr<<"path:"<<s<<endl;
		}
		bool callback(_key,string s){
			current_key=s;
			cerr<<"_key:"<<s<<endl;
		}
		bool callback(_value,string s){
			url_arguments[current_key]=s;
			cerr<<"_value:"<<s<<endl;
		}
	};
	*/
}
#endif
