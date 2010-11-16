#ifndef SHARED_PTR_H
#define SHARED_PTR_H
#define SHARED_PTR_H_IMPL_IN_HEADER
/*
	a generic shared pointer , the type T MUST have a member int T::n 
	intrusive reference counting pg167 `Modern C++ design' Alexandrescu 
*/
namespace objrdf{
	template<typename T> class shared_ptr{
	private:
		T* t;
	public:
		shared_ptr();//:t(0){}
		shared_ptr(T* _t);/*{
			t=_t;
			if(t) ++t->n;
		}*/
		inline T* operator->() const;//{return t;}
		inline T* get() const;//{return t;}
		inline T& operator*() const;//{return *t;}
		//we need this one!!!
		shared_ptr(shared_ptr const & s);/*{
			t=s.t;
			if(t) ++(t->n);
		}*/
		//S must derive from T
		//we need this one in the header file
		template<typename S> shared_ptr(const shared_ptr<S> & s);/*{
			//t=s.t;
			t=s.operator->();
			if(t) ++(t->n);
		}*/
		shared_ptr& operator=(const shared_ptr& s);/*{
			//should check if it is not the same
			if(t){
				--(t->n);
				if(t->n==0) delete t;
			}
			t=s.t;
			if(t) ++(t->n);
			return *this;
		}*/
		template<typename S> shared_ptr& operator=(const shared_ptr<S>& s);/*{
			if(t){
				--(t->n);
				if(t->n==0) delete t;
			}
			//t=s.t;
			t=s.operator->();
			if(t) ++(t->n);
			return *this;
		}*/
		bool operator==(const shared_ptr & s) const;//{return t==s.t;}
		bool operator!=(const shared_ptr & s) const;//{return t==s.t;}
		bool operator!() const;//{return t==0;}
		~shared_ptr();/*{
			if(t){
				--(t->n);
				if(t->n==0) delete t;
			}
		}*/
	};
	#ifdef SHARED_PTR_H_IMPL_IN_HEADER
	template<typename T> shared_ptr<T>::shared_ptr():t(0){}
	template<typename T> shared_ptr<T>::shared_ptr(T* _t){
		t=_t;
		if(t) ++t->n;
	}
	template<typename T> inline T* shared_ptr<T>::operator->() const{return t;}
	template<typename T> inline T* shared_ptr<T>::get() const{return t;}
	template<typename T> inline T& shared_ptr<T>::operator*() const{return *t;}
	//we need this one!!!
	template<typename T> shared_ptr<T>::shared_ptr(shared_ptr const & s){
		t=s.t;
		if(t) ++(t->n);
	}
	//S must derive from T
	template<typename T> template<typename S> shared_ptr<T>::shared_ptr(const shared_ptr<S> & s){
		//t=s.t;
		t=static_cast<T*>(s.operator->());
		if(t) ++(t->n);
	}
	template<typename T> shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr& s){
		//should check if it is not the same
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
		t=s.t;
		if(t) ++(t->n);
		return *this;
	}
	template<typename T> template<typename S> shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr<S>& s){
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
		//t=s.t;
		t=s.operator->();
		if(t) ++(t->n);
		return *this;
	}
	template<typename T> bool shared_ptr<T>::operator==(const shared_ptr<T> & s) const{return t==s.t;}
	template<typename T> bool shared_ptr<T>::operator!=(const shared_ptr<T> & s) const{return t!=s.t;}
	template<typename T> bool shared_ptr<T>::operator!() const{return t==0;}
	template<typename T> shared_ptr<T>::~shared_ptr(){
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
	}
	#endif
}
#endif
