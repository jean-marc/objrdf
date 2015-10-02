#ifndef URI_H
#define URI_H
#include <iostream>
#include <vector>
#include <algorithm>
//using namespace std;
namespace objrdf{
	class uri{
	public:
		typedef std::pair<std::string,std::string> ns_prefix;
		uri();
		/*
 		*	how to build uri from `http://www.w3.org/2000/01/rdf-schema#Class'?
 		*	there are 2 types of uri hash uri
 		*/ 
		explicit uri(std::string local);
		uri(std::string ns,std::string local);
		uri(std::string ns,std::string prefix,std::string local);
		/*
 		*	named constructor idiom: http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.8
 		*/ 
		static uri hash_uri(std::string s);
		static uri bnode_uri(std::string s);
		static uri qname_uri(std::string s);
		std::string ns() const;
		bool empty() const;
		bool is_local() const;
		bool operator==(const uri& u) const;
		bool operator!=(const uri& u) const;
		bool operator<(const uri& u) const;//for std::map
		int compare(const uri& u) const;
		friend std::ostream& operator<<(std::ostream& os,const uri& u);
		static void print();
		static void ns_declaration(std::ostream& os);
		static void ns_declaration_pretty(std::ostream& os);
		void to_uri(std::ostream& os) const;
		/*
 		*	we can use a pool allocator instead of fixed size array but the problem is
 		*	the id lives in in base_resource that will be derived by persistent or volatile
 		*	classes, so the char array must use the same allocator as derived class
 		*	this also means there will be 2 different pools for ID's, which is fine since the data
 		*	is un-managed anyway but can we use generic pointer inside container? why not?
 		*	eg: satdb 64K carriers name up to 51 bytes -> 3e6 bytes, that would work with 3 bytes pointer
 		*	(how do we do that? use union?) but it means cell is 6 bytes so we can actually 
 		*	we have to find a compromise: 6 bytes per cell ->
 		*	we only need 1 bit for the pool_ptr because there are only 2 pools to choose from, we might
 		*	have to be smart when creating original pool
 		*
 		*/ 
		uri(const uri& u);
		uri& operator=(const uri& u);
		enum{STR_SIZE=32};
		char local[STR_SIZE];	
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
		/*
 		*	indices will become stale when using persistence, that vector should be persisted too!!
 		*	but problem with allocator for string...
 		*/ 
		static std::vector<ns_prefix>& ns_v();
		short index;
	};
}
#endif

