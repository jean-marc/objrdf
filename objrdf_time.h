#ifndef OBJRDF_TIME_H
#define OBJRDF_TIME_H
#include <chrono>
#include "objrdf.h"

namespace objrdf{
	template<> struct selector<std::chrono::system_clock::time_point>{
		typedef xsd::dateTime ResultT;
		enum{IS_LITERAL=1};
	};
	typedef property<objrdf_rdfs_ns,str<'t','s'>,std::chrono::system_clock::time_point> ts;
	ostream& operator<<(ostream& os,const std::chrono::system_clock::time_point&);
	istream& operator>>(istream& is,std::chrono::system_clock::time_point&);
	ostream& operator<<(ostream& os,const std::chrono::milliseconds&);
	istream& operator>>(istream& is,std::chrono::milliseconds&);
}

#endif
