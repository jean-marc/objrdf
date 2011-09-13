#ifndef URL_DECODER_H
#define URL_DECODER_H
#include "ebnf_template.h"
#include <map>
#include <sstream>


struct url_decoder:char_iterator{
	typedef not_p<char_p<EOF> > any_p;
	typedef choice<range_p<'0','9'>,range_p<'a','f'>,range_p<'A','F'> > hex;
	typedef event<seq<char_p<'%'>,hex,hex> > encoded;
	typedef kleene_p<choice<encoded,event<any_p> > > doc;
	string decoded;
	url_decoder(istream& is):char_iterator(is){}
	~url_decoder(){
		cout<<decoded<<endl;
	}
	bool go(){return doc::go(*this);}
	bool callback(event<any_p>,string s){
		decoded.append(s);
		return true;
	}
	bool callback(encoded,string s){
		istringstream is(s.substr(1,2));
		short a=0;
		is>>std::hex>>a; 
		decoded.append(1,(char)a);
		/*typedef std::pair<string,const char> P;
		static std::map<string,const char> m={P("%20",' '),P("%3C",'<'),P("%3E",'>'),P("%23",'#'),P("%22",'"'),P("%27",'\'')};
		auto i=m.find(s);
		if(i==m.end())
			decoded.append(s);
		else
			decoded.append(1,i->second);
		*/
		return true;
	}

};
#endif
