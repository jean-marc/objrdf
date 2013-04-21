#ifndef TUPLE_HELPER_H
#define TUPLE_HELPER_H
/*
 *	extra function to help tuple handling
 *
 */
#include <tuple>
namespace std{
	template<typename _T,typename _Tp> struct tuple_index{
		enum{value=0};//we have reached end of tuple
	};
	template<typename _T,typename _Head,typename... _Tail> struct tuple_index<_T, tuple<_Head, _Tail...> >{
		enum{value=1+tuple_index<_T,tuple<_Tail...> >::value};
	};
	template<typename _Head, typename... _Tail> struct tuple_index<_Head, tuple<_Head, _Tail...> >{
		enum{value=0};
	};
	template<typename _Tp,typename F> struct static_for_each_impl{
		static F go(F f){return f;}//no-op
	};
	template<typename _Head,typename... _Tail,typename F> struct static_for_each_impl<tuple<_Head,_Tail...>,F>{
		static F go(F f){
			f.template operator()<_Head>();
			return static_for_each_impl<tuple<_Tail...>,F>::go(f);
			//return f;
		}
	};
	template<typename _Tp,typename F> F static_for_each(F f){return static_for_each_impl<_Tp,F>::go(f);}
}
#endif
