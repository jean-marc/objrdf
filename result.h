#ifndef RESULT_H
#define RESULT_H
#include <vector>
#include <iostream>
using namespace std;
//name conflict with another `result'
template<typename T> struct _result_{
	T t;
	typedef vector<_result_> V;
	V v;
	_result_(T t):t(t){}
	_result_(){}
	void add(T _t,int depth){
		if(depth==0)
			//v.push_back(result(_t));	
			v.insert(v.begin(),_result_(_t));	
		else	
			//v.back().add(_t,depth-1);
			v.front().add(_t,depth-1);
	}
};
//default, to be specialized
template<typename T> ostream& operator<<(ostream& os,_result_<T>& r){
	os<<"("<<r.t;
	for(auto i=r.v.begin();i<r.v.end();++i) os<<*i;
	return os<<")";
}
template<typename T> ostream& operator<<(ostream& os,const _result_<T>& r){
	os<<"("<<r.t;
	for(auto i=r.v.cbegin();i<r.v.cend();++i) os<<*i;
	return os<<")";
}
#endif
