#ifndef CHAR_ITERATOR_H
#define CHAR_ITERATOR_H
#include <iostream>
#include <string.h> //memset
using namespace std;
/*
 *	this does not check for end-of-file condition but the stream can be
 *	set to throw an exception by setting the exception mask:
 *
 *	stream.exceptions(iostream::eofbit);
 *
 * 	N must be power of 2!
 */
struct char_iterator{
	std::istream& is;
	enum{N=1<<14};
	enum{MASK=N-1};
	int n_read,index;
	char buffer[N+1];//convenience terminates string with \0
	char_iterator(std::istream& _is):is(_is){
		index=0;
		n_read=0;
		memset(buffer,0,N+1);
	}
	void increment(){
		++index;	
		if(index>n_read){//diff should always be 1
			buffer[index&MASK]=is.get();
			++n_read;
		}
	}
	char_iterator& operator++(){
		increment();
		return *this;
	}
	char& operator*(){
		return buffer[index&MASK];	
	}
	//should provide an end iterator
};
#endif
