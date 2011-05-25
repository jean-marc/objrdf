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
#ifdef VERBOSE
#include <iostream>
#endif
namespace special{
	template<typename T> class shared_ptr{
	private:
		T* t;
	public:
		typedef T element_type;
		shared_ptr():t(0){}
		explicit shared_ptr(T* t):t(t){
			if(t) ++t->n;
		}
		inline T* operator->() const{return t;}
		inline T* get() const{return t;}
		inline T& operator*() const{return *t;}
		int use_count() const{return t ? t->n : 0;}
		shared_ptr(const shared_ptr& s):t(s.t){
			if(t) ++(t->n);
		}
		template<typename S> shared_ptr(const shared_ptr<S>& s):t(s.get()){
			if(t) ++(t->n);
		}
		shared_ptr& operator=(const shared_ptr& s){
			/*
 			*	works also if `this' and `s' point to same object 
 			*/	
			if(t){
				#ifdef VERBOSE
				std::cerr<<t<<"\tattempting to delete `"<<t->id<<"'\t"<<t->n<<std::endl;
				#endif
				--(t->n);
				if(t->n==0){
					delete t;
					#ifdef VERBOSE
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
				#ifdef VERBOSE
				std::cerr<<t<<"\tattempting to delete `"<<t->id<<"'\t"<<t->n<<std::endl;
				#endif
				--(t->n);
				if(t->n==0){
					delete t;
					#ifdef VERBOSE
					std::cerr<<"deleted"<<std::endl;
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
