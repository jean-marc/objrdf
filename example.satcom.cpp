/*
 *	define a ~big database to test performance
 *
 *	~200 satellites
 *	~100 transponders per satellite (2e4 transponders)
 *	2 poles (vertical/horizontal linear/circular)
 *	~20 beams per satellite
 *	~100 carriers per transponder (100*100*200=2e6 carriers)
 *	~20 types of carriers
 *	~1000 earth stations
 *
 *
 */
#include "objrdf.h"
using namespace objrdf;

namespace sat{
	RDFS_NAMESPACE("http://www.example.org/satcom#","sat")
	PROPERTY(angle,short);//+-180 degree
	PROPERTY(cf,int);
	CLASS(Carrier,std::tuple<cf>);
	PROPERTY(carrier,Carrier*);
	//CLASS1(Transponder,array<carrier>);
	//CLASS(Transponder,std::tuple<array<carrier>>);
	char _Transponder[]="Transponder";
	class Transponder:public resource<
		rdfs_namespace,
		_Transponder,
		std::tuple<
			array<carrier>
		>,
		Transponder
	>{
	public:
		Transponder(uri u):SELF(u){}
		static Transponder create(uri u){
			Transponder t(u);
			int n=100;
			t.get<array<carrier>>().reserve(n);
			for(int i=0;i<n;++i){
				ostringstream os;
				os<<u.local<<"_"<<i;
				//need to clean up syntax
				t.get<array<carrier>>().push_back(carrier(carrier::PTR(carrier::PTR::pointer::construct(uri(os.str())))));	
				t.get<array<carrier>>().back()->set<cf>(cf(i));
			}
			return t;
		}

	};
	CLASS(Beam,std::tuple<>);
	PROPERTY(beam,Beam*);
	CLASS0(Polarization);
	PROPERTY(polarization,Polarization*);//we can use a set pointer for convenience
	PROPERTY(transponder,Transponder*);
	//what about
	//__PROPERTY(xpr,pseudo_ptr<Transponder,free_store<>>);
	char _Satellite[]="Satellite";
	class Satellite:public resource<
		rdfs_namespace,
		_Satellite,
		std::tuple<
			angle,
			array<beam>,
			array<transponder>
		>,
		Satellite
	>{
	public:
		typedef shared_ptr<Satellite>::pointer S_PTR;	
		Satellite(uri u):SELF(u){}
		static Satellite create(uri u){
			//typical satellite?
			int n=100;
			Satellite s(u);
			s.get<angle>()=angle(88);
			s.get<array<transponder>>().reserve(n);
			for(int i=0;i<n;++i){
				ostringstream os;
				os<<"xpr_"<<i;
				s.get<array<transponder>>().push_back(transponder(transponder::PTR(transponder::PTR::pointer::construct(Transponder::create(uri(os.str()))))));
			}
			return s;
		}
		
	};	
}
using namespace sat;
int main(int argc, char* argv[]){
	typedef transponder::PTR::pointer T_PTR;
	typedef carrier::PTR::pointer C_PTR;
	typedef shared_ptr<Satellite>::pointer S_PTR;	
	//needed to load the pools
	S_PTR::get_pool()->get_size();
	C_PTR::get_pool()->get_size();
	T_PTR::get_pool()->get_size();
	if(argc>1){
		int n=atoi(argv[1]);
		for(int i=0;i<n;++i){
			ostringstream os;
			os<<"sat_"<<i;
			S_PTR::construct(Satellite::create(uri(os.str())));
		}
	}
	to_rdf_xml(cout);
}
