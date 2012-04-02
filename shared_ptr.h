#ifndef SHARED_PTR_H
#define SHARED_PTR_H
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
namespace special{
	template<typename T> struct deleter;
}
#ifdef PERSISTENT
#include "pseudo_ptr.h"

//needed by shared_ptr
namespace special{
	template<typename T,typename STORE,bool POLYMORPHISM> struct deleter<pseudo_ptr<T,STORE,POLYMORPHISM>>{
		static void go(pseudo_ptr<T,STORE,POLYMORPHISM> p){
			p.destroy();
			//p->~T();//that's assuming the destructor is virtual
			//p.pool_index->deallocate(p);
		}
	};
}
#endif
namespace special{
	template<typename T> struct storage{
		static void* get(size_t s){return ::operator new(s);}
		static void release(void* p){::operator delete(p);}
	};
	/*
 	*	can we make it work with pseudo_ptr?
 	*/
	template<typename T> struct deleter<T*>{
		static void go(T* t){delete t;}
	};
	template<
		typename T,
		#ifdef PERSISTENT
		//typename _pointer_=pseudo_ptr<T,free_store<256>,true>,
		typename _pointer_=pseudo_ptr<T,persistent_store,true>,
		//typename _pointer_=pseudo_ptr<T,free_store,true>,
		#else
		typename _pointer_=T*,
		#endif
		typename _reference_=T&
	> class shared_ptr{
	public:
		typedef T element_type;
		typedef _pointer_ pointer;
		typedef _reference_ reference;
	private:
		pointer t;
	public:
		shared_ptr():t(0){}
		explicit shared_ptr(pointer t):t(t){
			//std::cerr<<"shared_ptr "<<this<<"\t"<<t<<std::endl;
			if(t) ++t->n;
		}
		inline pointer operator->() const{return t;}
		inline pointer get() const{return t;}
		inline reference operator*() const{return *t;}
		int use_count() const{return t ? t->n : 0;}
		shared_ptr(const shared_ptr& s):t(s.t){
			//std::cerr<<"shared_ptr "<<this<<"\t"<<t<<std::endl;
			if(t) ++(t->n);
		}
		template<typename S> shared_ptr(const shared_ptr<S>& s):t(s.get()){
			s.get().print(std::cerr);
			t.print(std::cerr);
			if(t) ++(t->n);
		}
		shared_ptr& operator=(const shared_ptr& s){
			/*
 			*	works also if `this' and `s' point to same object 
 			*/	
			if(t){
				#ifdef OBJRDF_VERB
				std::cerr<<(void*)t<<"\tattempting to delete `"<<t->id<<"'\t"<<t->n<<std::endl;
				#endif
				--(t->n);
				if(t->n==0){
					//delete t;
					deleter<pointer>::go(t);
					#ifdef OBJRDF_VERB
					std::cerr<<"deleted"<<std::endl;
					#endif
				}
			}
			t=s.t;
			if(t) ++(t->n);
			return *this;
		}
		bool operator==(const shared_ptr & s) const{return t==s.t;}
		bool operator!=(const shared_ptr & s) const{return t!=s.t;}
		bool operator!()const{return t==0;}
		operator bool()const{return t;}
		~shared_ptr(){
			if(t){
				#ifdef OBJRDF_VERB
				t.print(std::cerr);
				std::cerr<<(void*)t<<"\tattempting to delete `"<<t->id<<"'\t"<<t->n<<std::endl;
				#endif
				--(t->n);
				//there should be cycle detection otherwise infinite loop
				if(t->n==0){
					//what if not allocated on the free store
					//std::shared_ptr uses a `deleter' class 
					//delete t;
					deleter<pointer>::go(t);
					#ifdef OBJRDF_VERB
					std::cerr<<"deleted from "<<this<<std::endl;
					#endif
				}
			}
		}
	};
	//needs to be specialized
	template<typename S,typename T> shared_ptr<S> static_pointer_cast(const shared_ptr<T>& s){
		return shared_ptr<S>(static_cast<typename shared_ptr<S>::pointer>(s.get()));
	} 
}
#endif
