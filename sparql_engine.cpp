#include "sparql_engine.h"
subject::subject(base_resource* r):r(r),is_selected(true),bound(r!=0),is_root(false),busy(false){}
subject::subject(string s):r(0),s(s),is_selected(true),bound(s.size()),is_root(false),busy(false){}
#define LOG if(0) cerr
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
bool subject::run(rdf::RDF& doc,int n){
	//if(r) return run(handle(r)._get());
	//bool ret=false;
	//not very efficient just to get a handle
	/*
	base_resource::type_iterator i(&doc,rdf::RDF::v.begin()+1);
	for(base_resource::instance_iterator j=i->begin();j<i->end();++j){
		ret|=run(j);
	}	
	*/
	//better
	int s=0;
	for(unsigned int i=0;i<doc.get<rdf::RDF::V>().size();++i){
		//ret|=run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		//s+=run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		/*
		RES tmp=run_0(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		cout<<tmp.first<<" "<<tmp.second<<endl;
		::print_subject(tmp.second);
		cout<<endl;
		::result< ::binding> r= ::normalize(tmp.second);
		::normalize(r);
		*/
		RESULT tmp=run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		LOG<<tmp<<endl;
		//RESULT r=_run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		//cout<<r<<endl;
		if(s>=n) return true;
	}
	//return ret;
	//return s;
	return true;
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
				LOG<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
				return is_selected ? RESULT(1,vector<base_resource::instance_iterator>(1,i)) : RESULT(1);
			}
		}
	}else{
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

