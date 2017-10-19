#ifndef INTROSPECTION_H
#define INTROSPECTION_H
#include "objrdf.h"
/*
*	separates introspection from definitions
*/

namespace objrdf{
	#ifndef NEW_FUNC_TABLE 
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct base_f{
		typedef PROPERTY PP;
		/*
 		*	how could we reduce the number of created functions?
 		*		remove dependency on SUBJECT and PROPERTY, should only depend on PROPERTY::RANGE
 		*		if we know the offsetof the property we can just cast void* ptr (used to do that
 		*		in old implementation)
 		*
 		*/ 
		static inline PROPERTY& get(RESOURCE_PTR subject,size_t){
			#ifdef NATIVE
			return static_cast<SUBJECT*>(subject)->template get<PROPERTY>();
			#else
			//why is it derived?
			return static_cast<typename SUBJECT::allocator_type::generic_pointer>(subject)->template get<PROPERTY>();
			#endif
		}
		static inline const PROPERTY& get_const(CONST_RESOURCE_PTR subject,size_t){
			#ifdef NATIVE
			return static_cast<const SUBJECT*>(subject)->template cget<PROPERTY>();
			#else
			//why is it derived?
			return static_cast<typename SUBJECT::allocator_type::const_generic_pointer>(subject)->template cget<PROPERTY>();
			#endif
		}
		static size_t get_size(CONST_RESOURCE_PTR subject){return get_const(subject,0).get_size();}
		struct normal{
			static void erase(RESOURCE_PTR subject,size_t first,size_t last){
				get(subject,0).erase();
			}	
		};
		struct version{
			#ifndef NATIVE
			static void erase(RESOURCE_PTR subject,size_t first,size_t last){
				RESOURCE_PTR old=clone_and_swap(subject);//now subject points to the cloned resource
				ostringstream os;
				old._print(os);
				old->id=uri(subject->id.ns(),string(subject->id.local)+"."+os.str());
				//obj::next is wrong when more than 2 generations, should only use obj::prev for now
				static_cast<typename SUBJECT::allocator_type::generic_pointer>(subject)->template get<objrdf::prev>().set_object(old);
				static_cast<typename SUBJECT::allocator_type::generic_pointer>(old)->template get<objrdf::next>().set_object(subject);
				get(subject,0).erase();
			}	
			#endif
		};
		typedef typename IfThenElse<std::is_same<typename SUBJECT::VERSION,PROPERTY>::value,version,normal>::ResultT ERASE;
		static PROVENANCE get_provenance(CONST_RESOURCE_PTR subject,size_t){return 0;/*get_const(subject).p;*/}
		static function_table get_table(){
			function_table t;
			t.get_size=get_size;
			t.add_property=function_table::default_f::add_property_def;
			t.erase=ERASE::erase;
			t.get_provenance=get_provenance;
			return t;	
		}
	};
	template<
		typename SUBJECT,
		typename PROPERTY,
		typename ALLOCATOR
	> struct base_f<SUBJECT,objrdf::array<PROPERTY,ALLOCATOR>>{
		typedef objrdf::array<PROPERTY,ALLOCATOR> ARRAY;
		typedef PROPERTY PP;
		//why is it generic_pointer?
		static inline ARRAY& get(RESOURCE_PTR subject){
			#ifdef NATIVE
			return static_cast<SUBJECT*>(subject)->template get<ARRAY>();
			#else
			return static_cast<typename SUBJECT::allocator_type::generic_pointer>(subject)->template get<ARRAY>();
			#endif
		}
		static inline const ARRAY& get_const(CONST_RESOURCE_PTR subject){
			#ifdef NATIVE
			return static_cast<const SUBJECT*>(subject)->template get_const<ARRAY>();
			#else
			return static_cast<typename SUBJECT::allocator_type::const_generic_pointer>(subject)->template get_const<ARRAY>();
			#endif
		}
		static inline PROPERTY& get(RESOURCE_PTR subject,size_t index){
			#ifdef NATIVE
			return static_cast<SUBJECT*>(subject)->template get<ARRAY>()[index];
			#else
			return static_cast<typename SUBJECT::allocator_type::generic_pointer>(subject)->template get<ARRAY>()[index];
			#endif
		}
		static inline const PROPERTY& get_const(CONST_RESOURCE_PTR subject,size_t index){
			#ifdef NATIVE
			return static_cast<const SUBJECT*>(subject)->template get_const<ARRAY>()[index];
			#else
			return static_cast<typename SUBJECT::allocator_type::const_generic_pointer>(subject)->template get_const<ARRAY>()[index];
			#endif
		}
		static size_t get_size(CONST_RESOURCE_PTR subject){
			return get_const(subject).size();
		}
		static void add_property(RESOURCE_PTR subject,PROVENANCE p){
			typedef PROPERTY P;
			get(subject).push_back(P());
		}
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){
			get(subject).erase(get(subject).begin()+first,get(subject).begin()+last);
		}
		static PROVENANCE get_provenance(CONST_RESOURCE_PTR subject,size_t index){
			return 0;
		}
		static function_table get_table(){
			function_table t;
			t.get_size=get_size;
			t.add_property=add_property;
			t.erase=erase;
			t.get_provenance=get_provenance;
			return t;	
		}
	};
	template<
		typename SUBJECT,
		typename PROPERTY,
		size_t TYPE=PROPERTY::TYPE&0x7
	> struct functions;

	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONSTP|LITERAL>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			/*
 			*	at this stage we can detect if out has been overriden in a base class
 			*/ 
			BASE::get_const(subject,index).out(os);
		}	
		static int compare(CONST_RESOURCE_PTR a,size_t index_a,CONST_RESOURCE_PTR b,size_t index_b){
			return BASE::get_const(a,index_a).compare(BASE::get_const(b,index_b));
		}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.out=out;
			t.compare=compare;
			return t;
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,LITERAL>:functions<SUBJECT,PROPERTY,CONSTP|LITERAL>{
		typedef functions<SUBJECT,PROPERTY,CONSTP|LITERAL> BASE;
		static void in(RESOURCE_PTR subject,istream& is,size_t index){
			BASE::get(subject,index).in(is);
		}
		template<typename LEAF> struct trigger{
			static void in(RESOURCE_PTR subject,istream& is,size_t index){
				PROPERTY tmp;
				tmp.in(is);
				//add pointer because `this' cannot be converted to pointer
				static_cast<typename LEAF::allocator_type::generic_pointer>(subject)->set_p(tmp,static_cast<typename LEAF::allocator_type::generic_pointer>(subject));
			}
			static function_table patch(function_table t){
				t.add_property=function_table::default_f::add_property_def;
				t.in=in;
				return t;
			}
		};
		static function_table get_table(){
			auto t=BASE::get_table();//we could drop T here
			t.in=in;
			return t;	
		}
	};
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,STRING|LITERAL>:functions<SUBJECT,PROPERTY,LITERAL>{
		typedef functions<SUBJECT,PROPERTY,LITERAL> BASE;
		static void set_string(RESOURCE_PTR subject,string s,size_t index){BASE::get(subject,index).set_string(s);}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.set_string=set_string;
			return t;	
		}
	};
	//what if set_object is invoked on const property?
	static void set_const_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){LOG_ERROR<<"error: const property"<<std::endl;}
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,CONSTP>:base_f<SUBJECT,PROPERTY>{
		typedef base_f<SUBJECT,PROPERTY> BASE;
		static CONST_RESOURCE_PTR get_const_object(CONST_RESOURCE_PTR subject,size_t index){
			return BASE::get_const(subject,index).get_const_object();
		}
		static void set_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){
			BASE::get(subject,index).set_object(object);
		}
		template<typename LEAF> struct trigger{
			//let's override add_property so it does not modify 
			//it is now the trigger's responsibility to add property
			static void set_object(RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){
				typename BASE::PP tmp;
				tmp.set_object(object);
				//let's try to use SQL terminology instead_of_insert trigger?
				static_cast<typename LEAF::allocator_type::generic_pointer>(subject)->set_p(
					tmp,
					static_cast<typename LEAF::allocator_type::generic_pointer>(subject)
				);
			}
			static void erase(RESOURCE_PTR subject,size_t first,size_t last){
				assert(last==first+1);//only one property at a time
				static_cast<typename LEAF::allocator_type::generic_pointer>(subject)->instead_of_delete(
					//BASE::get(subject).begin()+first,//iterator would be nice, or pointer?
					BASE::get(subject,first),//not happy about this one
					static_cast<typename LEAF::allocator_type::generic_pointer>(subject)	
				);
			}
			static function_table patch(function_table t){
				t.erase=erase;
				t.add_property=function_table::default_f::add_property_def;
				t.set_object=set_object;
				return t;
			}
		};
		static function_table get_table(){
			auto t=BASE::get_table();
			t.cget_object=get_const_object;
			t.set_object=set_object;//???
			return t;	
		}
	};	
	template<typename SUBJECT,typename PROPERTY> struct functions<SUBJECT,PROPERTY,0>:functions<SUBJECT,PROPERTY,CONSTP>{
		typedef functions<SUBJECT,PROPERTY,CONSTP> BASE;
		static RESOURCE_PTR get_object(RESOURCE_PTR subject,size_t index){return BASE::get(subject,index).get_object();}
		static function_table get_table(){
			auto t=BASE::get_table();
			t.get_object=get_object;
			return t;	
		}
	};
	/*
 	*	properties are constant, how can we tell the parser?
 	*	one way is to group constant properties and put offset in type_iterator, but awkward with derived classes,
 	*	we could also return a fake property so that parsing can be carried on but will not modify the document
 	*	we can just sort by constness: interesting idea...
 	*
 	*/
	template<
		typename SUBJECT,
		typename NAMESPACE,
		typename NAME,
		typename BASE_PROPERTY
	> struct functions<SUBJECT,property<NAMESPACE,NAME,NIL,BASE_PROPERTY>,LITERAL>{
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			property<NAMESPACE,NAME,NIL,BASE_PROPERTY> tmp;
			static_cast<typename SUBJECT::allocator_type::const_generic_pointer>(subject)->out_p(tmp,os);
		}
		static function_table get_table(){
			function_table t;
			t.out=out;
			t.set_object=set_const_object;
			t.get_size=function_table::default_f::always_1;
			return t;
		}
	};
	template<
		typename SUBJECT,
		typename NAMESPACE,
		typename NAME,
		typename RANGE
	> struct functions<SUBJECT,property<NAMESPACE,NAME,RANGE,NIL>,LITERAL>{
		/*static void set_string(RESOURCE_PTR subject,string s,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<SUBJECT*>(subject)->set_string_p(tmp,s);//won't find function if in super-class
		}*/
		static void in(RESOURCE_PTR subject,istream& is,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<typename SUBJECT::allocator_type::generic_pointer>(subject)->in_p(tmp,is);//won't find function if in super-class
		}
		static void out(CONST_RESOURCE_PTR subject,ostream& os,size_t index){
			property<NAMESPACE,NAME,RANGE,NIL> tmp;//very awkward
			static_cast<typename SUBJECT::allocator_type::const_generic_pointer>(subject)->out_p(tmp,os);
		}
		static void erase(RESOURCE_PTR subject,size_t first,size_t last){}//idem
		static function_table get_table(){
			function_table t;
			t.in=in;
			t.out=out;
			//t.set_object=set_const_object;
			t.get_size=function_table::default_f::always_1;
			t.add_property=function_table::default_f::add_property_def;
			t.erase=erase;
			return t;
		}
	};

	#else
	template<
		char TYPE,/* 0-F */
		typename BASE_PROPERTY
	> struct get_ftable;	

	template<typename BASE_PROPERTY> struct get_ftable<LITERAL,BASE_PROPERTY>{
		static function_table go(){
			function_table t;
			t.in_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,istream& is,size_t index){
				is>>static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->t;
			};
			t.out_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,ostream& os,size_t index){
				//cerr<<"address:"<<static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))<<endl;
				os<<static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->t;
			};
			t.get_size_generic=function_table::default_f::always_1;
			t.add_property_generic=function_table::default_f::add_property_generic_def;
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<LITERAL|STRING,BASE_PROPERTY>{
		static function_table go(){
			function_table t;
			t.set_string_generic=[](RESOURCE_PTR subject,std::string s,ptrdiff_t offset,size_t index){
				static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->set_string(s);
			};
			t.in_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,istream& is,size_t index){
				is>>static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->t;
			};
			t.out_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,ostream& os,size_t index){
				os<<static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->t;
			};
			t.get_size_generic=function_table::default_f::always_1;
			t.add_property_generic=function_table::default_f::add_property_generic_def;
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<0,BASE_PROPERTY>{//pointer
		static function_table go(){
			function_table t;
			t.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->get_const_object();
			};
			t.get_object_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->get_object();
			};
			t.set_object_generic=[](RESOURCE_PTR subject,RESOURCE_PTR object,ptrdiff_t offset,size_t index){
				static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->set_object(object);
			};
			t.get_size_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset){
				return static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->get_size();
			};
			t.add_property_generic=function_table::default_f::add_property_generic_def;
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<LOCAL,BASE_PROPERTY>{//local storage
		static function_table go(){
			function_table t;
			t.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->get_const_object();
			};
			t.get_object_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->get_object();
			};
			t.get_size_generic=function_table::default_f::always_1;
			t.add_property_generic=function_table::default_f::add_property_generic_def;
			return t;
		}
	};

	template<typename BASE_PROPERTY> struct get_ftable<CONSTP,BASE_PROPERTY>{//const pointer
		static function_table go(){
			function_table t;
			t.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return (CONST_RESOURCE_PTR) static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->t;
			};
			t.get_size_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset){
				return size_t(static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset))->t!=0);
			};
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<LITERAL|ARRY,BASE_PROPERTY>{//array of literals
		static function_table go(){
			function_table t;
			t.in_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,istream& is,size_t index){
				is>>(*static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset)))[index].t;
			};
			t.out_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,ostream& os,size_t index){
				os<<(*static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))[index].t;
			};
			t.get_size_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset){
				return (static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))->size();
			};
			t.add_property_generic=[](RESOURCE_PTR subject,ptrdiff_t offset){
				static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->push_back(typename BASE_PROPERTY::value_type());
			};
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<CONSTP|ARRY,BASE_PROPERTY>{//array of const pointer
		static function_table go(){
			function_table t;
			t.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return (CONST_RESOURCE_PTR)(*static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))[index].t;
			};
			t.get_size_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset){
				return (static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))->size();
			};
			return t;
		}
	};
	template<typename BASE_PROPERTY> struct get_ftable<ARRY,BASE_PROPERTY>{//array of pointer
		static function_table go(){
			function_table t;
			t.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return (CONST_RESOURCE_PTR)(*static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))[index].t;
			};
			t.get_object_generic=[](RESOURCE_PTR subject,ptrdiff_t offset,size_t index){
				return (RESOURCE_PTR)(*static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset)))[index].t;
			};
			t.get_size_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t offset){
				return (static_cast<const BASE_PROPERTY*>((const void*)((const char*)subject+offset)))->size();
			};
			t.add_property_generic=[](RESOURCE_PTR subject,ptrdiff_t offset){
				static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset))->push_back(typename BASE_PROPERTY::value_type());
			};
			t.set_object_generic=[](RESOURCE_PTR subject,RESOURCE_PTR object,ptrdiff_t offset,size_t index){
				(*static_cast<BASE_PROPERTY*>((void*)((char*)subject+offset)))[index].set_object(object);
			};
			return t;
		}
	};
	#endif

	//template<typename RESOURCE> struct introspect:introspect<typename RESOURCE::SELF>{};
	template<
		typename SUBJECT,
		typename PROPERTY
	> property_info get_property_info(){
		//what happens when array of properties?
		introspect<PROPERTY>::get_property()->template get<rdf::Property::domains>().push_back(rdfs::domain(introspect<SUBJECT>::get_class()));
		#ifdef NEW_FUNC_TABLE
		auto p=property_info(introspect<PROPERTY>::get_property(),get_ftable<PROPERTY::TYPE,typename PROPERTY::BASE_PROPERTY>::go());
		//calculate offset
		SUBJECT* t=0;
		p.offset=(char*)&(t->get<PROPERTY>())-(char*)t;
		#else
		auto p=property_info(introspect<PROPERTY>::get_property(),functions<SUBJECT,PROPERTY>::get_table());
		#endif
		return p;
	};

	template<
		typename SUBJECT
	> struct _meta_{
		V v;
		template<typename PROPERTY> void operator()(){
			cerr<<"property:...\n";
			v.push_back(get_property_info<SUBJECT,PROPERTY>());
		};
	};

	struct meta{
		template<typename PROPERTY> void operator()(){
			introspect<PROPERTY>::get_property();
		}
	};
		template<
		typename NAMESPACE,
		typename NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,
		typename TRIGGER,
		typename ALLOCATOR
	>
	CONST_CLASS_PTR introspect<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>>::get_class(){
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR> RESOURCE;
		typedef typename IfThenElse<std::is_same<SUBCLASS,NIL>::value,RESOURCE,SUBCLASS>::ResultT TMP;
		#ifdef NATIVE
		static CONST_CLASS_PTR p=rdfs::Class::super(new rdfs::Class(
		#else
		static CONST_CLASS_PTR p=rdfs::Class::super(rdfs::Class::allocator_type::construct_allocate_at(
			//in case of persistent storage we will override old version and refresh pointers and function pointers
			//we have to find the index of pool where instances are stored, this is also where the pool is initialized
			//this is where the pool is created, actually it could have already been created 
			TMP::allocator_type::get_pool().index,
		#endif
			objrdf::get_uri<NAMESPACE,NAME>(),
			rdfs::subClassOf(introspect<SUPERCLASS>::get_class()),
			objrdf::base_resource::class_function_table(
				f_ptr::constructor<TMP>,
				f_ptr::destructor<TMP>,
				0/*f_ptr::copy_constructor<TMP>*/,//why is it 0???
				f_ptr::begin<TMP>,
				#ifdef NATIVE
				f_ptr::end<TMP>,
				//static_cast<objrdf::base_resource::type_iterator (*)(RESOURCE_PTR)>(f_ptr::end<TMP>),
				#else
				//if the pool is not writable all instances will be locked
				TMP::allocator_type::get_pool()->writable ? 
					static_cast<objrdf::base_resource::type_iterator (*)(RESOURCE_PTR)>(f_ptr::end<TMP>) : 
					static_cast<objrdf::base_resource::type_iterator (*)(RESOURCE_PTR)>(f_ptr::begin<TMP>),
				#endif
				f_ptr::cbegin<TMP>,
				f_ptr::cend<TMP>,
				f_ptr::allocate<TMP>,
				f_ptr::deallocate<TMP>,
				f_ptr::get_output<TMP>
			),
			TMP::get_comment!=SUPERCLASS::get_comment ? TMP::get_comment() : "",
			objrdf::sizeOf(sizeof(TMP))
			#ifndef NATIVE
			,objrdf::hashOf(pool_allocator::pool::get_hash<TMP>()),
			objrdf::persistent(!std::is_same<typename ALLOCATOR::_RAW_ALLOCATOR_,std::allocator<char>>::value)
			#endif
		));
		return p;
	}
	template<typename SUBJECT> struct add_trigger{
		V v;
		add_trigger(const V& v):v(v){}
		template<typename PROPERTY> void operator()(){
			LOG_INFO<<"trigger on property `"<<introspect<PROPERTY>::get_property()->id.local<<"'"<<std::endl;
			//look-up property in array
			auto i=find_if(v.begin(),v.end(),[](property_info& p){return p.p==introspect<PROPERTY>::get_property();});
			if(i!=v.end()){
				LOG_INFO<<"patching function table for `"<<i->p->id.local<<"'"<<endl;
				#ifndef NEW_FUNC_TABLE
				i->t=functions<SUBJECT,PROPERTY>::template trigger<SUBJECT>::patch(i->t);				
				#endif
			}
		}
	};
	template<
		typename NAMESPACE,
		typename NAME,
		typename PROPERTIES,
		typename SUBCLASS,
		typename SUPERCLASS,
		typename TRIGGER,
		typename ALLOCATOR
	>
	V introspect<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>>::get_vtable(){
		/*
			populate the function table for each resource
		*/
		cerr<<"vtable for "<<NAME::name()<<"...\n";
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR> RESOURCE;
		introspect<RESOURCE>::get_class();
		typedef typename IfThenElse<std::is_same<SUBCLASS,NIL>::value,RESOURCE,SUBCLASS>::ResultT TMP;
		V v=introspect<typename SUPERCLASS::SELF>::get_vtable();
		/*
		* multiple rdf:type properties, would be more consistent to have an array or a single type
		* actually not that simple because it is a pseudo property
		* for now the best is a single property
		*/
		function_table rdf_type;
	#ifdef NEW_FUNC_TABLE
		rdf_type.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t,size_t){return (CONST_RESOURCE_PTR)introspect<RESOURCE>::get_class();};
		rdf_type.get_size_generic=function_table::default_f::always_1;
	#else
		rdf_type.cget_object=[](CONST_RESOURCE_PTR subject,size_t){return (CONST_RESOURCE_PTR)introspect<RESOURCE>::get_class();};
		rdf_type.get_size=function_table::default_f::always_1;
		//does not do anything but needed when creating new resources with INSERT DATA{}
		rdf_type.add_property=function_table::default_f::add_property_def;
		rdf_type.set_object=[](RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){LOG_WARNING<<"rdf:type property ignored"<<endl;};
	#endif
		v.front()=property_info(introspect<rdf::type>::get_property(),rdf_type);
		//filter properties for convenience, we need to store index of first non-const property somewhere
		auto r=concat(v,objrdf::static_for_each<PROPERTIES>(_meta_<TMP>()).v);
		//objrdf::static_for_each<PROPERTIES>(_meta_<TMP>());
		//need to process triggers at this stage: 
		LOG_INFO<<"listing triggers for class `"<<NAME::name()<<"'"<<std::endl;
		r=objrdf::static_for_each<TRIGGER>(add_trigger<TMP>(r)).v;
		//make sure we only invoke once
		if(TMP::patch!=SUPERCLASS::patch){
			TMP::patch(r);//each class can decide to modify table: add pseudo-properties,...
			//if a property is added 
		}
		return r;
	}

	template<
		typename NAMESPACE,
		typename NAME,
		typename RANGE,
		typename BASE_PROPERTY
	> PROPERTY_PTR introspect<property<NAMESPACE,NAME,RANGE,BASE_PROPERTY>>::get_property(){
		#ifdef NATIVE
		static PROPERTY_PTR c=new rdf::Property(
		#else
		static PROPERTY_PTR c=rdf::Property::allocator_type::construct_allocate(
		#endif
				objrdf::get_uri<NAMESPACE,NAME>(),
				rdfs::range(introspect<typename selector<RANGE>::ResultT>::get_class()),
				/* will get rid of selector soon
				rdfs::range(BASE_PROPERTY::RDFS_RANGE::get_class()),
				*/
				property<NAMESPACE,NAME,RANGE,BASE_PROPERTY>::TYPE&LITERAL
		);
		return c;
	}
	/*
		static PROPERTY_PTR get_property_array(){
			static PROPERTY_PTR c(
				#ifdef NATIVE
				new rdf::Property(
				#else
				rdf::Property::allocator_type::construct_allocate(
				#endif
					objrdf::get_uri<NAMESPACE,NAME>(),
					rdfs::range(introspect<typename selector<RANGE>::ResultT>::get_class()),
					property<NAMESPACE,NAME,RANGE,BASE_PROPERTY>::TYPE&LITERAL,
					rdfs::subPropertyOf(static_cast<CONST_PROPERTY_PTR>(introspect<rdfs::member>::get_property()))
				)
			);
			return c;
		}
	*/
	template<
		typename PROPERTY,
		typename ALLOCATOR
	> struct introspect<array<PROPERTY,ALLOCATOR>>{
		static PROPERTY_PTR get_property(){
			/*
			*	temporary fix, property and array<property> are 2 distinct properties with the SAME uri -> confusion
			*/
			//return PROPERTY::get_property_array();
			return introspect<PROPERTY>::get_property();
		}
	};
	//there should be a cleaner way to do that
#if 0
	template<typename RANGE> struct selector{
		typedef get_Literal<RANGE> ResultT;
		enum{IS_LITERAL=1};
	};
#endif
	#ifdef NATIVE
	template<typename RANGE> struct selector<RANGE*>{
		typedef RANGE ResultT;
		enum{IS_LITERAL=0};
	};
	template<typename RANGE> struct selector<local<RANGE>>{
		typedef RANGE ResultT;
		enum{IS_LITERAL=0};
	};
	#else
	template<
		typename INDEX,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>struct selector<pool_allocator::pool::ptr<char,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		typedef xsd::String ResultT;
		enum{IS_LITERAL=1};
	};
	template<
		typename INDEX,
		typename PAYLOAD,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>struct selector<pool_allocator::pool::ptr<PAYLOAD,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		typedef PAYLOAD ResultT;
		enum{IS_LITERAL=0};
	};
	template<
		typename INDEX,
		typename PAYLOAD,
		typename ALLOCATOR,
		typename RAW_ALLOCATOR,
		typename MANAGEMENT
	>struct selector<pool_allocator::pool::ptr_d<PAYLOAD,INDEX,ALLOCATOR,RAW_ALLOCATOR,MANAGEMENT>>{
		typedef PAYLOAD ResultT;
		enum{IS_LITERAL=0};
	};

	#endif
	/*
 	*	we need to be able to tell if the property is an array
 	*
 	*/
		//pseudo property
	template<
		typename NAMESPACE,
		typename NAME,
		typename RANGE
	> PROPERTY_PTR property<NAMESPACE,NAME,RANGE,NIL>::get_property(){
		static PROPERTY_PTR c(
			#ifdef NATIVE
			new rdf::Property(
			#else
			rdf::Property::allocator_type::construct_allocate(
			#endif
				objrdf::get_uri<NAMESPACE,NAME>(),
				rdfs::range(introspect<RANGE>::get_class()),
				property<NAMESPACE,NAME,RANGE,NIL>::TYPE&LITERAL
			)
		);
		return c;
	}
	struct name_p{
		const uri n;
		name_p(const uri n);
		bool operator()(const property_info& p) const;
	};
	/*
	*	enforces consistency between the stores:
	*	subject		object
	*	persistent	persistent	ok
	*	persistent	volatile	no!
	*	volatile	persistent	ok (but might need garbage collection)
	*	volatile	volatile	ok
	*	
	*/
	/*
	template<typename SUBJECT_STORE,typename OBJECT_STORE> struct validate_store{enum{value=1};};
	template<> struct validate_store<persistent_store,free_store>{enum{value=0};};

	template<
		typename SUBJECT,
		typename PROPERTY,
		bool IS_LITERAL=PROPERTY::TYPE&LITERAL
	> struct help_validate_store:validate_store<typename SUBJECT::allocator_type::STORE,typename selector<typename PROPERTY::RANGE>::ResultT::allocator::STORE>{};
	//could still be allocated on the heap: eg. string
	template<
		typename SUBJECT,
		typename PROPERTY
	> struct help_validate_store<SUBJECT,PROPERTY,true>{enum{value=1};};
	*/	
	#ifdef NEW_FUNC_TABLE
	template<typename PROPERTY> property_info property_info::go(ptrdiff_t offset){
		return property_info(introspect<PROPERTY>::get_property(),get_ftable<PROPERTY::TYPE,typename PROPERTY::BASE_PROPERTY>::go(),offset);
	}
	#endif
} 
#endif
