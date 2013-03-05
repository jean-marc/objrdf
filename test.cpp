#include "ebnf.h"
#include <string.h>
//struct jm:seq_t<char_p<'('>,kleene_pt<or_p<event<char_p<'a'>>,jm>>,char_p<')'>>{};	
//simple turtle parser
typedef event<plus_p<char_p<'a'>>> token_0;
typedef event<char_p<'b'>> token_1;
//typedef char_p<'B'> token_1;
//typedef event<seq_t<char_p<'('>,plus_pt<or_p<token_0,token_1>>,char_p<')'>>> jm;
//struct jm:event<seq_t<char_p<'('>,plus_pt<choice<token_0,token_1,jm>>,char_p<')'>>>{}; 
struct jm:event<seq_t<char_p<'('>,event<kleene_p<not_p<char_p<')'>>>>,char_p<')'>>>{}; 
/*
 *	result 
 *
 */

int main(int argc,char* argv[]){
	cout<<argv[1]<<endl;
	auto r=get_range(argv[1],argv[1]+strlen(argv[1]));
	cout<<endl<<jm::go(r)<<endl;
}
