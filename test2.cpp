// casting a regular structure to a tuple
#include "objrdf.h"
using namespace std;
using namespace objrdf;
struct s{
	char a;
	//short a;
	short b;//offset by 2 bytes
	int c;
}__attribute__((__packed__));
struct _a;typedef property<_a,short> p_a;
//struct _a;typedef property<_a,char> p_a;
struct _b;typedef property<_b,short> p_b;
struct _c;typedef property<_c,int> p_c;
struct _s_t;typedef resource<_s_t,duo<p_a,duo<p_b,p_c> >,NIL> s_t;

int main(){
	cout<<offsetof(s,a)<<"\t"<<offsetof(s,b)<<"\t"<<offsetof(s,c)<<endl;
	cout<<get_size<duo<p_a,duo<p_b,p_c> > >::VALUE<<endl;
	cout<<sizeof(s)<<"\t"<<sizeof(s_t)<<"\t"<<sizeof(NIL)<<endl;
	cout<<sizeof(p_a)<<"\t"<<sizeof(p_b)<<"\t"<<sizeof(p_c)<<endl;
	s r;
	r.a='q';
	r.b=1234;
	r.c=12345678;
	cout<<((s_t*)&r)->get<p_a>().t<<endl;
	cout<<((s_t*)&r)->get<p_b>().t<<endl;
	cout<<((s_t*)&r)->get<p_c>().t<<endl;
}

