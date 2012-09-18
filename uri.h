#ifndef URI_H
#define URI_H
#include <iostream>
#include <vector>
#include <algorithm>
namespace objrdf{
	using namespace std;
	class uri{
	public:
		typedef std::pair<string,string> ns_prefix;
		uri();
		/*
 		*	how to build uri from `http://www.w3.org/2000/01/rdf-schema#Class'?
 		*	there are 2 types of uri hash uri
 		*/ 
		explicit uri(string local);
		uri(string ns,string local);
		uri(string ns,string prefix,string local);
		/*
 		*	named constructor idiom: http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.8
 		*/ 
		static uri hash_uri(string s);
		string ns() const;
		bool empty() const;
		bool is_local() const;
		bool operator==(const uri& u) const;
		bool operator!=(const uri& u) const;
		bool operator<(const uri& u) const;//for std::map
		friend ostream& operator<<(ostream& os,const uri& u);
		static void print();
		static void ns_declaration(ostream& os);
		static void ns_declaration_pretty(ostream& os);
		void to_uri(ostream& os) const;
		#ifdef PERSISTENT
		uri(const uri& u);
		uri& operator=(const uri& u);
		enum{STR_SIZE=16};
		char local[STR_SIZE];	
		#else
		const string local;
		#endif
	private:
		//make sure all prefixes are different
		static void check();
		/*
 		*	what happens when multithreaded?
 		*	it should really be a outside variable 
 		*	we can customize the serialization for instance
 		*	have a canonical one: 
 		*		xmlns:_0=... 
 		*		xmlns:_1=...
 		*		_0:node=... _1:node=...
 		*	and disregard prefix	
 		*
 		*/ 
	public:
		static vector<ns_prefix>& ns_v();
		short index;
	};
}
#endif

