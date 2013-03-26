/*
 *	support for entailment?
 *
 * 	need to find a way to detect when 2 statements share same object
 *
 * 	s_0--->O
 *	       ^
 *	s_1----+
 */
#include "sparql_engine.h"
subject::subject(SPARQL_RESOURCE_PTR r):r(r),is_selected(true),bound(r!=0),is_root(false),busy(false){}
subject::subject(string s):r(0),s(s),is_selected(true),bound(s.size()),is_root(false),busy(false){}
subject::subject(uri u):r(0),u(u),is_selected(true),bound(!u.empty()),is_root(false),busy(false){}
int subject::size(){
	int n=is_selected && !r && s.empty();
	for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j) n+=j->size();
	return n;
}
int verb::size(){
	int n=is_selected && !p;
	return n+object->size();
}
vector<string> subject::get_variables(){
	vector<string> v;
	if(!busy){
		if(!bound && is_selected) v.push_back(name.substr(1));
		for(vector<verb>::const_iterator j=verbs.begin();j<verbs.end();++j){
			busy=true;
			vector<string> tmp=j->get_variables();
			busy=false;
			v.insert(v.end(),tmp.begin(),tmp.end());
		}
	}
	return v;
}
vector<string> verb::get_variables() const{
	vector<string> v;
	if(!bound && is_selected) v.push_back(name.substr(1));
	vector<string> tmp=object->get_variables();
	v.insert(v.end(),tmp.begin(),tmp.end());
	return v;	
}
RESULT subject::run(size_t n){
	RESULT r;
	/*
	* before going through all the resources let's look at the properties bound and un-bound
	*/
	auto i=find_if(verbs.begin(),verbs.end(),match_property(rdf::type::get_property()));	
	if(i!=verbs.end()&&i->object&&i->object->bound){
		if(!i->object->r){
			i->object->r=find_t<CLASS_PTR>(i->object->u);

		}	
		//we should remove the rdf:type from the graph otherwise we are going to check the type again in run
		//find the pool
		//we have to make sure it is not base_resource::nil, why can't we use null??
		cerr<<"optimization..."<<i->object->r->id<<endl;
		if(i->object->r==CLASS_PTR(0)) return RESULT();
		if(get_class(i->object->r)!=rdfs::Class::get_class()) return RESULT();
		CONST_CLASS_PTR c(static_cast<CONST_CLASS_PTR>(i->object->r));
		//check if the pool exists
		POOL_PTR p(c.index);
		//not needed anymore
		cerr<<"assert pool `"<<c->id<<"'..."<<endl;
		assert(p->type_id);
		//iterate through the cells
		for(auto j=pool_iterator::cell_iterator(p,p->get_size());j<pool_iterator::cell_iterator(p);++j){
			RESULT tmp=run(get_const_self_iterator(*j),0);
			if(bound&&tmp.size()) return tmp;
			r.insert(r.end(),tmp.begin(),tmp.end());
			if(r.size()>=n) return r;
		}
		//now we have to find all the subclasses: we use superClassOf
		for(auto k=c->get_const<array<superClassOf>>().begin();k<c->get_const<array<superClassOf>>().end();++k){
			POOL_PTR p((*k).index);
			cerr<<"assert pool `"<<(*k)->id<<"'..."<<endl;
			assert(p->type_id);
			//iterate through the cells
			for(auto j=pool_iterator::cell_iterator(p,p->get_size());j<pool_iterator::cell_iterator(p);++j){
				RESULT tmp=run(get_const_self_iterator(*j),0);
				if(bound&&tmp.size()) return tmp;
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}
	}else{
		/*
		*	optimization : reason about given properties (bound or not)
		*	if one of the property is rdfs:domain, rdfs:range, then subject must be a Property
		*	if one of the property is rdfs:subClassOf then subject must be a Class
		*	could be done in a generic way
		*/		
		cerr<<"optimization ..."<<endl;
		bool is_Property=false,is_Class=false;
		for(auto i=verbs.begin();i<verbs.end();++i){
			cerr<<"current property: `"<<i->p->id<<"'"<<endl;
			is_Property|=(i->p==rdfs::domain::get_property())||(i->p==rdfs::range::get_property());
			//ugly but problem with array of properties
			is_Class|=(i->p==objrdf::array<rdfs::subClassOf>::get_property());
		}
		if(is_Property){
			cerr<<"optimization: Property only"<<endl;
			for(auto j=::begin<CONST_PROPERTY_PTR>();j< ::end<CONST_PROPERTY_PTR>();++j){
				RESULT tmp=run(get_const_self_iterator(*j),0);
				if(bound&&tmp.size()) return tmp;
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}else if(is_Class){
			cerr<<"optimization: Class only"<<endl;
			for(auto j=::begin<CLASS_PTR>();j< ::end<CLASS_PTR>();++j){
				RESULT tmp=run(get_const_self_iterator(*j),0);
				if(bound&&tmp.size()) return tmp;
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}else{	
			for(auto i=objrdf::begin();i<objrdf::end();++i){
				for(auto j=i.begin();j<i.end();++j){
					RESULT tmp=run(get_const_self_iterator(*j),0);
					if(bound&&tmp.size()) return tmp;
					r.insert(r.end(),tmp.begin(),tmp.end());
					if(r.size()>=n) return r;
				}
			}	
		}
	}
	return r;
}
RESULT subject::run(base_resource::const_instance_iterator i,CONST_PROPERTY_PTR p){
	if(i.literalp()){
		if(r){
			return RESULT(0);
		}else{
			if(verbs.size()) return RESULT(0);
			if(s.size()){
				LOG<<"bound\tR "<<this<<" to `"<<s<<"'"<<endl;
				return (i.str()==s) ? RESULT(1) : RESULT(0);
			}else{
				if(i.get_Property()->get_const<rdfs::range>()==rdfs::XML_Literal::get_class()){
					/*
 					*	shouldn't bind if empty, shouldn't be here in the first place
 					*/ 
					LOG<<"binding R "<<this<<" to XML_Literal ..."<<endl;
				}else{
					LOG<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
				}
				return is_selected ? RESULT(1,vector<base_resource::const_instance_iterator>(1,i)) : RESULT(1);
			}
		}
	}else{
		/*
 		*	what if bound to literal??
 		*/ 
		if(s.size()) return RESULT();
		SPARQL_RESOURCE_PTR _r=i->get_const_object();			
		if(r){
			LOG<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
			bool result=false;
			if(r==_r){
				result=true;
			}else if(get_class(r)==rdfs::Class::get_class()&&get_class(_r)==rdfs::Class::get_class()){
				cerr<<"entailment"<<endl;
				CONST_CLASS_PTR a(static_cast<CONST_CLASS_PTR>(r));		
				CONST_CLASS_PTR b(static_cast<CONST_CLASS_PTR>(_r));		
				//entailment rules depend on property
				//what about rdf::type?
				CONST_PROPERTY_PTR p=i->get_Property();
				if(p==rdfs::domain::get_property()) result=*b<*a;
				else if(p==rdfs::range::get_property()||p==rdfs::subClassOf::get_property()||p==rdf::type::get_property()) result=*a<*b;
			}
			if(!result) return RESULT();
		}else if(!u.empty()){
			LOG<<"bound\tR "<<this<<" to URI `"<<u<<"'"<<endl;
			/*
 			*	compare URI's instead
 			*/ 
			bool result=false;
			if(_r->id==u){
				cerr<<"URI comparison: "<<_r->id<<"=="<<u<<endl;
				r=_r;//bind!
				result=true;
			}else if(get_class(_r)==rdfs::Class::get_class()){
				//now we have to look up the resource but we know it is a Class
				CONST_CLASS_PTR a(find_t<CONST_CLASS_PTR>(u)),b(static_cast<CONST_CLASS_PTR>(_r));
				if(a){
					r=a;//bind!
					CONST_PROPERTY_PTR p=i->get_Property();
					if(p==rdfs::domain::get_property()) result=*b<*a;
					else if(p==rdfs::range::get_property()||p==rdfs::subClassOf::get_property()||p==rdf::type::get_property()) result=*a<*b;
				}
			}
			if(!result) return RESULT();
			/*
 			*	what if still not bound at this stage? fine
 			*/ 
		}else{
			LOG<<"binding R "<<this<<" to `"<<_r->id<<"'"<<endl;
		}		
		vector<RESULT> s;
		unsigned int n=0,m=1;
		//bind in case there is a cycle in the WHERE statement
		//cleaner would be to stow away 
		/*
		bool temp_bound=false;
		if(!r){
			r=_r;
			temp_bound=true;
		}	
		if(!busy){
		*/	for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
				busy=true;
				RESULT tmp=j->run(_r);
				busy=false;
				if(tmp.empty()){
					/*if(temp_bound){
						r=SPARQL_RESOURCE_PTR();
						temp_bound=false;//not needed
					}*/
					return RESULT();
				}
				if(tmp.size()==1 && tmp.front().size()==0){

				}else{
					n+=tmp.front().size();//all the same size 
					m*=tmp.size();
					s.push_back(tmp);
				}
			}
		//}
		/*
		if(temp_bound){
			r=SPARQL_RESOURCE_PTR();
			temp_bound=false;//not needed
		}
		*/
		RESULT ret=((r||!u.empty())||!is_selected) ? RESULT(m) : RESULT(m,vector<base_resource::const_instance_iterator>(1,i));	
		for(unsigned int i=0;i<m;++i){
			for(unsigned int j=0;j<s.size();++j){
				for(unsigned int k=0;k<s[j].front().size();++k){
					ret[i].push_back(s[j][i%s[j].size()][k]);		
				}
			}
		}	
		return ret;	
	}
}
verb::verb(CONST_PROPERTY_PTR p,subject* object,CONST_USER_PTR user):p(p),object(object),is_optional(false),is_selected(true),bound(p),user(user){}
RESULT verb::run(SPARQL_RESOURCE_PTR r){
	if(p){
		LOG<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::const_type_iterator current_property=std::find_if(cbegin(r,user),cend(r,user),match_property(p));
		if(current_property!=cend(r)){
			RESULT ret;
			for(base_resource::const_instance_iterator j=current_property->cbegin();j!=current_property->cend();++j){
				//they are all the same size so we just stack them up
				RESULT tmp=object->run(j,p);
				LOG<<tmp<<endl;
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
			if(current_property->cbegin()==current_property->cend()){
				/*
				*	how do we deal with optional results, let's say that rdfs::subPropertyOf should be optional
				*/ 
				cerr<<"optional? "<<p->id<<endl;
				if(p==rdfs::subPropertyOf::get_property()){
					RESULT tmp=object->run(get_const_self_iterator(objrdf::base_resource::nil),p);	
					LOG<<tmp<<endl;
					ret.insert(ret.end(),tmp.begin(),tmp.end());
				}
			}
			return ret;
		}else{
			return RESULT();
		}
	}else{
		RESULT ret;
		for(base_resource::const_type_iterator i=cbegin(r,user);i!=cend(r);++i){
			base_resource::const_instance_iterator pt=get_const_self_iterator(i->get_Property());
			for(base_resource::const_instance_iterator j=i->cbegin();j!=i->cend();++j){
				LOG<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				RESULT tmp=object->run(j,i->get_Property());
				LOG<<tmp<<endl;
				if(is_selected){
					for(auto k=tmp.begin();k<tmp.end();++k) k->insert(k->begin(),pt);
				}
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
		}
		return ret;
	}
}
struct comp_r{
	int i;//which variable to use
	comp_r(int i):i(i){}
	bool operator()(const vector<base_resource::const_instance_iterator>& a,const vector<base_resource::const_instance_iterator>& b){
		return a[i].compare(b[i])<0;	
	}	
};

struct comp_r2{
	int i,j;//which variable to use
	comp_r2(int i,int j):i(i),j(j){}
	bool operator()(const vector<base_resource::const_instance_iterator>& a,const vector<base_resource::const_instance_iterator>& b){
		int tmp=a[i].compare(b[i]);
		return tmp? tmp<0 : a[j].compare(b[j])<0;	
	}	
};
/*	it would be neat to have an iterator based on a sparql query but with
 *	casting if all the types are the same, should the query be run first and 
 *	the result stored in a temporary array?
 */
//void to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject& s){
void sparql_parser::sort(RESULT& r,vector<string> variables,vector<string> order_variables){
	vector<unsigned int> vv;
	for(auto i=order_variables.cbegin();i<order_variables.cend();++i){
		auto j=find(variables.cbegin(),variables.cend(),*i);
		if(j!=variables.cend()){
			vv.push_back(j-variables.cbegin());
			cerr<<"ordering by variable `"<<*i<<"'"<<endl;
		}
	}
	if(vv.size()==1) std::sort(r.begin(),r.end(),comp_r(vv[0]));
	else if(vv.size()==2) std::sort(r.begin(),r.end(),comp_r2(vv[0],vv[1]));
}
void sparql_parser::to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject& s){
	/*
 	*	we should not serialize empty literal but hard to know
 	*	before serializing (could use buffer), 
 	*	could play with instance iterators...
 	*	how do we conserve the original variable order?
 	*	we need an offset array
 	*/
	vector<string> v=s.get_variables();	
	sort(r,s.get_variables(),order_by_variables);
	os<</*"<?xml version=\"1.0\"?>\n*/"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">\n<head>\n";
	for(auto i=v.cbegin();i<v.cend();++i) os<<"<variable name='"<<*i<<"'/>\n";
	os<<"</head>\n<results>\n";
	for(auto i=r.begin();i<r.end();++i){
		os<<"<result>\n";
		int v_i=0;
		for(auto j=i->cbegin();j<i->cend();++j){
			os<<"<binding name='"<<v[v_i++]<<"'>";
			if(j->literalp())
				os<<"<literal>"<<*j<<"</literal>";
			else{
				os<<"<uri>";
				j->get_const_object()->id.to_uri(os);
				os<<"</uri>";
			}
			os<<"</binding>\n";
		}
		os<<"</result>\n";
	}
	os<<"</results>\n</sparql>\n";

}
sparql_parser::sparql_parser(istream& is,PROVENANCE p):char_iterator(is),sbj(0),current_sbj(0),q(no_q),p(p){
	//non standard but helpful
	prefix_ns["rdf"]="http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	prefix_ns["rdfs"]="http://www.w3.org/2000/01/rdf-schema#";
	prefix_ns["obj"]="http://www.example.org/objrdf#";
}
bool sparql_parser::go(){
	/*
 	*	we need to control privileges, can we use function pointers?
 	*	would be nice but function pointers are attached to classes, not to users
 	*	so we could modify the vtable according to the user's privilege table run her query
 	*	and reset the vtable, it will only work if we have a global lock on the database
 	*/ 
	if(!document::go(*this)) return false;
	if(q==select_q||q==describe_q) return sbj;
	return true;
}
void sparql_parser::out(ostream& os){//sparql XML serialization
	//if(sbj){
		switch(q){
			case select_q:{
				RESULT r=sbj->run();
				to_xml(os,r,*sbj);
			}break;
			case simple_describe_q:{
				os<<"<"<<rdf::_RDF<<"\n";
				uri::ns_declaration(os);
				os<<"xml:base='http://inventory.unicefuganda.org/'"<<endl;
				os<<">";
				if(d_resource)
					to_rdf_xml(d_resource,os);
				os<<"\n</"<<rdf::_RDF<<">\n";
			}break;
			case describe_q:{
				RESULT r=sbj->run();				
				os<<"<"<<rdf::_RDF<<"\n";
				uri::ns_declaration(os);
				os<<"xml:base='http://inventory.unicefuganda.org/'"<<endl;
				os<<">";
				sort(r,sbj->get_variables(),order_by_variables);
				for(auto i=r.begin();i<r.end();++i){
					//only if resource
					for(auto j=i->cbegin();j<i->cend();++j){
						if(j->literalp()){
							//os<<"<literal>"<<*j<<"</literal>";
						}else{
							to_rdf_xml(j->get_const_object(),os);
						}
					}
				}
				os<<"\n</"<<rdf::_RDF<<">\n";
			}break;
			case insert_data_q:{
			}break;
			case delete_data_q:{
			}break;
			default:break;
		}
	//}else{

	//}
}
bool sparql_parser::callback(PARSE_RES_TREE& r){
	cerr<<r<<endl;
	switch(r.t.first){
		//what about multiple statements?
		case prefix_decl::id:
			prefix_ns[r.v[0].t.second]=r.v[1].t.second;	
		break;
		case select_query::id:{
			q=select_q;
			auto i=r.v.begin();
			while(i!=r.v.end()&& i->t.first==turtle_parser::variable::id){
				variable_set.insert(i->t.second);
				++i;
			}
			//return parse_where_statement(*i);
			bool b=parse_where_statement(*i);
			++i;
			for(;i<r.v.end();++i) parse_extra_statement(*i);	
			return b;
		}break;
		case simple_describe_query::id:{
			q=simple_describe_q;
			auto i=r.v.begin();
			while(i!=r.v.end()&& i->t.first==turtle_parser::variable::id){
				variable_set.insert(i->t.second);
				++i;
			}
			//d_resource=find(uri::hash_uri(r.v[0].t.second));	
			d_resource=parse_object(*i);
		}break;
		case describe_query::id:{
			q=describe_q;
			auto i=r.v.begin();
			while(i!=r.v.end()&& i->t.first==turtle_parser::variable::id){
				variable_set.insert(i->t.second);
				++i;
			}
			//return parse_where_statement(*i);
			bool b=parse_where_statement(*i);
			++i;
			for(;i<r.v.end();++i) parse_extra_statement(*i);	
			return b;
		}break;
		case insert_data_query::id:{
			q=insert_data_q;	
			return parse_update_data_statement(r,false);
		}break;
		case delete_data_query::id:{
			q=delete_data_q;
			return parse_update_data_statement(r,true);
		}break;
		/*
		case update_data_query::id:{

			cerr<<"update data query"<<endl;

		}break;
		*/
		case update_query::id:{
			//we have 2 statements
			cerr<<"update query"<<endl;
			for(auto i=r.v.begin();i<r.v.end();++i)
				cerr<<i->t.first<<endl;
			//need to process where statement first
			parse_where_statement(r.v[1]);
			//all the variables are defined, we can iterate
			//we can repeatedly parse that statement with variables replaced or patch the parser output
			//how do we deal with properties?, maybe no properties for now
			RESULT res=sbj->run();				
			vector<string> var=sbj->get_variables();	
			for(auto i=res.begin();i<res.end();++i){
				//we can define the current variable list
				VARIABLES v;
				auto v_i=var.cbegin();
				for(auto j=i->begin();j<i->end();++j,++v_i){
					cerr<<"copying variable `"<<*v_i<<"'"<<endl;
					 v[*v_i]=*j;
				}
				parse_update_data_statement(r.v[0],r.v[0].t.first==delete_statement::id,v);
			}

		}break;
		//case insert_data_statement::id://need to process where statement first
			//return parse_insert_data_statement(r);
		//break;
		//case delete_statement::id://need to process where statement first
		//break;
		//case insert_statement::id://need to process where statement first
		//break;

	}
	return true;
}
bool sparql_parser::parse_extra_statement(PARSE_RES_TREE& r){
	if(r.t.first==order_by::id){
		cerr<<"order by!"<<endl;
		for(auto i=r.v.cbegin();i!=r.v.cend();++i)
			order_by_variables.push_back(i->t.second.substr(1));
	}
	return true;
}
bool sparql_parser::parse_where_statement(PARSE_RES_TREE& r){
	for(auto i=r.v.begin();i<r.v.end();++i){
		cerr<<"current:\n"<<*i<<endl;
		switch(i->t.first){
			case turtle_parser::subject::id:{
				switch(i->v[0].t.first){
					case turtle_parser::variable::id:{
						INDEX::iterator j=index.find(i->v[0].t.second);
						if(j!=index.end()){
							current_sbj=j->second;
						}else{
							sbj=new subject();
							sbj->name=i->v[0].t.second;
							sbj->is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
							index[i->v[0].t.second]=sbj;
							current_sbj=sbj;
						}	
					}
					break;
					case turtle_parser::uriref::id:{
						//optimization: use the properties to restrict search
						uri u=uri::hash_uri(i->v[0].t.second);
						//SPARQL_RESOURCE_PTR r=find(uri::hash_uri(i->v[0].t.second));
						//SPARQL_RESOURCE_PTR r=find(u);
						//we should bail out there if not found
						sbj=new subject(u);
						index[i->v[0].t.second]=sbj;
						current_sbj=sbj;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							sbj=new subject(u);
							index[i->v[0].v[0].t.second]=sbj;
							current_sbj=sbj;
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;		
					default:
						cerr<<"??"<<endl;
					break;
				}
			}
			break;
			case turtle_parser::verb::id:{
				switch(i->v[0].t.first){
					case turtle_parser::variable::id:{
						current_sbj->verbs.push_back(verb(CONST_PROPERTY_PTR(),0,user));
						current_sbj->verbs.back().name=i->v[0].t.second;		
						current_sbj->verbs.back().is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
					}
					break;
					case turtle_parser::uriref::id:{
						CONST_PROPERTY_PTR r=find_t<CONST_PROPERTY_PTR>(uri::hash_uri(i->v[0].t.second));
						current_sbj->verbs.push_back(verb(r,0,user));
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							CONST_PROPERTY_PTR r=find_t<CONST_PROPERTY_PTR>(u);
							current_sbj->verbs.push_back(verb(r,0,user));
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::is_a::id:{
						current_sbj->verbs.push_back(verb(rdf::type::get_property(),0,user));
					}
					break;
				}
			}
			break;
			case turtle_parser::object::id:{
				switch(i->v[0].t.first){
					case turtle_parser::variable::id:{
						INDEX::iterator j=index.find(i->v[0].t.second);
						if(j!=index.end()){
							current_sbj->verbs.back().object=j->second;
						}else{
							subject* object=new subject();
							object->name=i->v[0].t.second;
							object->is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
							index[i->v[0].t.second]=object;
							current_sbj->verbs.back().object=object;
						}	
					}
					break;
					//should not try to resolve resources, waste
					case turtle_parser::uriref::id:{
						SPARQL_RESOURCE_PTR r;
						uri u=uri::hash_uri(i->v[0].t.second);
						subject* object=new subject(u);
						index[i->v[0].t.second]=object;
						current_sbj->verbs.back().object=object;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							subject* object=new subject(u);
							index[i->v[0].v[0].t.second]=object;
							current_sbj->verbs.back().object=object;
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::literal::id:{
						current_sbj->verbs.back().object=new subject(i->v[0].v[0].t.second);
					}
					break;
				}
			}
			break;
		}
	}
	return true;
}
CONST_PROPERTY_PTR sparql_parser::parse_property(const PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:{
			//return find_t<rdf::Property>(uri::hash_uri(r.t.second));
		}
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return find_t<CONST_PROPERTY_PTR>(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return CONST_PROPERTY_PTR();
			}
		}break;
		case turtle_parser::is_a::id:return rdf::type::get_property();
		default:return CONST_PROPERTY_PTR();	
	}
}
//won't parse literal
//waste because does not use any information from the query
SPARQL_RESOURCE_PTR sparql_parser::parse_object(const PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:return find(uri::hash_uri(r.t.second));break;
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return find(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return CONST_PROPERTY_PTR();
			}
		}break;
		default:return CONST_PROPERTY_PTR();//???	
	}
}
bool sparql_parser::parse_update_data_statement(const PARSE_RES_TREE& r,bool do_delete,VARIABLES v,RESOURCE_PTR sub){
	return parse_update_data_statement(r.v.cbegin(),r.v.cend(),do_delete,v,sub);	
}
bool sparql_parser::parse_update_data_statement(PARSE_RES_TREE::V::const_iterator begin,PARSE_RES_TREE::V::const_iterator end,bool do_delete,VARIABLES v,RESOURCE_PTR sub){
/*
*	it is actually a plain turtle parser, it might not be practical to store thr document in a PARSE_RES_TREE,
*	we need a generic resource that can store literals and pointers, could use it for rdf_xml_parser as well
*	pair<uri,string> and 
*	pair<uri,SPARQL_RESOURCE_PTR >
*	it could be build 
*	we could break it down in triples and put on stack
*	a lot of optimizations when using the schema
*/
	//RESOURCE_PTR sub;//subject, could be modified
	//needs a default value
	base_resource::type_iterator current_property(RESOURCE_PTR(0),objrdf::V::iterator());
	//for(auto i=r.v.begin();i<r.v.end();++i){
	for(auto i=begin;i<end;++i){
		cerr<<"!!!current:\n"<<*i<<endl;
		switch(i->t.first){
			case turtle_parser::subject::id:{
				cerr<<"subject"<<endl;
				/*
 				*	we should keep the map::iterator to the subject in case we
 				*	need to remove it from the document
 				*/ 
				switch(i->v[0].t.first){
					case turtle_parser::uriref::id:{
						uri u=uri::hash_uri(i->v[0].t.second);
						sub=find(u);
						if(!sub){//we have to create the resource but we need to know its type, we have access to the whole document
							//we assume that the types will be given in this statement
							auto j=i+1;
							while(j<end&&j->t.first!=turtle_parser::subject::id){
								if(j->t.first==turtle_parser::verb::id){
									cerr<<"verb:\n"<<*j<<endl;
									if(parse_property(j->v[0])==rdf::type::get_property()){
										cerr<<"rdf:type!"<<endl;
										//the next object will be the rdfs::Class
										CONST_RESOURCE_PTR r=parse_object((j+1)->v[0]);
										if(r){
											if(get_class(r)==rdfs::Class::get_class()){
												cerr<<"rdfs:Class!"<<endl;
												CONST_CLASS_PTR c(r);
												sub=create_by_type(c,u);
												if(!sub){
													cerr<<"cannot create instances of class `"<<r->id<<"'"<<endl;
													return false;
												}
												break;	
											}else{
												cerr<<"resource is not a rdfs:Class"<<endl;
												return false;	
											}
										}else{
											cerr<<"resource not found"<<endl;
											return false;
										}
									}	
								}
								++j;
							}
							if(!sub) return false;
						}
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							sub=find(u);
							if(!sub){
								auto j=i+1;
								while(j<end&&j->t.first!=turtle_parser::subject::id){
									if(j->t.first==turtle_parser::verb::id){//is there a chance we get to next statement?
										cerr<<"verb:\n"<<*j<<endl;
										if(parse_property(j->v[0])==rdf::type::get_property()){
											cerr<<"rdf:type!"<<endl;
											//the next object will be the rdfs::Class
											SPARQL_RESOURCE_PTR r=parse_object((j+1)->v[0]);
											if(r&&get_class(r)==rdfs::Class::get_class()){
												cerr<<"rdfs:Class!"<<endl;
												CONST_CLASS_PTR c(r);
												sub=create_by_type(c,u);
												if(!sub){
													cerr<<"cannot create instances of class `"<<r->id<<"'"<<endl;
													return false;
												}
												break;	
											}
										}	
									}
									++j;
								}
							}
							if(!sub) return false;
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::variable::id:{
						auto j=v.find(i->v[0].t.second.substr(1));
						if(j!=v.end())
							//we need to cast away constness, ugly but makes it noticeable
							sub=RESOURCE_PTR(j->second->get_const_object().index,j->second->get_const_object().pool_ptr);
						else
							cerr<<"subject variable `"<<i->v[0].t.second<<"' not found"<<endl;	
							
					}break;
					default:return false;
				}
			}
			break;
			case turtle_parser::verb::id:{
				/*
 				*	the parser should be more forgiving: if a property does not belong to the resource
 				*	it should just be ignored
 				*/ 
				cerr<<"verb"<<endl;
				switch(i->v[0].t.first){
					case turtle_parser::uriref::id:{ 
						uri u=uri::hash_uri(i->v[0].t.second);
						current_property=std::find_if(objrdf::begin(sub,user),objrdf::end(sub,user),name_p(u));
						if(current_property==objrdf::end(sub)){
							cerr<<"property `"<<u<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
							current_property=objrdf::begin(base_resource::nil,user);
						}else{//what if property constant
							if(current_property.constp()){
								cerr<<"property `"<<u<<"' can not be modified"<<endl;
								current_property=objrdf::begin(base_resource::nil,user);
							}
						}
					}break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							//find if property belongs to subject
							current_property=std::find_if(objrdf::begin(sub,user),objrdf::end(sub,user),name_p(u));
							if(current_property==objrdf::end(sub)){
								cerr<<"property `"<<u<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
								current_property=objrdf::begin(base_resource::nil,user);
							}else{
								if(current_property.constp()){
									cerr<<"property `"<<u<<"' can not be modified"<<endl;
									current_property=objrdf::begin(base_resource::nil,user);
								}
							}
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::is_a::id:{
						current_property=objrdf::begin(sub,user);//first property is always rdf::type
					}break;
					case turtle_parser::variable::id:{
						cerr<<"property variable: `"<<i->v[0].t.second<<"'"<<endl;
						auto j=v.find(i->v[0].t.second.substr(1));
						if(j!=v.end()){
							current_property=std::find_if(objrdf::begin(sub,user),objrdf::end(sub,user),match_property(static_cast<CONST_PROPERTY_PTR>(j->second->get_const_object())));
							if(current_property==objrdf::end(sub,user)){
								cerr<<"property `"<<j->second->get_const_object()->id<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
								return false;
							}
						}else
							cerr<<"property variable `"<<i->v[0].t.second<<"' not found"<<endl;	
							
					}break;
					default:return false;
				}
			}
			break;
			case turtle_parser::object::id:{
				cerr<<"object"<<endl;
				switch(i->v[0].t.first){
					case turtle_parser::literal::id:{
						if(current_property.literalp()){
							if(do_delete){
								auto j=current_property->begin();
								while(j!=current_property->end()){
									if(j->str()==i->v[0].v[0].t.second){
										//sub->erase(j);
										erase(sub,j);
										break;
									}	
									++j;
								}
							}else{
								if(current_property->get_Property()->get_const<rdfs::range>()==xsd::String::get_class()){
									current_property->add_property(0)->set_string(i->v[0].v[0].t.second);
								}else{
									istringstream is(i->v[0].v[0].t.second);
									current_property->add_property(0)->in(is);
									//there should not be anything left in the stream
								}
							}
						}else{
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is not literal"<<endl;
						}
					}
					break;
					case turtle_parser::uriref::id:{
						if(current_property.literalp()){
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is literal"<<endl;
						}else{
							uri u=uri::hash_uri(i->v[0].t.second);//creates a uri if empty!
							//hack to remove bad data
							//uri u;
							if(do_delete){
								auto j=current_property->begin();
								for(;j!=current_property->end();++j){
									if(j->get_const_object()->id==u) break;
								}
								if(j==current_property->end()){
									cerr<<"resource `"<<u<<"' not found"<<endl;
									return false;
								}else{
									//sub->erase(j);
									erase(sub,j);
								}	
							}else{
								SPARQL_RESOURCE_PTR r=find(u);
								if(r){
									//cast away constness
									current_property->add_property(0)->set_object(RESOURCE_PTR(r.index,r.pool_ptr));
								}else{
									cerr<<"resource `"<<u<<"' not found"<<endl;
								}
							}
						}
					}
					break;
					case turtle_parser::qname::id:{
						if(current_property.literalp()){
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is literal"<<endl;
						}else{
							PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
							if(j!=prefix_ns.end()){
								uri u(j->second,i->v[0].v[1].t.second);
								if(do_delete){
									auto j=current_property->begin();
									while(j!=current_property->end()){
										if(j->get_const_object()->id==u){
											//sub->erase(j);
											erase(sub,j);
											break;
										}	
									}
								}else{
									SPARQL_RESOURCE_PTR r=find(u);
									if(r){
										//cast away constness
										current_property->add_property(0)->set_object(RESOURCE_PTR(r.index,r.pool_ptr));
									}else{
										cerr<<"resource `"<<u<<"' not found"<<endl;
									}
								}
							}else{
								cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
								return false;
							}
						}
					}
					break;
					case 9999:{
						cerr<<"blank node!"<<endl;
						//we might have a problem here: if set_object() fails we end up with non-initialized memory, also happens
						//if the connection is lost,
						/*
						base_resource::instance_iterator o=current_property->add_property(0);
						o->set_object(create_by_type_blank(current_property->get_Property()->get_const<rdfs::range>()));	
						parse_update_data_statement(i->v[0].v.cbegin(),i->v[0].v.cend(),do_delete,v,o->get_object());
						*/
						RESOURCE_PTR tmp=create_by_type_blank(current_property->get_Property()->get_const<rdfs::range>());	
						parse_update_data_statement(i->v[0].v.cbegin(),i->v[0].v.cend(),do_delete,v,tmp);
						current_property->add_property(0)->set_object(tmp);
					}
					break;
					case turtle_parser::variable::id:{
						auto j=v.find(i->v[0].t.second.substr(1));
						if(j!=v.end()){
							if(current_property.literalp()){
								if(j->second.literalp()){
									ostringstream os;
									os<<*j->second;
									string object_str=os.str();
									if(do_delete){
										auto k=current_property->begin();
										while(k!=current_property->end()){
											ostringstream os;
											os<<*k;
											if(os.str()==object_str){		
												//sub->erase(k);
												erase(sub,k);
												break;
											}	
											++k;
										}
									}else{
										current_property->add_property(0)->set_string(object_str);
									}	
								}else{
									cerr<<"current property `"<<current_property->get_Property()->id<<"' is literal"<<endl;
									return false;
								}
							}else{
								if(j->second.literalp()){
									cerr<<"current property `"<<current_property->get_Property()->id<<"' is not literal"<<endl;
									return false;
								}else{
									CONST_RESOURCE_PTR obj=j->second->get_const_object();
									if(do_delete){
										auto k=current_property->begin();
										while(k!=current_property->end()){
											if(k->get_const_object()==obj){
												//sub->erase(k);
												erase(sub,k);
												break;
											}	
											++k;
										}
									}else{
										//cast away constness
										current_property->add_property(0)->set_object(RESOURCE_PTR(obj.index,obj.pool_ptr));
									}
								}
							}
						}else
							cerr<<"object variable `"<<i->v[0].t.second<<"' not found"<<endl;	
					}break;
					default:return false;
				}
			}break;
			default:return false;
		}	
	}
	return true;
}
ostream& operator<<(ostream& os,const RESULT& r){
	for(auto i=r.begin();i<r.end();++i){
		for(auto j=i->cbegin();j<i->cend();++j){
			if((*j)!=cbegin(base_resource::nil)->cbegin()){
				if(j->get_Property()->get_const<rdfs::range>()==rdfs::XML_Literal::get_class())
					os<<"XML_Literal ...\t|";
				else{
					os<<*j<<"\t|";
				}
			}else{
				os<<"nil\t|";
			}
		}
	}
	return os;
}
