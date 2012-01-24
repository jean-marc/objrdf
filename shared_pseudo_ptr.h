#ifndef SHARED_PSEUDO_PTR_H
#define SHARED_PSEUDO_PTR_H
#include "objrdf.h"
#include <vector>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
/*
 *	will only work in objrdf framework
 *
 */
/*
	a generic shared pointer , the type T MUST have a member int T::n 
	intrusive reference counting pg167 `Modern C++ design' Alexandrescu 
*/
/*
 *	since it is customized anyway we could have some special behaviour when
 *	n reaches 1 or 2, because it might mean that the resource is only referenced
 *	by the RDF document (in the map and the vector) and it is ready for 
 *	deletion. It might be desirable to leave a resource dangling for later reuse.
 *
 */
#ifdef OBJRDF_VERB
#include <iostream>
#endif
#include <sstream>
using namespace std;
namespace pseudo{
	enum{N=64};
	typedef char POOL_INDEX;
	struct pool{
		/*
		*	one pool per type, they could all have different size
		*	the allocator does not depend on the type, it just sees
		*	it as size-byte word. 
		*/
		struct next_free{
			//we cannot use ptr because the structure will be persisted
			//char* ptr;//if 0 then next cell is assumed to be good
			//we have to make sure it fits into the cell
			size_t i;
			next_free():i(0){}
		};
		/*const*/ size_t size;
		char* v;
		int fd;
		/*size_t next_index;*/
		inline next_free* root(){return (next_free*)(void*)v;}
		pool(size_t size,POOL_INDEX p):size(size)/*,v(new char[N*size])*//*,next_index(1)*/{
			//can use memory map
			ostringstream os;
			os<<"db/_"<<(unsigned int)p;
			fd = open(os.str().c_str(), O_RDWR /*| O_CREAT | O_TRUNC*/, (mode_t)0600);
			if (fd == -1) {
				perror("Error opening file for writing");
				exit(EXIT_FAILURE);
			}
			v = (char*)mmap((void*)NULL, N*size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			cerr<<" mmap:"<<(void*)v<<endl;
			if (v == MAP_FAILED) {
				close(fd);
				perror("Error mmapping the file");
				exit(EXIT_FAILURE);
			}
			//v[0]=0xab;	
			//memset(v,0,N*size);//it has to be zeroed for scheme to work
			//VERY IMPORTANT!
			if(root()->i==0) root()->i=size;
			std::cerr<<" size "<<size<<" @"<<(void*)v<<std::endl;
			//the memory is not ready yet: the vtable pointer is no longer valid, it will create segmentation fault
			//how do we know what the current vtable pointer is? we could create an instance but dangerous: it will 
			//call allocate!
		}
		~pool(){
		}
		char* get(size_t index){return v+index*size;}
		template<typename T> void* allocate(size_t s){
			char* n=v+root()->i;
			std::cerr<<" size "<<size<<" @"<<(void*)n<<" "<<(n-v)<<std::endl;
			if(((next_free*)(void*)(v+root()->i))->i==0){
				root()->i+=size;//next cell
			}else{
				root()->i=((next_free*)(void*)(v+root()->i))->i;
			}
			//std::cerr<<" size "<<size<<" @"<<(T*)v+next_index<<" "<<next_index<<std::endl;
			//return (T*)v+next_index++;
			return n;
		}
		/*
 		*	when releasing it copies information to the cell about next available empty cell
 		*	or top of stack, we can use cell[0] to store information
 		*/ 
		template<typename T> void release(void* p){
			static_cast<next_free*>(p)->i=root()->i;
			root()->i=(char*)p-v;	
		}
		void output(ostream& os){
			os<<"********";
			os.write(v,N*size);
		}
	};
	struct pools{
		/*
 		*	could eventually replace rdf::RDF
 		*
 		*/ 
		typedef pseudo::POOL_INDEX POOL_INDEX;
		std::vector<pool> v;
		pools(){
			std::cerr<<"create pools..."<<std::endl;
		}
		template<typename T> const POOL_INDEX init(){
			POOL_INDEX p=v.size();
			std::cerr<<"initialize pool "<<(int)p;
			//let's create instance to figure out the pointer
			//we can create on the stack
			//char tmp[sizeof(T)];
			v.push_back(pool(sizeof(T),p));
			return p;
		}
		template<typename T> inline const POOL_INDEX get_pool_index(){
			static const POOL_INDEX p=init<T>();
			return p;	
		}
		template<typename T> void * allocate(size_t s){
			std::cerr<<"allocate "<<s<<" bytes from pool "<<(int)get_pool_index<T>();
			return v[get_pool_index<T>()].allocate<T>(s);
		}
		template<typename T> void release(void *p){v[get_pool_index<T>()].release<T>(p);}
		template<typename T> T* get(){return (T*)v[get_pool_index<T>()].v;}
		char* get(POOL_INDEX pool_index,int index){return v[pool_index].get(index);}
		static pools& get_instance(){
			static pools* p=new pools();
			return *p;
		} 
		/*
 		*	we can dump the whole document in binary
 		*/ 
		void output(ostream& os){
			for(auto i=v.begin();i<v.end();++i){
				i->output(os);
			}	
		}
		/*
 		*	we can also serialize as a RDF document
 		*/ 
	};
	template<typename T> class shared_ptr{
	protected:
		/*
 		*	the index type will depend on the number of instances
 		*/
	public:
		pools::POOL_INDEX pool_index;
		unsigned short index;
		typedef T element_type;
		shared_ptr():pool_index(0),index(0){}
		explicit shared_ptr(T* t):pool_index(t->pool_index){
			//it could belong to different pools
			//how can we tell the type?
			//we can use get_Class virtual function, would be nice to have a member in base_resource
			//we can use get_Class()->get<c_index> for now
			//t* is supposed to be stored inside an array at storage<T>::v
			//things will go very bad if t is not of type T
			//
			/*
 			*	first we have to establish if t is of type T
 			*/ 
			//would be better if the index was given by
			index=((char*)(t) - pools::get_instance().get(pool_index,0))/pools::get_instance().v[pool_index].size;
			std::cerr<<"shared_ptr "<<this<<"\t"<<t<<"\t{"<<(int)pool_index<<","<<index<<"}"<<std::endl;
			if(index) ++get()->n;
		}
		inline T* get() const{return (T*)pools::get_instance().get(pool_index,index);}
		inline T* operator->() const{return get();}
		inline T& operator*() const{return *get();}
		int use_count() const{return index ? get()->n : 0;}
		shared_ptr(const shared_ptr& s):pool_index(s.pool_index),index(s.index){
			//std::cerr<<"shared_ptr "<<this<<"\t"<<t<<std::endl;
			std::cerr<<"{"<<(int)pool_index<<","<<index<<"}"<<std::endl;
			if(index) ++(get()->n);
		}
		template<typename S> shared_ptr(const shared_ptr<S>& s):pool_index(s.pool_index),index(s.index){
			std::cerr<<"{"<<(int)pool_index<<","<<index<<"}"<<std::endl;
			if(index) ++(get()->n);
		}
		shared_ptr& operator=(const shared_ptr& s){
			/*
 			*	works also if `this' and `s' point to same object 
 			*/	
			if(index){
				#ifdef OBJRDF_VERB
				std::cerr<<"{"<<(int)pool_index<<","<<index<<"}\t"<<get()<<"\tattempting to delete `"<<get()->id<<"'\t"<<get()->n<<std::endl;
				#endif
				--(get()->n);
				if(get()->n==0){
					delete get();
					#ifdef OBJRDF_VERB
					std::cerr<<"deleted"<<std::endl;
					#endif
				}
			}
			pool_index=s.pool_index;
			index=s.index;
			if(index) ++(get()->n);
			return *this;
		}
		bool operator==(const shared_ptr & s) const{return index==s.index && pool_index==s.pool_index;}
		bool operator!=(const shared_ptr & s) const{return index!=s.index || pool_index!=s.pool_index;}
		bool operator!()const{return index==0;}
		operator bool()const{return index;}
		~shared_ptr(){
			if(index){
				#ifdef OBJRDF_VERB
				std::cerr<<"{"<<(int)pool_index<<","<<index<<"}\t"<<get()<<"\tattempting to delete `"<<get()->id<<"'\t"<<get()->n<<std::endl;
				#endif
				--(get()->n);
				if(get()->n==0){
					delete get();
					#ifdef OBJRDF_VERB
					std::cerr<<"deleted from "<<this<<std::endl;
					#endif
				}
			}
		}
	};
	template<typename S,typename T> shared_ptr<S> static_pointer_cast(const shared_ptr<T>& s){
		return shared_ptr<S>(static_cast<S*>(s.get()));
	} 
}
#endif
