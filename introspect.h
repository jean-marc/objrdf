#ifndef INTROSPECTION_H
#define INTROSPECTION_H
#include "objrdf.h"
/*
*	separates introspection from definitions
*/

namespace objrdf{
	template<typename RESOURCE> struct introspect:introspect<typename RESOURCE::SELF>{};
	template<> struct introspect<base_resource>{
		static CONST_CLASS_PTR get_class(){
			#ifdef NATIVE
			static CONST_CLASS_PTR p=new rdfs::Class(
			#else
			static CONST_CLASS_PTR p=rdfs::Class::allocator_type::construct_allocate_at(
				base_resource::allocator_type::get_pool().index,
			#endif
				objrdf::get_uri<rdfs::rdfs_namespace>("Resource"),
				rdfs::subClassOf(),
				objrdf::base_resource::class_function_table(
					f_ptr::constructor<base_resource>,
					f_ptr::destructor<base_resource>,
					f_ptr::copy_constructor<base_resource>,
					f_ptr::begin<base_resource>,
					f_ptr::end<base_resource>,
					f_ptr::cbegin<base_resource>,
					f_ptr::cend<base_resource>,
					f_ptr::allocate<base_resource>,
					f_ptr::deallocate<base_resource>,
					f_ptr::get_output<base_resource>
				)			
				,base_resource::get_comment()
				,objrdf::sizeOf(sizeof(base_resource))
				#ifndef NATIVE
				,objrdf::hashOf(pool_allocator::pool::get_hash<base_resource>())
				#endif
			);
			return p;
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
	struct introspect<resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>>{
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR> RESOURCE;
		static CONST_CLASS_PTR get_class(){
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
	};
	template<
		typename NAMESPACE,
		typename NAME,
		typename RANGE,
		typename BASE_PROPERTY
	> struct introspect<property<NAMESPACE,NAME,RANGE,BASE_PROPERTY>>{
		static PROPERTY_PTR get_property(){
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
	};
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
			//would be nice to have message
			//static_assert(help_validate_store<SUBJECT,PROPERTY>::value,"inconsistent stores");
			/*if(!help_validate_store<SUBJECT,PROPERTY>::value){
				LOG<<SUBJECT::get_class()->id<<" "<<introspect<PROPERTY>::get_property()->id<<std::endl;
				exit(1);
			}*/
			v.push_back(get_property_info<SUBJECT,PROPERTY>());
		};
	};
	template<> struct get_generic_property<base_resource>{
		static V go(){
			static V v=_go();
			return v;	
		}
		static V _go(){
			LOG_DEBUG<<"get_generic_property:`base_resource'"<<std::endl;
			function_table rdf_type,objrdf_self,objrdf_id;
		#ifdef NEW_FUNC_TABLE
			rdf_type.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t,size_t){return (CONST_RESOURCE_PTR)introspect<base_resource>::get_class();};
			objrdf_self.cget_object_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t,size_t index){return subject;};
			objrdf_id.out_generic=[](CONST_RESOURCE_PTR subject,ptrdiff_t,ostream& os,size_t){os<<subject->id;};
			rdf_type.get_size_generic=function_table::default_f::always_1;
			objrdf_self.get_size_generic=function_table::default_f::always_1;
			objrdf_id.get_size_generic=function_table::default_f::always_1;
		#else
			rdf_type.cget_object=[](CONST_RESOURCE_PTR subject,size_t){return (CONST_RESOURCE_PTR)introspect<base_resource>::get_class();};
			rdf_type.get_size=function_table::default_f::always_1;
			//does not do anything but needed when creating new resources with INSERT DATA{}
			rdf_type.add_property=function_table::default_f::add_property_def;
			rdf_type.set_object=[](RESOURCE_PTR subject,RESOURCE_PTR object,size_t index){LOG_WARNING<<"rdf:type property ignored"<<endl;};
			objrdf_self.cget_object=[](CONST_RESOURCE_PTR subject,size_t index){return subject;};
			objrdf_self.get_size=function_table::default_f::always_1;
			objrdf_id.set_string=[](RESOURCE_PTR subject,string s,size_t){
				if(subject->id.is_local()) //only makes sense with local resources
					subject->id=uri(s);//could add code to detect duplicate id's
			};
			objrdf_id.in=[](RESOURCE_PTR subject,istream& is,size_t){
				string tmp;
				is>>tmp;
				if(subject->id.is_local()) //only makes sense with local resources
					subject->id=uri(tmp);//could add code to detect duplicate id's
			};	
			objrdf_id.out=[](CONST_RESOURCE_PTR subject,ostream& os,size_t){os<<subject->id;};	
			objrdf_id.get_size=function_table::default_f::always_1;
		#endif
			V v={
				property_info(introspect<rdf::type>::get_property(),rdf_type),//missing rdfs:domain
				property_info(introspect<objrdf::self>::get_property(),objrdf_self),
				property_info(introspect<objrdf::id>::get_property(),objrdf_id)
			};
			/*
 			*	since we don't use get_property_info we need to set the rdfs::domain of rdf::type
 			*	we can not use property_info::p because it is a pointer to const
 			*/ 
			introspect<rdf::type>::get_property()->get<rdf::Property::domains>().push_back(rdfs::domain(introspect<base_resource>::get_class()));
			return v;
		}
	};
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
	> struct get_generic_property<
		resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR>
	>{
		typedef resource<NAMESPACE,NAME,PROPERTIES,SUBCLASS,SUPERCLASS,TRIGGER,ALLOCATOR> RESOURCE;
		typedef typename IfThenElse<std::is_same<SUBCLASS,NIL>::value,RESOURCE,SUBCLASS>::ResultT TMP;
		static V go(){
			static V v=_go();
			return v;	
		}
		static V _go(){
			LOG_DEBUG<<"get_generic_property:`"<<NAME::name()<<"'"<<std::endl;
			V v=get_generic_property<typename SUPERCLASS::SELF>::go();
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
	};


} 
#endif
