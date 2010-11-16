#include "sparql_parser.h"
#include <sstream>
#include <algorithm>
bool is_variable(string s){return (!s.empty())&&((s[0]=='?')||(s[0]=='$'));}
ret _resource::_bind(string s){
	ret r(false,0);
	if(is_variable(name)){
		//cerr<<"binding "<<name<<" to "<<s<<endl;
		r.first=p.empty();	
		r.second=new binding;		
		r.second->name=name;
		r.second->s=s;
	}else{
		r.first=(name==s);
	}
	return r;
}
ret _resource::_bind(shared_ptr<base_resource> r){
	ret rr(false,0);
	cerr<<"##"<<name<<endl;
	if(is_variable(name)){
		//cerr<<"binding "<<name<<" to "<<r->id<<endl;
		rr.second=new binding;		
		rr.second->name=name;
		rr.second->s=r->id;
		rr.second->r=r;
	}else{
		if(name!=r->id.name) return rr;
	}
	bool m=true;
	for(vector<predicate>::iterator k=p.begin();k<p.end();++k){
		bool mm=false;
		binding* current=new binding;//leak!
		if(is_variable(k->name)){
			for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
				binding* _current=new binding;//leak!
				for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					ret tmp=k->_bind(j);
					if(tmp.first){
						_current->floor()->down=tmp.second;	
						mm=true;	
					}
				}
				if(rr.second)
					rr.second->end()->right=_current->down;//might be 0	
				else
					rr.second=_current->down;
				_current->down=0;
				delete _current;
			}
		}else{
			base_resource::type_iterator i=std::find_if(r->begin(),r->end(),namep(k->name));
			if(i!=r->end()){
				for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					ret tmp=k->_bind(j);
					if(tmp.first){
						current->floor()->down=tmp.second;	
						mm=true;	
					}
				}
			}
		}
		if(rr.second)
			rr.second->end()->right=current->down;//might be 0	
		else
			rr.second=current->down;
		m=m&&mm;	
		current->down=0;
		delete current;
	}
	rr.first=m;
	/*
	if(rr.second){
		rr.first=m;
		rr.second->right=current->down;
		//rr.second->right=current;
	}else{
		rr.first=m;
		rr.second=current->down;
		//rr.second=current;
	}
	*/
	return rr;
}
ret _resource::_bind(base_resource::instance_iterator i){
	return i->literalp() ? _bind(i->str()): _bind(i->get_object());
}
ret predicate::_bind(base_resource::instance_iterator i){
	ret rr(false,0);
	if(is_variable(name)){
		//cerr<<"binding "<<name<<" to "<<i->get_Property()->id<<endl;
		rr.second=new binding;	
		rr.second->name=name;
		rr.second->s=i->get_Property()->id;
		rr.second->r=i->get_Property();
		ret tmp=object->_bind(i);
		rr.first=tmp.first;
		rr.second->right=tmp.second;
		return rr;
	}
	return object->_bind(i);
}
/*
bool _resource::_bind(binding& b,string s){
	if(is_variable(name)){
		cerr<<"binding "<<name<<" to "<<s<<endl;
		b.name=name;
		b.s=s;
	}else{
		if(name!=s) return false;
	}
	return p.empty();
}
*/
/*
bool _resource::_bind(binding& b,shared_ptr<base_resource> r){
	if(is_variable(name)){
		cerr<<"binding "<<name<<" to "<<r->id<<endl;
		b.name=name;
		b.s=r->id;
		b.r=r;
	}else{
		if(name!=r->id.name) return false;
	}
	bool m=true;
	for(vector<predicate>::iterator k=p.begin();k<p.end();++k){
		bool mm=false;
		if(is_variable(k->name)){
			for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
				for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					binding tmp;
					if(k->_bind(tmp,j)){
						b.v.push_back(tmp);
						mm=true;	
					}
					//mm=k->_bind(j)||mm;
				}
			}
		}else{
			base_resource::type_iterator i=std::find_if(r->begin(),r->end(),namep(k->name));
			if(i!=r->end()){
				for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
					binding tmp;
					if(k->_bind(tmp,j)){
						b.v.push_back(tmp);
						mm=true;	
					}
					//mm=k->_bind(j)||mm;
				}
			}
		}
		m=m&&mm;	
	}
	return m;
}
bool _resource::_bind(binding& b,base_resource::instance_iterator i){
	return i->literalp() ? _bind(b,i->str()): _bind(b,i->get_object());
}
bool predicate::_bind(binding& b,base_resource::instance_iterator i){
	if(is_variable(name)){
		cerr<<"binding "<<name<<" to "<<i->get_Property()->id<<endl;
		b.p=i;
		b.name=name;
		b.s=i->get_Property()->id;
		b.v.push_back(binding());
		return object->_bind(b.v.back(),i);
	}
	return object->_bind(b,i);
}
*/
ostream& operator<<(ostream& os,_resource& r){
	os<<"("<<r.name<<" ";
	for(vector<predicate>::iterator i=r.p.begin();i<r.p.end();++i) os<<*i;
	return os<<")";
}
ostream& operator<<(ostream& os,predicate& p){
	os<<"("<<p.name;
	if(p.object.get()) os<<" "<<*p.object;
	return os<<")";
}	


