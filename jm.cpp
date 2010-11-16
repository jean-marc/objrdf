#include "jm.h"

struct B:bi<B>{
	int i;
	friend ostream& operator<<(ostream& os,B& b){return os<<b.i;}
	B(){}
	B(int _i):i(_i){}
};
//typedef bi<int> B;
//typedef bi<t> B;

int main(){
	int i=0;
	B b(++i);
	b.right=new B(++i);
	b.end()->down=new B(++i);
	b.floor()->down=new B(++i);
	b.floor()->down=new B(++i);
	b.floor()->end()->right=new B(++i);
	b.floor()->end()->right=new B(++i);
	b.floor()->end()->down=new B(++i);
	b.floor()->end()->floor()->down=new B(++i);
	b.floor()->down=new B(++i);
	//cout<<b<<endl;
	b.serialize(cout);
	b.normalize();
	//cout<<endl<<b<<endl;
	cout<<endl;
	b.serialize(cout);

}
