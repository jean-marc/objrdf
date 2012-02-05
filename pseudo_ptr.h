#ifndef PSEUDO_PTR_H
#define PSEUDO_PTR_H

#include <vector>
#include <iterator>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <iostream>

using namespace std;
template<typename T> string property_filename(){
	static int n=0;
	ostringstream os;
	os<<"pool_"<<n++;
	return os.str();
};
/*
 *	simple pointer to only one type
 *	pointer to type and derived types
 *	can we cast from one to another?
 *
 *	storage: local or memory-mapped
 *
 */
struct generic_pool{
	struct info{
		size_t next;
		size_t n_cells;
	};
	template<typename T> union cell{
		char data[sizeof(T)];
		info meta;
	};
	void* v;
	size_t n;
	const size_t cell_size;
	/* does not compile*/
	/*
	template<typename T> explicit generic_pool(void* v,size_t n):v(v),n(n),cell_size(sizeof(T)){
		cell<T>* c=static_cast<cell<T>*>(v);
		if(c[0].meta.next==0){
			c[0].meta.next=1;
			c[0].meta.n_cells=0;
		}
		cerr<<c[0].meta.n_cells<<" out of "<<n<<" cells used"<<endl;
	}
	*/
	generic_pool(void* v,size_t n,size_t cell_size):v(v),n(n),cell_size(cell_size){
		info& first=*static_cast<info*>(v);
		if(first.next==0){
			first.next=1;
			first.n_cells=0;
		}
		cerr<<"new pool "<<this<<" "<<first.n_cells<<" out of "<<n<<" cells used"<<endl;

	}
	//dangerous!!
	template<typename T> size_t allocate(){
		cell<T>* c=static_cast<cell<T>*>(v);
		size_t i=c[0].meta.next;
		cerr<<"allocate cell at index "<<(int)c[0].meta.next<<" from pool "<<this<<endl;
		if(c[c[0].meta.next].meta.next==0)
			c[0].meta.next++;
		else
			c[0].meta.next=c[c[0].meta.next].meta.next;
		c[0].meta.n_cells++;
		return i;
	}
	void deallocate(size_t i){
		//assume only 1 cell allocated
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		//problem we don't know T but we know `size', tricky
		info& first=*static_cast<info*>(v);
		info& current=*static_cast<info*>(v+i*cell_size);
		current.next=first.next;	
		first.next=i;
		first.n_cells--;
	}
	void* get(size_t i){
		return v+i*cell_size;
	}
	template<typename T,typename STORE> static generic_pool* get_instance(){
		static generic_pool* p=STORE::create_generic_pool();
		return p;

	}
};
template<
	typename T,
	typename STORE//free store or persistent
			//support derived types
> struct pseudo_ptr{
	typedef unsigned char INDEX;
	struct pool{
		struct info{
			INDEX next;
			size_t n_cells;
		};
		union cell{
			char data[sizeof(T)];
			info meta;
		};
		cell* v;
		size_t n;
		pool(void* _v,size_t n):v((cell*)_v),n(n){
			if(v[0].meta.next==0){
				v[0].meta.next=1;
				v[0].meta.n_cells=0;
			}
			cerr<<v[0].meta.n_cells<<" out of "<<n<<" cells used"<<endl;
		}	
		~pool(){
		}
		static pool& get_instance(){
			static pool* p=STORE::create_pool();
			return *p;
		}
		pseudo_ptr allocate(){
			pseudo_ptr p(v[0].meta.next);
			cerr<<"allocate cell at index "<<(int)v[0].meta.next<<endl;
			if(v[v[0].meta.next].meta.next==0)
				v[0].meta.next++;
			else
				v[0].meta.next=v[v[0].meta.next].meta.next;
			v[0].meta.n_cells++;
			return p;
		}
		//we could just use the index
		void deallocate(pseudo_ptr p){
			//assume only 1 cell allocated
			cerr<<"deallocate cell at index "<<(int)p.index<<endl;
			v[p.index].meta.next=v[0].meta.next;	
			v[0].meta.next=p.index;
			v[0].meta.n_cells--;
		}
		void* get(INDEX i){return &v[i];}
	};
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	generic_pool* pool_index;//how can we make it optional?
	INDEX index;
	explicit pseudo_ptr(INDEX index):index(index),pool_index(generic_pool::get_instance<T,STORE>()){}
	static pseudo_ptr allocate(){return pseudo_ptr(generic_pool::get_instance<T,STORE>()->template allocate<T>());}
	//only makes sense if S is derived from T and uses same store? will always use different stores!
	template<typename S,typename _STORE_> pseudo_ptr(const pseudo_ptr<S,_STORE_>& p):pool_index(p.pool_index),index(p.index){}
	T* operator->()const{return (T*)pool_index->get(index);}
	pseudo_ptr& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr& operator++(){++index;return *this;}
	pseudo_ptr& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr& p)const{return pool_index!=p.pool_index || index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return pool_index==p.pool_index && index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return pool_index==p.pool_index && index==p.index;}
	operator bool() const{return index;}
	operator void*() {return pool_index->get(index);}
};
template<typename T,typename STORE> pseudo_ptr<T,STORE> operator+(const pseudo_ptr<T,STORE>& a,size_t s){
	pseudo_ptr<T,STORE> tmp=a;
	return tmp+=s;
}
template<typename T,typename STORE> pseudo_ptr<T,STORE> operator-(const pseudo_ptr<T,STORE>& a,size_t s){
	pseudo_ptr<T,STORE> tmp=a;
	return tmp-=s;
}
template<typename T,typename STORE> ptrdiff_t operator-(const pseudo_ptr<T,STORE>& a,const pseudo_ptr<T,STORE>& b){
	return a.index-b.index;
}

template<typename T,size_t SIZE=256> struct free_store{
	//awkward
	static typename pseudo_ptr<T,free_store>::pool* create_pool(){
		char* v=new char[SIZE*sizeof(T)];
		memset(v,0,SIZE*sizeof(T));
		return new typename pseudo_ptr<T,free_store>::pool(v,SIZE);
	}
	static generic_pool* create_generic_pool(){
		char* v=new char[SIZE*sizeof(T)];
		memset(v,0,SIZE*sizeof(T));
		return new generic_pool(v,SIZE,sizeof(T));
		//return new template generic_pool::generic_pool<T>(v,SIZE);
	}
};
template<typename T,size_t SIZE,int NAME> struct persistent_store{
	//use memory map
	static generic_pool* create_generic_pool(){
	//static typename pseudo_ptr<T,persistent_store>::pool* create_pool(){
		ostringstream os;
		os<<"db/pool_"<<NAME;
		string filename=os.str();
		int fd = open(filename.c_str(), O_RDWR | O_CREAT/* | O_TRUNC*/, (mode_t)0600);
		if (fd == -1) {
			cerr<<"\nError opening file `"<<filename<<"' for writing"<<endl;
			exit(EXIT_FAILURE);
		}
		//set the size
		struct stat s;
		int r=fstat(fd,&s);
		if(r==-1){
			cerr<<"\ncould not stat file `"<<filename<<"'"<<endl;
			exit(EXIT_FAILURE);
		}
		size_t file_size=max<size_t>(4096,sizeof(T)*SIZE);
		if(s.st_size<file_size){
			int result = lseek(fd,file_size-1, SEEK_SET);
			if (result == -1) {
				close(fd);
				cerr<<"Error calling lseek() to 'stretch' the file"<<endl;
				exit(EXIT_FAILURE);
			}
			result = write(fd, "", 1);
			if (result != 1) {
				close(fd);
				cerr<<"Error writing last byte of the file"<<endl;
				exit(EXIT_FAILURE);
			}
		}
		char* v = (char*)mmap((void*)NULL,file_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (v == MAP_FAILED) {
			close(fd);
			cerr<<"Error mmapping the file"<<endl;
			exit(EXIT_FAILURE);
		}
		//return new typename pseudo_ptr<T,persistent_store>::pool(v,file_size/sizeof(T));
		return new generic_pool(v,file_size/sizeof(T),sizeof(T));
	}

};
//needed by shared_ptr
namespace special{
	template<typename T,typename STORE> struct deleter<pseudo_ptr<T,STORE>>{
		static void go(pseudo_ptr<T,STORE> p){
			p->~T();
			p.pool_index->deallocate(p);
		}
	};
}

#endif
