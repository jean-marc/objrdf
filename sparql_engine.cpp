#include "sparql_engine.h"
subject::subject(SPARQL_RESOURCE_PTR r):r(r),is_selected(true),bound(r!=0),is_root(false),busy(false){}
subject::subject(string s):r(0),s(s),is_selected(true),bound(s.size()),is_root(false),busy(false){}
objrdf::V subject::_v;
//#define LOG if(0) cerr
//subject subject::bound(SPARQL_RESOURCE_PTR r){return subject(r);}
//subject subject::bound(string s){return subject(s);}
//subject subject::unbound(string name){
	//subject s;
	//s.name=name;
	//return s;
//}
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
	//optimization when subject is bound
	if(bound){
		r=run(this->r,0);
	}else{
		/*
 		* before going through all the resources let's look at the properties bound and un-bound
 		*/
		auto i=find_if(verbs.begin(),verbs.end(),match_property(rdf::type::get_property()));	
		if(i!=verbs.end()&&i->object&&i->object->bound){
			//find the pool
			cerr<<"optimization..."<<endl;
			POOL_PTR p(CLASS_PTR(i->object->r).index);
			//iterate through the cells
			for(auto j=pool_iterator::cell_iterator(p,p->get_size());j<pool_iterator::cell_iterator(p);++j){
				RESULT tmp=run(*j,0);
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}else{
		
			/*
			*	optimization : reason about given properties (bound or not)
			*	if one of the property is rdfs:domain, rdfs:range, then subject must be a Property
			*	if one of the property is rdfs:subClassOf then subject must be a Class
			*	more optimization:
			*		is the type given?
			*/		
			bool is_Property=false,is_Class=false;
			for(auto i=verbs.begin();i<verbs.end();++i){
				is_Property|=(i->p==rdfs::domain::get_property())||(i->p==rdfs::range::get_property());
				is_Class|=i->p==rdfs::subClassOf::get_property();
			}
			
			for(auto i=objrdf::begin();i<objrdf::end();++i){
				for(auto j=i.begin();j<i.end();++j){
					RESULT tmp=run(*j,0);
					r.insert(r.end(),tmp.begin(),tmp.end());
					if(r.size()>=n) return r;
				}
			}	
		}
		/*
		auto i=doc.begin();++i;//could be simpler
		for(auto i=doc.mm.begin();i!=doc.mm.end();++i)
			cerr<<i->first<<"\t"<<i->second->id<<endl;
		if(is_Class){
			cerr<<"################"<<endl;
			auto r=doc.mm.equal_range(rdfs::Class::get_class()->get<objrdf::c_index>().t);
			RESULT rr;
			for(auto j=r.first;j!=r.second;++j){
				RESULT tmp=run(j->second.get(),0);
				//cerr<<tmp<<endl;
				rr.insert(rr.end(),tmp.begin(),tmp.end());
			}
			to_xml(cerr,rr,*this);		
			cerr<<"################"<<endl;
		}
		if(is_Class){
			//use pools instead
			for(auto j=i->begin()+rdf::Property::get_instances().size();j<i->begin()+rdf::Property::get_instances().size()+rdfs::Class::get_instances().size();++j){
				RESULT tmp=run(j,0);
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}else if(is_Property){
			for(auto j=i->begin();j<i->begin()+rdf::Property::get_instances().size();++j){
				RESULT tmp=run(j,0);
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}else{
			for(auto j=i->begin();j<i->end();++j){
				RESULT tmp=run(j,0);
				r.insert(r.end(),tmp.begin(),tmp.end());
				if(r.size()>=n) return r;
			}
		}
		*/
	}
	return r;
}
RESULT subject::run(base_resource::const_instance_iterator i,PROPERTY_PTR p){
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
			bool result=(r==_r);
			if(!result) return RESULT();
		}else{
			LOG<<"binding R "<<this<<" to `"<<_r->id<<"'"<<endl;
		}		
		vector<RESULT> s;
		unsigned int n=0,m=1;
		//bind in case there is a cycle in the WHERE statement
		//cleaner would be to stow away 
		bool temp_bound=false;
		if(!r){
			r=_r;
			temp_bound=true;
		}	
		if(!busy){
			for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
				busy=true;
				RESULT tmp=j->run(_r);
				busy=false;
				if(tmp.empty()){
					if(temp_bound){
						r=SPARQL_RESOURCE_PTR();
						temp_bound=false;//not needed
					}
					return RESULT();
				}
				if(tmp.size()==1 && tmp.front().size()==0){

				}else{
					n+=tmp.front().size();//all the same size 
					m*=tmp.size();
					s.push_back(tmp);
				}
			}
		}
		if(temp_bound){
			r=SPARQL_RESOURCE_PTR();
			temp_bound=false;//not needed
		}
		RESULT ret=(r||!is_selected) ? RESULT(m) : RESULT(m,vector<base_resource::const_instance_iterator>(1,i));	
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
RESULT subject::run(SPARQL_RESOURCE_PTR _r,PROPERTY_PTR p){//p is not used?
	if(s.size()) return RESULT();
	if(r){
		LOG<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
		bool result=(r==_r);
		if(!result) return RESULT();
	}else{
		LOG<<"binding R "<<this<<" to `"<<_r->id<<"'"<<endl;
	}		
	vector<RESULT> s;
	unsigned int n=0,m=1;
	bool temp_bound=false;
	if(!r){
		r=_r;
		temp_bound=true;
	}	
	if(!busy){
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			busy=true;
			RESULT tmp=j->run(_r);
			busy=false;
			if(tmp.empty()){
				if(temp_bound){
					r=SPARQL_RESOURCE_PTR();
					temp_bound=false;//not needed
				}
				return RESULT();
			}
			if(tmp.size()==1 && tmp.front().size()==0){
			}else{
				n+=tmp.front().size();//all the same size 
				m*=tmp.size();
				s.push_back(tmp);
			}
		}
	}
	if(temp_bound){
		r=SPARQL_RESOURCE_PTR();
		temp_bound=false;//not needed
	}
	//need to craft a special base_resource::const_instance_iterator where we can fit info about _r
	base_resource::const_instance_iterator special(_r.operator->(),_v.cbegin(),0);//will crash if dereferenced!!!
	RESULT ret=(r||!is_selected) ? RESULT(m) : RESULT(m,vector<base_resource::const_instance_iterator>(1,special));
	//RESULT ret=(r||!is_selected) ? RESULT(m) : RESULT(m,vector<base_resource::const_instance_iterator>(1,base_resource::nil->cbegin()->cbegin()));//we lose information here	
	for(unsigned int i=0;i<m;++i){
		for(unsigned int j=0;j<s.size();++j){
			for(unsigned int k=0;k<s[j].front().size();++k){
				ret[i].push_back(s[j][i%s[j].size()][k]);		
			}
		}
	}	
	return ret;	
}

verb::verb(PROPERTY_PTR p,subject* object):p(p),object(object),is_optional(false),is_selected(true),bound(p){}
RESULT verb::run(SPARQL_RESOURCE_PTR r){
	if(p){
		LOG<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::const_type_iterator current_property=std::find_if(cbegin(r),cend(r),match_property(p));
		/*
 		*	maybe the sub class has that property
 		*/ 
		if(current_property!=cend(r)){
			RESULT ret;
			for(base_resource::const_instance_iterator j=current_property->cbegin();j!=current_property->cend();++j){
				//they are all the same size so we just stack them up
				RESULT tmp=object->run(j,p);
				LOG<<tmp<<endl;
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
			//we can use inference here too
			//we use superClassOf
			//more generic? if(p->get_Property()->get<rdfs::range>().t==rdfs::Class::get_class()){}
			//careful at exponential growth of results!!!, could use the rule only when object is bound.... 
			if(/*p==rdfs::range::get_property()||*/p==rdfs::domain::get_property()){
				if(current_property->cbegin()!=current_property->cend()){
					CLASS_PTR c(current_property->cbegin()->get_const_object());
					auto c_p=std::find_if(c->cbegin(),c->cend(),match_property(objrdf::superClassOf::get_property()));
					for(base_resource::const_instance_iterator j=c_p->cbegin();j!=c_p->cend();++j){
						//they are all the same size so we just stack them up
						RESULT tmp=object->run(j,p);
						LOG<<tmp<<endl;
						ret.insert(ret.end(),tmp.begin(),tmp.end());
					}
				}
			}
			return ret;
		}else{
			return RESULT();
		}
	}else{
		RESULT ret;
		for(base_resource::const_type_iterator i=cbegin(r);i!=cend(r);++i){
			base_resource::const_instance_iterator pt=i->get_Property()->get_const_self_iterator();
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
/*
 * ordering
 * we have typedef vector<vector<base_resource::instance_iterator> > RESULT;
 */
struct comp_r{
	int i;
	comp_r(int i):i(i){}
	bool operator()(const vector<base_resource::const_instance_iterator>& a,const vector<base_resource::const_instance_iterator>& b){
		return a[i].get_const_object()<b[i].get_const_object();	
	}	
};
/*	it would be neat to have an iterator based on a sparql query but with
 *	casting if all the types are the same, should the query be run first and 
 *	the result stored in a temporary array?
 */
void to_xml(ostream& os,/*const*/ RESULT& r,/*const*/ subject& s){
	/*
 	*	we should not serialize empty literal but hard to know
 	*	before serializing (could use buffer), 
 	*	could play with instance iterators...
 	*/
	//sort(r.begin(),r.end(),comp_r(1));
	os<</*"<?xml version=\"1.0\"?>\n*/"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">\n<head>\n";
	vector<string> v=s.get_variables();	
	for(vector<string>::const_iterator i=v.begin();i<v.end();++i) os<<"<variable name='"<<*i<<"'/>\n";
	os<<"</head>\n<results>\n";
	for(auto i=r.begin();i<r.end();++i){
		os<<"<result>\n";
		int v_i=0;
		for(auto j=i->cbegin();j<i->cend();++j){
			os<<"<binding name='"<<v[v_i++]<<"'>";
			//if(*j==base_resource::nil->cbegin()->cbegin()){
			if(j->i==subject::_v.cbegin()){
				os<<"<uri>";
				j->subject->id.to_uri(os);
				os<<"</uri>";
			}else{
				if(j->literalp())
					os<<"<literal>"<<*j<<"</literal>";
				else{
					os<<"<uri>";
					j->get_const_object()->id.to_uri(os);
					os<<"</uri>";
				}
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
				os<<">";
				if(d_resource)
					to_rdf_xml(d_resource,os);
				os<<"\n</"<<rdf::_RDF<<">\n";
			}break;
			case describe_q:{
				RESULT r=sbj->run();				
				os<<"<"<<rdf::_RDF<<"\n";
				uri::ns_declaration(os);
				os<<">";
				for(auto i=r.begin();i<r.end();++i){
					//only if resource
					for(auto j=i->begin();j<i->end();++j){
						if(!j->literalp())
							if(j->i==subject::_v.cbegin()){
								j->subject->to_rdf_xml(os);
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
			return parse_where_statement(*i);
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
			return parse_where_statement(*i);
		}break;
		case insert_data_query::id:{
			q=insert_data_q;	
			return parse_update_data_statement(r,false);
		}break;
		case delete_data_query::id:{
			q=delete_data_q;
			return parse_update_data_statement(r,true);
		}break;
		//case delete_data_statement::id://need to process where statement first
		//break;
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
bool sparql_parser::parse_where_statement(PARSE_RES_TREE& r){
	for(auto i=r.v.begin();i<r.v.end();++i){
		//cerr<<"current:\n"<<*i<<endl;
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
						SPARQL_RESOURCE_PTR r=find(uri::hash_uri(i->v[0].t.second));
						sbj=new subject(r ? r : base_resource::nil);
						index[i->v[0].t.second]=sbj;
						current_sbj=sbj;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							SPARQL_RESOURCE_PTR r=find(u);
							sbj=new subject(r ? r : base_resource::nil);
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
						current_sbj->verbs.push_back(verb(PROPERTY_PTR(),0));
						current_sbj->verbs.back().name=i->v[0].t.second;		
						current_sbj->verbs.back().is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
					}
					break;
					case turtle_parser::uriref::id:{
						PROPERTY_PTR r=find_t<PROPERTY_PTR>(uri::hash_uri(i->v[0].t.second));
						current_sbj->verbs.push_back(verb(r ? r : rdf::Property::nil,0));
						//use pool
						//auto r=find_if(begin<PROPERTY_PTR>(),end<PROPERTY_PTR>(),test_by_uri(uri::hash_uri(i->v[0].t.second)));
						//current_sbj->verbs.push_back(verb(r!=end<PROPERTY_PTR>() ? *r : rdf::Property::nil,0));
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							//auto r=find_if(begin<PROPERTY_PTR>(),end<PROPERTY_PTR>(),test_by_uri(u));
							//current_sbj->verbs.push_back(verb(r!=end<PROPERTY_PTR>() ? *r : rdf::Property::nil,0));
							PROPERTY_PTR r=find_t<PROPERTY_PTR>(u);
							current_sbj->verbs.push_back(verb(r ? r : rdf::Property::nil,0));
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::is_a::id:{
						current_sbj->verbs.push_back(verb(rdf::type::get_property(),0));
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
					case turtle_parser::uriref::id:{
						SPARQL_RESOURCE_PTR r=find(uri::hash_uri(i->v[0].t.second));
						subject* object=new subject(r ? r : base_resource::nil );//should get rid of that
						//cerr<<"####"<<r<<"\t"<<i->v[0].t.second<<endl;
						index[i->v[0].t.second]=object;
						current_sbj->verbs.back().object=object;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							SPARQL_RESOURCE_PTR r=find(u);
							subject* object=new subject(r ? r : base_resource::nil);
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
PROPERTY_PTR sparql_parser::parse_property(PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:{
			//return find_t<rdf::Property>(uri::hash_uri(r.t.second));
		}
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return find_t<PROPERTY_PTR>(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return PROPERTY_PTR();
			}
		}break;
		case turtle_parser::is_a::id:return rdf::type::get_property();
		default:return PROPERTY_PTR();	
	}
}
//won't parse literal
SPARQL_RESOURCE_PTR sparql_parser::parse_object(PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:return find(uri::hash_uri(r.t.second));break;
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return find(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return PROPERTY_PTR();
			}
		}break;
		default:return PROPERTY_PTR();//???	
	}
}
bool sparql_parser::parse_update_data_statement(PARSE_RES_TREE& r,bool do_delete){
/*
*	it is actually a plain turtle parser, it might not be practical to store thr document in a PARSE_RES_TREE,
*	we need a generic resource that can store literals and pointers, could use it for rdf_xml_parser as well
*	pair<uri,string> and 
*	pair<uri,SPARQL_RESOURCE_PTR >
*	it could be build 
*	we could break it down in triples and put on stack
*/
	RESOURCE_PTR sub;//subject, will be modified
	//problem base_resource::nil is a CONST_RESOURCE_PTR but we need a base_resource::type_iterator
	//base_resource::const_type_iterator current_property=base_resource::nil->cend();	
	//needs a default value
	base_resource::type_iterator current_property(0,objrdf::V::iterator());//=base_resource::nil->cend();	
	for(auto i=r.v.begin();i<r.v.end();++i){
		cerr<<"!!!current:\n"<<*i<<endl;
		switch(i->t.first){
			case turtle_parser::subject::id:{
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
							while(j<r.v.end()&&j->t.first!=turtle_parser::subject::id){
								if(j->t.first==turtle_parser::verb::id){
									cerr<<"verb:\n"<<*j<<endl;
									if(parse_property(j->v[0])==rdf::type::get_property()){
										cerr<<"rdf:type!"<<endl;
										//the next object will be the rdfs::Class
										CONST_RESOURCE_PTR r=parse_object((j+1)->v[0]);
										if(r&&get_class(r)==rdfs::Class::get_class()){
											cerr<<"rdfs:Class!"<<endl;
											CLASS_PTR c(r);
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
								while(j<r.v.end()&&j->t.first!=turtle_parser::subject::id){
									if(j->t.first==turtle_parser::verb::id){//is there a chance we get to next statement?
										cerr<<"verb:\n"<<*j<<endl;
										if(parse_property(j->v[0])==rdf::type::get_property()){
											cerr<<"rdf:type!"<<endl;
											//the next object will be the rdfs::Class
											SPARQL_RESOURCE_PTR r=parse_object((j+1)->v[0]);
											if(r&&get_class(r)==rdfs::Class::get_class()){
												cerr<<"rdfs:Class!"<<endl;
												CLASS_PTR c(r);
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
					default:return false;
				}
			}
			break;
			case turtle_parser::verb::id:{
				switch(i->v[0].t.first){
					case turtle_parser::uriref::id:{ 
						uri u=uri::hash_uri(i->v[0].t.second);
						current_property=std::find_if(begin(sub),end(sub),name_p(u));
						if(current_property==end(sub)){
							cerr<<"property `"<<u<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
							return false;
						}
					}break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							//find if property belongs to subject
							current_property=std::find_if(begin(sub),end(sub),name_p(u));
							if(current_property==end(sub)){
								cerr<<"property `"<<u<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
								return false;
							}
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::is_a::id:{
						current_property=begin(sub);//first property is rdf::type
					}break;
					default:return false;
				}
			}
			break;
			case turtle_parser::object::id:{
				switch(i->v[0].t.first){
					case turtle_parser::literal::id:{
						if(current_property.literalp()){
							if(do_delete){

							}else{
								istringstream is(i->v[0].v[0].t.second);
								current_property->add_property(p)->in(is);
								//could check if any char left in stream
							}
						}else{
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is not literal"<<endl;
							return false;
						}
					}
					break;
					case turtle_parser::uriref::id:{
						if(current_property.literalp()){
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is literal"<<endl;
							return false;
						}
						uri u=uri::hash_uri(i->v[0].t.second);
						if(do_delete){
							auto j=current_property->begin();
							while(j!=current_property->end()){
								if(j->get_object()->id==u){
									sub->erase(j);
									break;
								}	
								++j;
							}
						}else{
							SPARQL_RESOURCE_PTR r=find(u);
							if(r){
								//how do we support shared_ptr?
								current_property->add_property(p)->set_object(r);
							}else{
								cerr<<"resource `"<<u<<"' not found"<<endl;
								return false;
							}
						}
					}
					break;
					case turtle_parser::qname::id:{
						if(current_property.literalp()){
							cerr<<"current property `"<<current_property->get_Property()->id<<"' is literal"<<endl;
							return false;
						}
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							if(do_delete){
								auto j=current_property->begin();
								while(j!=current_property->end()){
									if(j->get_object()->id==u){
										sub->erase(j);
										break;
									}	
								}
							}else{
								SPARQL_RESOURCE_PTR r=find(u);
								if(r){
									current_property->add_property(p)->set_object(r);
								}else{
									cerr<<"resource `"<<u<<"' not found"<<endl;
									return false;
								}
							}
						}else{
							cerr<<"prefix `"<<i->v[0].v[0].t.second<<"' not associated with any namespace"<<endl;
							return false;
						}
					}
					break;
					default:return false;
				}
			}break;
			default:return false;
		}	
	}
	return true;
}
