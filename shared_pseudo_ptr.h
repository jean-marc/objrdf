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
#include <stdexcept>
/*
 *	will only work in objrdf framework, not necessarily
 */
#ifdef OBJRDF_VERB
#include <iostream>
#endif
#include <sstream>
using namespace std;
namespace pseudo{
	enum{N=1024};
	typedef char POOL_INDEX;
	struct pool{
		/*
		*	one pool per type, they could all have different size
		*	the allocator does not depend on the type, it just sees
		*	it as size-byte word. 
		*/
		struct next_free{
			//we cannot use ptr because the structure will be persisted
			//we have to make sure it fits into the cell
			//also it should not overlap base_resource::n because that tells us if a cell is used or not
			size_t i;
			size_t n_cells;//total number of cells
			next_free():i(0),n_cells(0){}

		};
		//too bad because can not optimize
		/*const*/ size_t size;
		char* v;
		POOL_INDEX p;
		int fd;
		inline next_free* root(){return (next_free*)(void*)v;}
		template<typename T> pool(POOL_INDEX):size(sizeof(T)),v(0),fd(-1){

		}
		pool(size_t size,POOL_INDEX p,string filename):size(size),v(0),p(p),fd(-1)/*,v(new char[N*size])*/{
			//creates the file if it does not exist
			fd = open(filename.c_str(), O_RDWR | O_CREAT/* | O_TRUNC*/, (mode_t)0600);
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
			size_t file_size=max<size_t>(4096,size*N);
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
			v = (char*)mmap((void*)NULL, N*size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (v == MAP_FAILED) {
				close(fd);
				cerr<<"Error mmapping the file"<<endl;
				exit(EXIT_FAILURE);
			}
			//sanity check (checksum?) otherwise it will crash and burn
			/*
 			*	what to do about duplication???
 			*/ 
			//memset(v,0,N*size);//it has to be zeroed for scheme to work
			//VERY IMPORTANT!
			if(root()->i==0) {
				root()->i=size;
				root()->n_cells=0;
			}
			std::cerr<<" size "<<size<<" @"<<(void*)v<<std::endl;
			std::cerr<<root()->n_cells<<" cells used"<<endl;
			//the memory is not quite ready yet: the vtable pointer is no longer valid, it will create segmentation fault
			//how do we know what the current vtable pointer is? we could create an instance but dangerous: it will 
			//call `allocate'!
		}
		~pool(){
		}
		char* get(size_t index){return v+index*size;}
		template<typename T> void* allocate(size_t s){
			if(root()->n_cells>N){
				throw std::runtime_error("pool full");
				//could also return NULL
			}
			char* n=v+root()->i;
			//could we use it to allocate contiguous blocks?
			std::cerr<<"allocate "<<s<<" bytes from pool "<<(int)p<<" size "<<size<<" @"<<(void*)n<<" "<<(n-v)<<std::endl;
			if(((next_free*)(void*)(v+root()->i))->i==0){
				root()->i+=size;//next cell
			}else{
				root()->i=((next_free*)(void*)(v+root()->i))->i;
			}
			root()->n_cells++;
			return n;
		}
		/*
 		*	when releasing it copies information to the cell about next available empty cell
 		*/ 
		template<typename T> void release(void* p){
			// we are not using the type
			static_cast<next_free*>(p)->i=root()->i;
			root()->i=(char*)p-v;	
			root()->n_cells--;
		}
		void output(ostream& os){
			os<<"********";
			os.write(v,N*size);
		}
	};
	/*
 	*	we want storage like
 	*	db/rdf/Class;
 	*	to be implemented (with specialization) in objrdf.h
 	*/ 
	template<typename T> string filename();//{return "??";}
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
			v.push_back(pool(sizeof(T),p,"db/"+filename<T>()));
			return p;
		}
		template<typename T> inline const POOL_INDEX get_pool_index(){
			static const POOL_INDEX p=init<T>();
			return p;	
		}
		template<typename T> void * allocate(size_t s){
			//std::cerr<<"allocate "<<s<<" bytes from pool "<<(int)get_pool_index<T>();
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
	//pseudo_ptr should be drop-in replacement
	template<typename T> class shared_ptr{
	/*
 	*	how do we persist data if the goal of the shared_pointer is to garbage collect?
 	*	just increase base_resource::n
 	*
 	*/
	/*
 	*	specialize for property, no derivation for now so no need to keep a pool index
 	*	also no need for reference counting
 	*/ 
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
			index=((char*)(t) - pools::get_instance().get(pool_index,0))/pools::get_instance().v[pool_index].size;
			std::cerr<<"shared_ptr "<<this<<"\t"<<t<<"\t{"<<(int)pool_index<<","<<index<<"}"<<std::endl;
			if(index) ++get()->n;
		}
		inline T* get() const{return index ? (T*)pools::get_instance().get(pool_index,index):0;}
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
		//a bit more complicated
		shared_ptr<S> tmp;
		tmp.pool_index=s.pool_index;
		tmp.index=s.index;
		return tmp;
		//return shared_ptr<S>(static_cast<S*>(s.get()));
	} 
}
#endif
