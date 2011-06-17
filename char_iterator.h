#ifndef CHAR_ITERATOR_H
#define CHAR_ITERATOR_H
#include <iostream>
#include <string.h> //memset
#include <vector>
#include <tuple>
using namespace std;
/*
 *	this does not check for end-of-file condition but the stream can be
 *	set to throw an exception by setting the exception mask:
 *
 *	stream.exceptions(iostream::eofbit);
 *
 * 	N must be power of 2!
 * 	but it should not be very big, could be adjusted depending on the grammar
 */
class char_iterator{
public:
	/*
 	*	should be put somewhere else
 	*/ 
	//typedef int* ID;
	typedef int ID; //we use __COUNTER__
	bool inside;
	int depth;
	typedef std::tuple<ID,string,int> R;
	typedef vector<R> V;
	V v;
	int index;
	int n_read;
	std::istream& is;
	enum{N=1<<14};
	enum{MASK=N-1};
	char buffer[N+1];//convenience terminates string with \0
	char_iterator(std::istream& is):inside(false),depth(0),index(0),n_read(0),is(is){
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
	inline char& operator*(){
		//cout<<buffer[index&MASK];
		return buffer[index&MASK];	
	}
	//should provide an end iterator
};
#endif
