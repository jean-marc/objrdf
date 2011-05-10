#include "sparql_engine.h"
//#include "cartesian.h"
subject::subject(base_resource* r):is_selected(true),r(r),busy(false),root(false),bound(r!=0){}
subject::subject(string s):is_selected(true),r(0),s(s),busy(false),root(false),bound(s.size()){}
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
	return n+o->size();
}
int verb::last_binding=0;
/*
ostream& operator<<(ostream& os,base_resource::instance_iterator&){
	return os;
}
*/
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
		RES_1 tmp=run_1(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		cout<<tmp<<endl;
		//RESULT r=_run(base_resource::instance_iterator(&doc,rdf::RDF::v.begin()+1,i));
		//cout<<r<<endl;
		if(s>=n) return true;
	}
	//return ret;
	//return s;
	return true;
}
result<binding> normalize(result<binding>& r){
	int n=1;
	for(vector<result<binding> >::iterator i=r.v.begin();i<r.v.end();++i){
		//normalize(*i);
		n*=i->v.size();
	}
	cout<<"tot size:"<<n<<endl;
	result<binding> rr(binding(base_resource::nil->begin()->begin(),false));
	//rr.v.resize(n);
	//similar to cartesian product
	/*
 	* 	(_2(@(0))(@(0)(1)))	
 	* 	(_2(@(0))(@(0)))(_2(@(0))(@(1)))	
 	*/ 
	for(int i=0;i<n;++i){
		rr.v.push_back(r.t);
		for(int j=0;j<r.v.size();++j){
			rr.v.back().v.push_back(r.v[j].v[i%r.v[j].v.size()]);
		}
	}
	cout<<rr<<endl;
	return rr;
}
void print_subject(result<binding>& r){
	//we must use base_resource::instance_iterator::operator== 
	if(r.t.first==base_resource::nil->begin()->begin()){
		
	}else{
		if(r.t.second){
			cout<<*r.t.first<<"\t";
		}else{ 
			cout<<r.t.first->get_Property()->id<<"\t";
		}
	}
	/*
 	*	make sure the data is consistent
 	*/	 
	for(vector<result<binding> >::iterator i=r.v.begin();i<r.v.end();++i){
		print_verb(*i,r.v.size()==1);
	}
}
void print_verb(result<binding>& r,bool deep){
	if(r.t.first==base_resource::nil->begin()->begin()){
		
	}else{
		if(r.t.second){
			cout<<*r.t.first<<"\t";
		}else{ 
			cout<<r.t.first->get_Property()->id<<"\t";
		}
	}
	if(deep){
		for(vector<result<binding> >::iterator i=r.v.begin();i<r.v.end();++i){
			print_subject(*i);
			cout<<endl;
		}
	}else{
		for(vector<result<binding> >::iterator i=r.v.begin();i<min(r.v.begin()+1,r.v.end());++i){
			print_subject(*i);
		}
	}
}
void normalize(){
	/*
 	*	1 p_0 2,3,4.
 	*	SELECT $x,$y WHERE {$x p_0 $y.}
 	*
 	*	cartesian product
 	*	{1}x{2,3,4}={(1,2),(1,3),(1,4)}
 	*	results:
 	*	$x	$y
 	*	1	2
 	*	1	3
 	*	1	4	
 	*
 	*	1 p_0 2,3,4;p_1 5,6.
 	*	SELECT $x,$y,$z WHERE {$x p_0 $y;p_1 $z.}
 	*	{1}x{2,3,4}x{5,6}	={1}x{(2,5),(2,6),(3,5),(3,6),(4,5),(4,6)}
 	*				={(1,2,5),(1,2,6),(1,3,5),(1,3,6),(1,4,5),(1,4,6)}
 	*	results:
 	*	$x	$y	$z
 	*	1	2	5
 	*	1	3	5
 	*	1	4	5
 	*	1	2	6
 	*	1	3	6
 	*	1	4	6
 	*
 	* 	the problem is that we don't know if the query is successful until we're back at
 	* 	root element
 	*/ 	
}
RESULT subject::_run(base_resource::instance_iterator i){
	RESULT res;
	if(r){//bound to resource
		cout<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
		if(i.literalp()) return res;
		base_resource* _r=i->get_object().get();			
		if(r!=_r) return res;
		//if(busy) return true;//
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			res=cartesian(res,j->_run(r));//could have early failure
			//if(!j->run(_r)) return false;//should reset results
		}			
	}else if(s.size()){//bound to literal
		cout<<"bound\tR "<<this<<" to `"<<i.str()<<"'"<<endl;
		return (i.literalp()&&i.str()==s) ? RESULT(1):res;
	}else{
		if(i.literalp()){
			if(verbs.size()) return res;//a literal does not have properties
			cout<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
			//result.push_back(i);
			res.push_back(vector<base_resource::instance_iterator>(1,i));
		}else{
			base_resource* _r=i->get_object().get();			
			cout<<"binding R "<<this<<" to `"/*<<_r<<"\t"*/<<_r->id<<"'"<<endl;
			res.push_back(vector<base_resource::instance_iterator>(1,i));
			cout<<"@"<<res<<endl;
			r=_r;
			busy=true;//to handle loop
			/*
 			*	
 			*
 			*/ 
			for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
				RESULT
				res=cartesian(res,j->_run(r));
				/*if(!j->run(_r)){
					r=0;
					busy=false;
					return false;
				}*/
			}			
			r=0;
			busy=false;
			//cartesian product
			//we could reset results in branches
			//result.push_back(i);
		}
	}	
	cout<<"return:"<<res<<endl;
	return res;
	//return true;
}
int verb::get_binding_size(){
	if(!bound) return result.size();
	if(o) return o->get_binding_size();
	return 0;
}
int subject::get_binding_size(){
	if(!bound) return result.size();
	if(verbs.size()) return verbs[0].get_binding_size();//should all be the same size
	return 0;
}
bool subject::run(base_resource::instance_iterator i){
	if(r){//bound to resource
		cout<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
		if(i.literalp()) return false;
		base_resource* _r=i->get_object().get();			
		if(r!=_r) return false;
		if(busy) return true;
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			cout<<"@"<<j->get_binding_size()<<endl;
			if(!j->run(_r)) return false;//should reset results
			cout<<"@"<<j->get_binding_size()<<endl;
		}			
	}else if(s.size()){//bound to literal
		cout<<"bound\tR "<<this<<" to `"<<i.str()<<"'"<<endl;
		return(i.literalp()&&i.str()==s);
	}else{
		if(i.literalp()){
			if(verbs.size()) return false;//a literal does not have properties
			cout<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
			if(is_selected) result.push_back(i);
			//cout<<"size:"<<result.size()<<"\t"<<&result<<endl;
		}else{
			base_resource* _r=i->get_object().get();			
			cout<<"binding R "<<this<<" to `"/*<<_r<<"\t"*/<<_r->id<<"'"<<endl;
			r=_r;
			busy=true;//to handle loop
			for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
				cout<<"@"<<j->get_binding_size()<<endl;
				if(!j->run(_r)){
					r=0;
					busy=false;
					return false;
				}
				cout<<"@"<<j->get_binding_size()<<endl;
			}			
			r=0;
			busy=false;
			//we could reset results in branches
			/*
 			*	we need to find out how many bindings were created 
 			*/ 
			if(is_selected){
				result.push_back(i);

			}
		}
	}	
	//normalization
	cout<<"normalizing:"<<endl;
	int nn=1;//total size
	for(vector<verb>::iterator i=verbs.begin();i<verbs.end();++i){
		int n=i->get_binding_size();
		nn*=n;	
	}
	
	return true;
}
RES subject::run_0(base_resource::instance_iterator i){
	if(r){//bound to resource
		cout<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
		if(i.literalp()) return RES(false,binding(i,true));
		base_resource* _r=i->get_object().get();			
		if(r!=_r) return RES(false,binding(base_resource::nil->begin()->begin(),true));
		if(busy) return RES(true,binding(base_resource::nil->begin()->begin(),true));
		RES ret(true,binding(base_resource::nil->begin()->begin(),true));
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			RES tmp=j->run_0(_r);
			if(!tmp.first) return RES(false,binding(base_resource::nil->begin()->begin(),true));
			ret.second.v.push_back(tmp.second);	
		}			
		return ret;
	}else if(s.size()){//bound to literal
		cout<<"bound\tR "<<this<<" to `"<<i.str()<<"'"<<endl;
		return RES(i.literalp()&&i.str()==s,binding(base_resource::nil->begin()->begin(),true));
	}else{
		if(i.literalp()){
			if(verbs.size()) return RES(false,binding(i,true));//a literal does not have properties
			cout<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
			if(is_selected) return RES(true,binding(i,true));
		}else{
			base_resource* _r=i->get_object().get();			
			cout<<"binding R "<<this<<" to `"/*<<_r<<"\t"*/<<_r->id<<"'"<<endl;
			r=_r;
			busy=true;//to handle loop
			RES ret(true,binding(i,true));
			for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
				RES tmp=j->run_0(_r);
				if(!tmp.first){
					r=0;
					busy=false;
					return RES(false,binding(base_resource::nil->begin()->begin(),true));
				}
				ret.second.v.push_back(tmp.second);
			}			
			r=0;
			busy=false;
			return ret;
		}
	}	
	return RES(true,binding(i,true));

}
RES_1 subject::run_1(base_resource::instance_iterator i){
	if(i.literalp()){
		if(r){
			return RES_1(0);
		}else{
			if(verbs.size()) return RES_1(0);
			if(s.size()){
				cout<<"bound\tR "<<this<<" to `"<<s<<"'"<<endl;
				return (i.str()==s) ? RES_1(1) : RES_1(0);
			}else{
				cout<<"binding R "<<this<<" to `"<<i.str()<<"'"<<endl;
				return is_selected ? RES_1(1,vector<base_resource::instance_iterator>(1,i)) : RES_1(1);
			}
		}
	}else{
		base_resource* _r=i->get_object().get();			
		if(r){
			cout<<"bound\tR "<<this<<" to `"<<r->id<<"'"<<endl;
			if(r!=_r) return RES_1();
		}else{
			cout<<"binding R "<<this<<" to `"<<_r->id<<"'"<<endl;
		}		
		vector<RES_1> s;
		int n=0,m=1;
		for(vector<verb>::iterator j=verbs.begin();j<verbs.end();++j){
			RES_1 tmp=j->run_1(_r);
			if(tmp.empty()) return RES_1();
			if(tmp.size()==1 && tmp.front().size()==0){

			}else{
				n+=tmp.front().size();//all the same size 
				m*=tmp.size();
				s.push_back(tmp);
			}
		}
		RES_1 ret=r ? RES_1(m) : RES_1(m,vector<base_resource::instance_iterator>(1,i));	
		for(int i=0;i<m;++i){
			for(int j=0;j<s.size();++j){
				for(int k=0;k<s[j].front().size();++k){
					ret[i].push_back(s[j][i%s[j].size()][k]);		
				}
			}
		}	
		return ret;	
	}
}
void subject::print(unsigned int index){
	if(!busy){
		if(!r){
			cout.width(30);
			if(index<result.size())
				//cout<<result[index].get_object().get()<<"\t";//cout<<*result[index]<<"\t";
				cout<<*result[index];//<<"\t";
			else
				cout<<"??";//\t";
		}
	//if(!busy){
		busy=true;
		for(vector<verb>::iterator i=verbs.begin();i<verbs.end();++i) i->print(index);
		busy=false;
	}
}
void subject::print(){
	for(unsigned int i=0;i<2*result.size();++i){
		print(i);
		cout<<endl;
	}
}
void subject::_print(){
	if(!r) cout<<this<<"\t";	
	for(vector<verb>::iterator i=verbs.begin();i<verbs.end();++i) i->_print();
}
void subject::normalize(){
	//depth first
	for(vector<verb>::iterator i=verbs.begin();i<verbs.end();++i){

	}
	
}
verb::verb(rdf::Property* p,subject* o):is_optional(false),is_selected(true),p(p),o(o),bound(p!=0){}
verb::verb():is_optional(false),is_selected(true),p(0),o(0),bound(false){}
RESULT verb::_run(base_resource* r){
	RESULT res;
	if(p){
		cout<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::type_iterator current_property=std::find_if(r->begin(),r->end(),match_property(p));
		if(current_property!=r->end()){
			for(base_resource::instance_iterator j=current_property->begin();j!=current_property->end();++j){
				RESULT tmp=o->_run(j);
				if(tmp.size()) res=cartesian(res,tmp);
			}
		}
	}else{
		//<subject> $p ...
		//not bound go through all properties
		//we need at least one match
		for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
				RESULT tmp(1,vector<base_resource::instance_iterator>(1,j));
				cout<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				tmp=cartesian(tmp,o->_run(j));
				if(tmp.size()) res=cartesian(res,tmp);
			}
		}
	}
	return res;
}
bool verb::run(base_resource* r){
	if(p){
		cout<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::type_iterator current_property=std::find_if(r->begin(),r->end(),match_property(p));
		if(current_property!=r->end()){
			//bool ret=false; //there should be at least one match
			bool ret=is_optional;
			for(base_resource::instance_iterator j=current_property->begin();j!=current_property->end();++j) 
				ret|=o->run(j);
			return ret;
		}else{
			return false;
		}
	}else{
		//<subject> $p ...
		//not bound go through all properties
		//we need at least one match
		//bool ret=false;
		bool ret=is_optional;
		for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
				cout<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				if(o->run(j)){
					ret=true;
					if(is_selected) result.push_back(i->get_Property().get());
				}
			}
		}
		return ret;
	}
}
RES verb::run_0(base_resource* r){
	if(p){
		cout<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::type_iterator current_property=std::find_if(r->begin(),r->end(),match_property(p));
		if(current_property!=r->end()){
			//bool ret=false; //there should be at least one match
			RES ret(is_optional,binding(base_resource::nil->begin()->begin(),false));
			for(base_resource::instance_iterator j=current_property->begin();j!=current_property->end();++j){
				RES tmp=o->run_0(j);
				if(tmp.first){
					ret.second.v.push_back(tmp.second);
				}
			}
			ret.first|=ret.second.v.size();
			return ret;
		}else{
			return RES(false,binding(base_resource::nil->begin()->begin(),false));
		}
	}else{
		RES ret(is_optional,binding(base_resource::nil->begin()->begin(),false));
		for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
				cout<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				//only add if positive
				RES tmp=o->run_0(j);
				if(tmp.first){
					ret.second.v.push_back(binding(j,false));//need to tell if we are interested in property or object
					ret.second.v.back().v.push_back(tmp.second);
				}
			}
		}
		ret.first|=ret.second.v.size();
		return ret;
	}
}
RES_1 verb::run_1(base_resource* r){
	if(p){
		cout<<"bound\tP "<<this<<" to `"<<p->id<<"'"<<endl;	
		base_resource::type_iterator current_property=std::find_if(r->begin(),r->end(),match_property(p));
		if(current_property!=r->end()){
			RES_1 ret;
			for(base_resource::instance_iterator j=current_property->begin();j!=current_property->end();++j){
				//they are all the same size so we just stack them up
				RES_1 tmp=o->run_1(j);
				cout<<tmp<<endl;
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
			return ret;
		}else{
			return RES_1();
		}
	}else{
		RES_1 ret;
		for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
			base_resource::instance_iterator pt=i->get_Property()->get_self_iterator();
			for(base_resource::instance_iterator j=i->begin();j!=i->end();++j){
				cout<<"binding P "<<this<<" to `"<<i->get_Property()->id<<"'"<<endl;	
				RES_1 tmp=o->run_1(j);
				cout<<tmp<<endl;
				for(auto k=tmp.begin();k<tmp.end();++k) k->insert(k->begin(),pt);
				ret.insert(ret.end(),tmp.begin(),tmp.end());
			}
		}
		return ret;
	}
}

void verb::print(unsigned int index){
	if(!p){
		cout.width(30);
		if(index<result.size())
			cout<</*result[index]<<" "<<*/result[index]->id;//<<"\t";
		else
			cout<<"??";//\t";
	}
	if(o) o->print(index);
}
void verb::_print(){
	if(!p) cout<<this<<"\t";
	o->_print();	
}

