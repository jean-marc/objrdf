#ifndef OBJRDF_CONVERSION_H
#define OBJRDF_CONVERSION_H
/*
*	from/to base64, hex
*/
#include <iterator>
namespace objrdf{
	template<
		typename INPUT_ITERATOR,
		typename OUTPUT_ITERATOR //must point to char type
	> void to_hex(INPUT_ITERATOR begin,INPUT_ITERATOR end,OUTPUT_ITERATOR output_begin){
		for(auto i=begin;i!=end;++i){
			static const char hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
			auto tmp=*i;
			//char buffer[2*sizeof(tmp)];
			vector<char> buffer(2*sizeof(tmp));
			for(auto j=buffer.rbegin();j!=buffer.rend();++j,tmp>>=4) *j=hex[tmp&0xf];
			output_begin=copy(buffer.cbegin(),buffer.cend(),output_begin);
		}
	}
	template<
		typename INPUT_ITERATOR,//must point to char type
		typename OUTPUT_ITERATOR 
	> void from_hex(INPUT_ITERATOR begin,INPUT_ITERATOR end,OUTPUT_ITERATOR output_begin){
		typedef typename OUTPUT_ITERATOR::value_type OUTPUT;//does not work with output_iterator!
		assert((end-begin)%(2*sizeof(OUTPUT))==0);
		for(auto i=begin;i!=end;){
			OUTPUT tmp=0;
			for(auto j=0;j<2*sizeof(OUTPUT);++i,++j){
				tmp<<=4;
				if(*i>='a') tmp|=0xA+*i-'a';
				else if(*i>='A') tmp|=0xA+*i-'A';
				else tmp|=*i-'0';
				//cerr<<*i<<"\t"<<(int)tmp<<endl;
			}
			*output_begin=tmp;
			++output_begin;
		}	
	}
		
}
#endif
