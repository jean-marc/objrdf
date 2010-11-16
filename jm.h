#ifndef JM_H
#define JM_H
#include <iostream>
using namespace std;
/*
 *	problem: how to represent an empty structure (like an empty vector)
 *	we could consider the default struct to be empty
 *
 */
template<typename T> struct bi{
//template<typename T> struct bi{
	//T t;
	T* right;
	T* down;
	//int id;
	//bi(int _id=0):id(_id),right(0),down(0){}
	//bi(T _t):t(_t),right(0),down(0){}
	bi():right(0),down(0){}
	~bi(){
		delete right;
		delete down;
	}
	T* floor(){return down ? down->floor() : static_cast<T*>(this);}
	T* end(){return right ? right->end() : static_cast<T*>(this);}
	/*
	int height(){
		int i=0;
		bi* current=down;
		while(current){
			i+=current->right ? current->right->height() : 1;
			current=current->down;
		}
		return i;
	}*/
	void normalize(){
		if(right) right->normalize();
		if(right&&right->down){
			T* tmp=down;
			//down=new bi(id);
			//down=new bi(t);
			down=new T(*static_cast<T*>(this));//how come pointers are not copied???
			down->right=right->down;
			right->down=0;
			down->down=tmp;
		}
		if(down) down->normalize();
	}
	void serialize(ostream& os,int depth=0){
		os<<"\r";
		for(int i=0;i<depth;++i)os<<"\t\t";
		//T::operator<<(os,*this);
		//os<<"["<<t<<"]";
		os<<"["<<*static_cast<T*>(this)<<"]";
		if(right){ 
			os<<"->-";
			right->serialize(os,depth+1);
		}else
			os<<"\n";
		if(down) down->serialize(os,depth);
	}
	friend ostream& operator<<(ostream& os,bi& b){
		b.serialize(os,0);
		return os;
	}
};
#endif
