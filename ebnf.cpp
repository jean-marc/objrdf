#include "ebnf.h"
ostream& operator<<(ostream& os,char_iterator::R& vv){
	return os<<get<0>(vv)<<"\t"<<get<1>(vv)<<"\t"<<get<2>(vv);
}
ostream& operator<<(ostream& os,PARSE_RES& r){
	return os<<r.first<<"\t"<<r.second;
}
void print(ostream& os,const PARSE_RES_TREE& p,int depth){
	string s(depth,'\t');
	os<<p.t.first<<"\t"<<s<<p.t.second<<endl;
	for(auto i=p.v.cbegin();i<p.v.cend();++i) print(os,*i,depth+1);
	
}
ostream& operator<<(ostream& os,const PARSE_RES_TREE& p){
	print(os,p,0);
	return os;
};

