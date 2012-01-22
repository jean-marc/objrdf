#ifndef SHARED_PSEUDO_PTR_H
#define SHARED_PSEUDO_PTR_H
#include "objrdf.h"
#include <vector>
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
using namespace std;
namespace pseudo{
	//one storage per type
	//will create a single binary file for the whole db
	//need to allocate a big chunk of memory
	//keeps track of all the stores
	enum{N=32};
	struct pool{
		/*const*/ size_t size;
		char* v;
		size_t next_index;
		pool(size_t size):size(size),v(new char[N*size]),next_index(1){
			memset(v,0xff,N*size);
			std::cerr<<" size "<<size<<" @"<<(void*)v<<std::endl;
		}
		char* get(size_t index){return v+index*size;}
		template<typename T> void* allocate(size_t s){
			std::cerr<<" size "<<size<<" @"<<(T*)v+next_index<<" "<<next_index<<std::endl;
			return (T*)v+next_index++;
		}
		template<typename T> void release(void* p){}
		void output(ostream& os){
			os<<"********";
			os.write(v+size,next_index*size);
		}
	};
	struct pools{
		typedef char POOL_INDEX;
		std::vector<pool> v;
		pools(){
			std::cerr<<"create pools..."<<std::endl;
		}
		template<typename T> const POOL_INDEX init(){
			POOL_INDEX p=v.size();
			std::cerr<<"initialize pool "<<(int)p;
			v.push_back(pool(sizeof(T)));
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
