#include "sparql_engine.h"
subject::subject(base_resource* r):r(r),is_selected(true),bound(r!=0),is_root(false),busy(false){}
subject::subject(string s):r(0),s(s),is_selected(true),bound(s.size()),is_root(false),busy(false){}
//#define LOG if(0) cerr
//subject subject::bound(base_resource* r){return subject(r);}
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
vector<string> subject::get_variables() const{
	vector<string> v;
	if(!bound && is_selected) v.push_back(name.substr(1));
	for(vector<verb>::const_iterator j=verbs.begin();j<verbs.end();++j){
		vector<string> tmp=j->get_variables();
		v.insert(v.end(),tmp.begin(),tmp.end());
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
RESULT subject::run(rdf::RDF& doc,size_t n){
	RESULT r;
	//optimization when subject is bound
	//but we need an iterator to it
	if(bound){
		doc.get<rdf::RDF::V>().push_back(shared_ptr<base_resource>(this->r));
		r=run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,doc.get<rdf::RDF::V>().size()-1));
		doc.get<rdf::RDF::V>().pop_back();//what if document modified during process??
	}else{
		//for(unsigned int i=0;i<doc.get<rdf::RDF::V>().size();++i){
		//	RESULT tmp=run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		auto i=doc.begin();++i;//could be simpler
		for(auto j=i->begin();j<i->end();++j){
			RESULT tmp=run(j);
			r.insert(r.end(),tmp.begin(),tmp.end());
			if(r.size()>=n) return r;
		}
	}
	return r;
}
RESULT subject::run(base_resource::instance_iterator i){
	if(i.literalp()){
		if(r){
			return RESULT(0);
		}else{
			if(verbs.size()) return RESULT(0);
			if(s.size()){
				LOG<<"bound\tR "<<this<<" to `"<<s<<"'"<<endl;
				return (i.str()==s) ? RESULT(1) : RESULT(0);
			}else{
				if(i.get_Property()->get<rdfs::range>()==rdfs::XML_Literal::get_class()){
					/*
 					*	shouldn't bind if empty, shouldn't be here in the first place
 					*/ 
					LOG<<"binding R "<<this<<" to XML_Literal ..."<<endl;
				}else{
					LOG<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
				}
				return is_selected ? RESULT(1,vector<base_resource::instance_iterator>(1,i)) : RESULT(1);
			}
		}
	}else{
		/*
 		*	what if bound to literal??
 		*/ 
		if(s.size()) return RESULT();
		base_resource* _r=i->get_object().get();			
		if(r){
			LOG<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
			if(r!=_r) return RESULT();
		}else{
			LOG<<"binding R "<<this<<" to `"<<_r->id<<"'"<<endl;
		}		
		vector<RESULT> s;
		unsigned int n=0,m=1;
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			RESULT tmp=j->run(_r);
			if(tmp.empty()) return RESULT();
			if(tmp.size()==1 && tmp.front().size()==0){

			}else{
				n+=tmp.front().size();//all the same size 
				m*=tmp.size();
				s.push_back(tmp);
			}
		}
		RESULT ret=(r||!is_selected) ? RESULT(m) : RESULT(m,vector<base_resource::instance_iterator>(1,i));	
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
verb::verb(shared_ptr<rdf::Property> p,subject* object):p(p),object(object),is_optional(false),is_selected(true),bound(p){}
RESULT verb::run(base_resource* r){
	if(p){
		LOG<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::type_iterator current_property=std::find_if(r->begin(),r->end(),match_property(p.get()));
		if(current_property!=r->end()){
			RESULT ret;
			for(base_resource::instance_iterator j=current_property->begin();j!=current_property->end();++j){
				//they are all the same size so we just stack them up
				RESULT tmp=object->run(j);
				LOG<<tmp<<endl;
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
			return ret;
		}else{
			return RESULT();
		}
	}else{
		RESULT ret;
		for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
			base_resource::instance_iterator pt=i->get_Property()->get_self_iterator();
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
				LOG<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				RESULT tmp=object->run(j);
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

void to_xml(ostream& os,const RESULT& r,const subject& s){
	/*
 	*	we should not serialize empty literal but hard to know
 	*	before serializing (could use buffer), 
 	*	could play with instance iterators...
 	*/
	os<</*"<?xml version=\"1.0\"?>\n*/"<sparql xmlns=\"http://www.w3.org/2005/sparql-results#\">\n<head>\n";
	vector<string> v=s.get_variables();	
	for(vector<string>::const_iterator i=v.begin();i<v.end();++i) os<<"<variable name='"<<*i<<"'/>\n";
	os<<"</head>\n<results>\n";
	for(auto i=r.begin();i<r.end();++i){
		os<<"<result>\n";
		int v_i=0;
		for(auto j=i->begin();j<i->end();++j){
			os<<"<binding name='"<<v[v_i++]<<"'>";
			if(j->literalp())
				os<<"<literal>"<<*j<<"</literal>";
			else{
				os<<"<uri>";
				j->get_object()->id.to_uri(os);
				os<<"</uri>";
			}
			os<<"</binding>\n";
		}
		os<<"</result>\n";
	}
	os<<"</results>\n</sparql>\n";

}
sparql_parser::sparql_parser(rdf::RDF& doc,istream& is):char_iterator(is),doc(doc),sbj(0),current_sbj(0),q(no_q){
	//non standard but helpful
	prefix_ns["rdf"]="http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	prefix_ns["rdfs"]="http://www.w3.org/2000/01/rdf-schema#";
}
bool sparql_parser::go(){
	if(!document::go(*this)) return false;
	if(q==select_q||q==describe_q) return sbj;
	return true;
}
void sparql_parser::out(ostream& os){//sparql XML serialization
	//if(sbj){
		switch(q){
			case select_q:
				to_xml(os,sbj->run(doc),*sbj);
			break;
			case simple_describe_q:{
				os<<"<"<<rdf::_RDF<<"\n";
				uri::ns_declaration(os);
				os<<">";
				if(d_resource.get())
					d_resource->to_rdf_xml(os);
				os<<"\n</"<<rdf::_RDF<<">\n";
			}break;
			case describe_q:{
				RESULT r=sbj->run(doc);				
				os<<"<"<<rdf::_RDF<<"\n";
				uri::ns_declaration(os);
				os<<">";
				for(auto i=r.begin();i<r.end();++i){
					//only if resource
					for(auto j=i->begin();j<i->end();++j){
						if(!j->literalp())
							j->get_object()->to_rdf_xml(os);
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
			//d_resource=doc.query(uri::hash_uri(r.v[0].t.second));	
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
						shared_ptr<base_resource> r=doc.query(uri::hash_uri(i->v[0].t.second));
						sbj=new subject(r.get() ? r.get() : base_resource::nil.get());
						index[i->v[0].t.second]=sbj;
						current_sbj=sbj;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							shared_ptr<base_resource> r=doc.query(u);
							sbj=new subject(r.get() ? r.get() : base_resource::nil.get());
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
						current_sbj->verbs.push_back(verb(shared_ptr<rdf::Property>(),0));
						current_sbj->verbs.back().name=i->v[0].t.second;		
						current_sbj->verbs.back().is_selected=variable_set.empty()||variable_set.count(i->v[0].t.second);
					}
					break;
					case turtle_parser::uriref::id:{
						shared_ptr<rdf::Property> r=doc.query_t<rdf::Property>(uri::hash_uri(i->v[0].t.second));
						current_sbj->verbs.push_back(verb(r ? r : rdf::Property::nil,0));
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							shared_ptr<rdf::Property> r=doc.query_t<rdf::Property>(u);
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
						shared_ptr<base_resource> r=doc.query(uri::hash_uri(i->v[0].t.second));
						subject* object=new subject(r.get() ? r.get() : base_resource::nil.get() );
						index[i->v[0].t.second]=object;
						current_sbj->verbs.back().object=object;
					}
					break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							shared_ptr<base_resource> r=doc.query(u);
							subject* object=new subject(r.get() ? r.get() : base_resource::nil.get() );
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
shared_ptr<rdf::Property> sparql_parser::parse_property(PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:return doc.query_t<rdf::Property>(uri::hash_uri(r.t.second));
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return doc.query_t<rdf::Property>(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return shared_ptr<rdf::Property>();
			}
		}break;
		case turtle_parser::is_a::id:return rdf::type::get_property();
		default:return shared_ptr<rdf::Property>();	
	}
}
//won't parse literal
shared_ptr<base_resource> sparql_parser::parse_object(PARSE_RES_TREE& r){
	switch(r.t.first){
		case turtle_parser::uriref::id:return doc.query(uri::hash_uri(r.t.second));
		case turtle_parser::qname::id:{
			PREFIX_NS::iterator j=prefix_ns.find(r.v[0].t.second);
			if(j!=prefix_ns.end()){
				uri u(j->second,r.v[1].t.second);
				return doc.query(u);
			}else{
				cerr<<"prefix `"<<r.v[0].t.second<<"' not associated with any namespace"<<endl;
				return shared_ptr<rdf::Property>();
			}
		}break;
		default:return shared_ptr<rdf::Property>();	
	}
}
bool sparql_parser::parse_update_data_statement(PARSE_RES_TREE& r,bool do_delete){
/*
*	it is actually a plain turtle parser, it might not be practical to store thr document in a PARSE_RES_TREE,
*	we need a generic resource that can store literals and pointers, could use it for rdf_xml_parser as well
*	pair<uri,string> and 
*	pair<uri,shared_ptr<base_resource> >
*	it could be build 
*	we could break it down in triples and put on stack
*/
	shared_ptr<base_resource> sub;//subject
	base_resource::type_iterator current_property=base_resource::nil->end();	
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
						sub=doc.query(u);
						if(!sub){//we have to create the resource but we need to know its type, we have access to the whole document
							//we assume that the types will be given in this statement
							auto j=i+1;
							while(j<r.v.end()&&j->t.first!=turtle_parser::subject::id){
								if(j->t.first==turtle_parser::verb::id){
									cerr<<"verb:\n"<<*j<<endl;
									if(parse_property(j->v[0])==rdf::type::get_property()){
										cerr<<"rdf:type!"<<endl;
										//the next object will be the rdfs::Class
										shared_ptr<base_resource> r=parse_object((j+1)->v[0]);
										if(r.get()&&r->get_Class()==rdfs::Class::get_class().get()){
											cerr<<"rdfs:Class!"<<endl;
											shared_ptr<rdfs::Class> c=static_pointer_cast<rdfs::Class>(r);
											if(c->f){
												sub=shared_ptr<base_resource>(c->f(u));
												doc.insert(sub);
											}else{
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
							sub=doc.query(u);
							if(!sub){
								auto j=i+1;
								while(j<r.v.end()&&j->t.first!=turtle_parser::subject::id){
									if(j->t.first==turtle_parser::verb::id){//is there a chance we get to next statement?
										cerr<<"verb:\n"<<*j<<endl;
										if(parse_property(j->v[0])==rdf::type::get_property()){
											cerr<<"rdf:type!"<<endl;
											//the next object will be the rdfs::Class
											shared_ptr<base_resource> r=parse_object((j+1)->v[0]);
											if(r.get()&&r->get_Class()==rdfs::Class::get_class().get()){
												cerr<<"rdfs:Class!"<<endl;
												shared_ptr<rdfs::Class> c=static_pointer_cast<rdfs::Class>(r);
												if(c->f){
													sub=shared_ptr<base_resource>(c->f(u));
													doc.insert(sub);
												}else{
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
						current_property=std::find_if(sub->begin(),sub->end(),namep(u));
						if(current_property==sub->end()){
							cerr<<"property `"<<u<<"' does not belong to resource `"<<sub->id<<"'"<<endl;
							return false;
						}
					}break;
					case turtle_parser::qname::id:{
						PREFIX_NS::iterator j=prefix_ns.find(i->v[0].v[0].t.second);
						if(j!=prefix_ns.end()){
							uri u(j->second,i->v[0].v[1].t.second);
							//find if property belongs to subject
							current_property=std::find_if(sub->begin(),sub->end(),namep(u));
							if(current_property==sub->end()){
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
						current_property=sub->begin();//first property is rdf::type
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
								current_property->add_property()->in(is);
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
							shared_ptr<base_resource> r=doc.query(u);
							if(r.get()){
								current_property->add_property()->set_object(r);
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
								shared_ptr<base_resource> r=doc.query(u);
								if(r.get()){
									current_property->add_property()->set_object(r);
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
