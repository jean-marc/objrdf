/*
 *	schema to keep track of all items
 *	should be also defined in SQL for comparison
 *	there should be forms to help entering data: sparql+js or sparql+xslt (maybe better)
 *	we could also creates special handler in the server for each request:
 *		.new piece of inventory
 *		.update piece of inventory
 *		....
 *	it could also be combined with monitoring system but then needs to be made secure because will
 *	be on-line, we could tunnel through ssh
 *	need to be able to dump to RDF anytime for back-up and schema upgrade
 *	need to have generic schema defined in a library
 *
 *	prefix :<http://inventory.unicefuganda.org/#>
 *	select ?type ?model ?equipment ?set ?site ?manufacturer 
 *	where {	?equipment a :Equipment;:partOf ?set;a ?type;:model ?model .
 *		?set :located ?site . 
 *		?model :manufacturer ?manufacturer .
 *	} order by ?type
 *
 * 	query to get all information about modems at the Warchild sites
 *
  	prefix :<http://inventory.unicefuganda.org/#> 
  	select * 
  	where{	?modem a :Modem;:sim ?sim . 
  		?sim :number ?number .
  		?modem :partOf ?kiosk . 
  		?kiosk :located ?site . 
  		?site :organization <Warchild> .
  	}
 *	to send HTML formated email use:
 *	mutt -e "set content_type=text/html" address -s "subject" < test.html
 */
#include "objrdf.h"
#include "sparql_engine.h"
#include "httpd.h"
#include "geo.h"
#include "rdf_xml_parser.h"
#include <memory>
using namespace objrdf;

RDFS_NAMESPACE("http://inventory.unicefuganda.org/#","inv")

typedef persistent_store STORE;

/*
 *	new class Report: everytime we have a conversation with a site or a visit we should log a small report,
 *	properties would be author, time, content: plain text or html if we want to include references (to person, equipment,...)
 *	should it be attached to a Site or a Set?, Site makes more sense to allow reporting about a prospective site (no Set installed yet) 
 *
 */
typedef basic_string<
		char,
		std::char_traits<char>,
		custom_allocator<
			char,
			pseudo_ptr_array<char,STORE>/*,
			pseudo_ptr_array<const char,STORE>*/
		>
	> my_string;	
PROPERTY(text,my_string);
PROPERTY(time_stamp,time_t);
PROPERTY(time_stamp_v,objrdf::NIL);
//CLASS(Report,std::tuple<text>);
char _Report[]="Report";
class Report:public resource<rdfs_namespace,_Report,std::tuple<time_stamp,text>,Report>{
public:
	typedef std::tuple<time_stamp_v> PSEUDO_PROPERTIES;
	Report(uri id):SELF(id){
		get<time_stamp>().t=time(0);	
	}
	void out_p(time_stamp_v,ostream& os) const{
		os<<ctime(&get_const<time_stamp>().t);
	}

};
PROPERTY(report,Report::allocator::pointer);//what about const_pointer?

CLASS(Organization,std::tuple<>);
PROPERTY(organization,Organization::allocator::pointer);
DERIVED_CLASS(Site,geo::Point,std::tuple<organization,array<report>>);
PROPERTY(located,Site::allocator::pointer);
//maybe should use friend of a friend ontology for people
//people
PROPERTY(phone,long);
PROPERTY(site,Site::allocator::pointer);
CLASS(Person,std::tuple<array<phone,STORE>,array<site>>);//same person can be responsible for more than one site
PROPERTY(status,int);//needs to be developed: working/not working,...history
PROPERTY(_mac_,long);//MAC address 48 bytes
struct mac:_mac_{
	void in(istream& is){is>>hex>>t>>dec;}
	void out(ostream& os) const{os<<hex<<t<<dec;}
};
/*
 *	a server can send an update based on its own mac address and the one returned by arp
 *	
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 */
//would be nice to know where a piece of equipment has been, also to know if it has been serviced
//history is a linked-list (how does git work?)
CLASS(Manufacturer,std::tuple<>);
PROPERTY(manufacturer,Manufacturer::allocator::pointer);
CLASS(Model,std::tuple<manufacturer>);
PROPERTY(model,Model::allocator::pointer);
/*
 *	add properties and functions to track usage: how many hours it has been up, the VPN server
 *	will run a query when a client connects or disconnects, it will trigger a counter on the object.
 *	shall we store all the information in the db or use files? let's use the db, it will help create nice plots
 *
 */
/*
 *	logger: to keep track of the important data:
 *		on,
 *		uptime, will have to be adjusted if it is more than time since last entry
 *		number of clients on
 *		voltage
 *	should use blank nodes so the monitoring module does not have to come up with new id
 *	prefix :<http://inventory.unicefuganda.org/#> insert data {
 *		<test> :logger [
 *			:voltage 14.1;
 *			:n_client 3;
 *			:uptime 10
 *			] .}
 * 	
 * 	there are 2 ways to store the information:
 * 	1: Set -->-- Logger
 * 	<Set id='a'>
 * 	 <logger><Logger/></logger>
 * 	 <logger><Logger/></logger>
 * 	 ...
 * 	</Set>
 * 	instances have a list of predicate logger stored in a vector, the vector plays 2 roles: indicate which Logger belongs
 * 	to the Set and the sequence order, this can be accomplished in the second method by using indexes on `time' and `prop'
 * 	the problem with this method is that the object grows over time and becomes cumbersome to serialize
 * 	For instance if we want to track version it get expensive to stow away the object
 * 	2: Logger -->-- Set
 * 	<Set id='a'/>
 * 	<Logger time='0'><prop resource='#a'/></Logger>
 * 	<Logger time='1'><prop resource='#a'/></Logger>
 * 	...
 * 	we could use a special pool with no de-allocate so order matches time of instantiation
 *
 */
PROPERTY(uptime,time_t);//how long has the kiosk been up
PROPERTY(uptime_v,objrdf::NIL);
PROPERTY(n_client,uint16_t);
PROPERTY(volt,float);
//PROPERTY(out,unsigned int);//outgoing traffic in number of bytes
//PROPERTY(in,unsigned int);//incoming traffic in number of bytes
PROPERTY(data,unsigned int);//traffic in bytes
/*
 *	network usage monitoring and accounting, we need to be able to separate traffic by destination
 */
char _Logger[]="Logger";
/*
 *	this class will use the bulk of the storage space and will increase linearly with time 
 *	a new log is generated at constant 10 min interval, the size is 80 bytes (per obj:sizeOf) so 
 *	an operating site will add ~ 8*6*80=~3K of data per day
 *	there is a simple compression scheme implemented in Set::set_p(logger& p) that discards repeated
 *	or similar entries (battery voltage changes slowly).
 *	it should also report network usage for monitoring and accounting (billing)
 *
 */
class Logger:public resource<rdfs_namespace,_Logger,std::tuple<time_stamp,uptime,n_client,volt,data>,Logger>{
public:
	typedef std::tuple<time_stamp_v,uptime_v> PSEUDO_PROPERTIES;
	Logger(uri id):SELF(id){
		get<time_stamp>().t=time(0);	
	}
	void out_p(time_stamp_v,ostream& os) const{
		os<<ctime(&get_const<time_stamp>().t);
	}
	void out_p(uptime_v,ostream& os) const{
		time_t t=get_const<uptime>().t;
		int m=(t/60)%60;	
		int h=(t/3600)%24;	
		int d=t/(3600*24);	
		os<<d<<"d "<<h<<"h "<<m<<"m";
	}
};
char _Set[]="Set";
PROPERTY(on,bool);
PROPERTY(tot_uptime,time_t);//how long has the kiosk been up
PROPERTY(logger,Logger::allocator::pointer);
PROPERTY(plot,objrdf::NIL);
PROPERTY(svg_plot,rdfs::XMLLiteral);//we can choose to make it real property or pseudo
/*
 *	data package management:
 *		up to 4 Gb
 *		how much data used so far and how much available,
 *		we must have used_data < available_data < reserved_data
 */
PROPERTY(used_data,unsigned int);
PROPERTY(available_data,unsigned int);
PROPERTY(reserved_data,unsigned int);
/*
 *	how do we track changes: eg new location
 *
 */
class Set:public resource<
	rdfs_namespace,
	_Set,
	std::tuple<
		located,
		on,
		/*uptime*/
		array<logger>
		/*,objrdf::prev*/
		,used_data
		,available_data
		,reserved_data
	>,
	Set/*very important!*/>{
	time_t start;
public:
	typedef array<logger> loggers;
	typedef std::tuple<plot/*,svg_plot*/> PSEUDO_PROPERTIES;
	//typedef on TRIGGER;
	//typedef located TRIGGER;
	typedef array<logger> TRIGGER;
	//typedef located VERSION;
	Set(uri id):SELF(id),start(time(0)){cerr<<"new Set()"<<endl;}
	static bool comp(const logger& a,time_t b){return a->get_const<time_stamp>().t<b;}
	static bool comp_data(const logger& a,const logger& b){return a->get_const<data>().t<b->get_const<data>().t;}
	void out_p(plot,ostream& os) const{
	/*
 	*	display of uptime, power, bandwidth,....
 	*	get the time at midnight or display the last 24 hours, or the last 24 hours recorded
 	*	it might make sense to cache it, but the function is declared const
 	*/ 
		if(get_const<array<logger>>().size()==0) return;
		int width=200,label_width=20,height=100;//labels on both side
		//should be made a parameter
		time_t duration=10*24*3600;//1 day
		float scale_x=static_cast<float>(width)/duration;
		//need to distinguish 12V/24V
		//we could query db or guess from voltage
		float min_v=0,max_v=0,dscn_v=0;
		//problem: if measured failed (-1) it assumes it is 12V system so we look for good measurement in first 10
		auto k=get_const<loggers>().cbegin();
		while(k!=get_const<loggers>().cbegin()+min<int>(10,get_const<loggers>().size())&&(*k)->get_const<volt>().t==-1) ++k;
		//if(get_const<array<logger>>().back()->get_const<volt>().t>20){
		if((*k)->get_const<volt>().t>20){
			min_v=20;
			max_v=30;
			dscn_v=24.5;
		}else{
			min_v=10;
			max_v=20;
			dscn_v=12.25;
		}
		float range_v=max_v-min_v;
		float scale_y=static_cast<float>(height)/range_v;
		//careful : could be empty!
		//time_t stop=get_const<array<logger>>().back()->get_const<time_stamp>().t;	
		time_t stop=time(0);//now
		time_t start=stop-duration; 
		//find the index in get<array>
		auto j=lower_bound(get_const<loggers>().cbegin(),get_const<loggers>().cend(),start,Set::comp);
		os<<"<svg width='"<<label_width+width+label_width<<"' height='"<<height<<"' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>";
		//data
		//find maximum data in this range
		auto max_data=max_element(j,get_const<loggers>().cend(),Set::comp_data);
		//next range multiple of 10: 1K, 10K, 100K, 1G,...
		auto max_data_10=pow(10,ceil(log10((*max_data)->get_const<data>().t)));
		data::RANGE min_d=0,max_d=max_data_10;
		data::RANGE range_d=max_d-min_d;
		float scale_d=static_cast<float>(height)/range_d;
		//how to draw a line at midnight?
		time_t rawtime;
		time ( &rawtime );
		struct tm * timeinfo;
		timeinfo = localtime ( &rawtime );
		timeinfo->tm_sec=0;
		timeinfo->tm_min=0;
		timeinfo->tm_hour=0;
		time_t mn=mktime(timeinfo);
		{
			//labels, needs to be cleaned up!!!
			os<<"<g class='left_labels' transform='translate(0,0)'>";
			float v=min_v+9*range_v/10;
			for(int i=height/10;i<height;i+=height/10,v-=range_v/10)
				os<<"<text class='label' y='"<<i<<"'>"<<v<<"V</text>"<<endl;
			os<<"</g>"<<endl;
			os<<"<g class='right_labels' transform='translate("<<label_width+width<<",0)'>";
			data::RANGE d=min_d+9*(range_d/10);//!!!int
			for(int i=height/10;i<height;i+=height/10,d-=range_d/10)
				os<<"<text class='label' y='"<<i<<"'>"<<(d/1000000)<<"Mb</text><!-- "<<d<<"-->"<<endl;
			os<<"</g>"<<endl;
			//change the reference but messes up text display (mirror image)
			os<<"<g transform='scale(1,-1) translate("<<label_width<<","<<-height<<")'>"<<endl;;
			for(;mn>start;mn-=24*3600){
				int wday=localtime(&mn)->tm_wday;
				//buggy here...
				if(wday==0||wday==6) 
					os<<"<rect class='we' x='"<<((mn-start)*scale_x)<<"' y='0' width='"<<(24*3600*scale_x)<<"' height='"<<height<<"'/>"<<endl;
				os<<"<path class='vgrid' d='M"<<((mn-start)*scale_x)<<" 0v"<<height<<"'/>"<<endl;
			}
			//we must use the same grid for all curves: voltage (current), data,... 
			//for(float v=min_v+1.0;v<max_v;v+=1.0)
			//	os<<"<path class='hgrid' d='M0 "<<((v-min_v)*scale_y)<<"h"<<width<<"'/>"<<endl;
			for(int i=height/10;i<height;i+=height/10)
				os<<"<path class='hgrid' d='M0 "<<i<<"h"<<width<<"'/>"<<endl;
			//problem: mirror image because of scale(1,-1) in parent <g/>
			/*
			os<<"<g class='data_labels'>";
			for(int i=height/10;i<height;i+=height/10)
				//os<<"<text class='label' y='"<<i<<"'>"<<i<<"</text>"<<endl;
				os<<"<text transform='scale(1,-1) translate(0,"<<i<<")' class='label'>"<<i<<"</text>"<<endl;
			os<<"</g>";
			*/
			os<<"<path class='dscn' d='M0 "<<((dscn_v-min_v)*scale_y)<<"h"<<width<<"'/>"<<endl;
			{
				auto i=j;
				int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
				int y=((*i)->get_const<volt>().t-min_v)*scale_y;
				os<<"<path class='trace' d='M"<<x<<" "<<y<<"L";
				++i;
				for(;i<get_const<array<logger>>().cend();++i){
					if((*i)->get_const<volt>().t!=-1){
						int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
						int y=((*i)->get_const<volt>().t-min_v)*scale_y;
						os<<x<<" "<<y<<" ";
					}
				}	
				os<<"'/>"<<endl;
			}
			//data usage plot
			{
				auto i=j;
				int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
				int y=((*i)->get_const<data>().t-min_d)*scale_d;
				os<<"<path class='trace_data' d='M"<<x<<" "<<y<<"L";
				++i;
				for(;i<get_const<array<logger>>().cend();++i){
					int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
					int y=((*i)->get_const<data>().t-min_d)*scale_d;
					os<<x<<" "<<y<<" ";
				}	
				os<<"'/>"<<endl;
			}
			//os<<"</g>"<<endl;
			//is the machine on? how many clients connected?, let's draw rectangles
			time_t prev=start;
			for(auto i=j;i<get_const<array<logger>>().cend();++i){
				time_t current=(*i)->get_const<time_stamp>().t;
				float x=(current-start)*scale_x;
				float w=min(current-prev,(*i)->get_const<uptime>().t)*scale_x;
				int h=0.1*height*(1+(*i)->get_const<n_client>().t);
				//room for optimization: 1 big rectangle instead of many (assuming same number of clients)
				os<<"<rect class='uptime' x='"<<x-w<<"' y='0' width='"<<w<<"' height='"<<h<<"'/>"<<endl;		
				prev=current;
			}	
		}
		os<<"<rect class='frame' x='0' y='0' width='"<<width<<"' height='"<<height<<"'/>";
		os<<"</g>";
		os<<"</svg>";
	}
	/*
 	* we can add some form of compression, if the new Logger is not very different from previous one it simply overrides is
 	* we have to be careful, by now the property has already been appended, 
 	* space has been allocated for the objects wether they are used or not
 	*/
	void set_p(logger& p){
		get<loggers>().back()=p;
		/*
 		*	let us increment the used_data counter, has the counter be reset?
 		*	problem: the blank object has not been parsed yet!!!!!!!
 		*
 		*/
		if(get_const<loggers>().size()==1){
			get<used_data>().t+=p->get<data>().t;
		}else{
			auto previous=get_const<loggers>()[get_const<loggers>().size()-2];
			cerr<<"current:"<<endl;
			objrdf::to_rdf_xml(p,cerr);	
			if(previous->get_const<time_stamp>().t < (p->get_const<time_stamp>().t-p->get_const<uptime>().t))
				get<used_data>().t+=p->get<data>().t;
			else
				get<used_data>().t+=p->get<data>().t-previous->get<data>().t;
		}
		/*
		cerr<<"trigger!"<<endl;
		get<loggers>().pop_back(); //ugly
		if(get_const<array<logger>>().size()==0){
			get<array<logger>>().push_back(p);
		}else{
			if(((p->get<time_stamp>().t-get<loggers>().back()->get<time_stamp>().t)<p->get<uptime>().t)&&(p->get<n_client>().t==get<loggers>().back()->get<n_client>().t)&&(fabs(p->get<volt>().t-get<loggers>().back()->get<volt>().t)<0.1)){
				cerr<<"discarding old result!"<<endl;
				//should free memory here
				//not supported yet in g++ 4.4.4
				//allocator_traits<Logger::allocator>::deallocate(p,1);
				//something buggy here!!!
				//Logger::allocator a;
				//a.destroy(get<loggers>().back());
				//a.deallocate(get<loggers>().back(),1);
				get<loggers>().back()=p;
			}else
				get<array<logger>>().push_back(p);
		} 
		*/
	}
	void set_p(on& p){
		/*if(p.t)
			start=time(0);
		else
			get<uptime>().t+=time(0)-start;
		*/
	//	get<on>()=p;
	}
	void set_p(const located& p){
		//get<located>()=p;
	}
};


PROPERTY(partOf,pseudo_ptr<Set,Set::STORE,true>);
/*
 *	we could have manufacturer & model but it could become inconsistent, so only model is used
 */
CLASS(Equipment,std::tuple<partOf,model,status>);
//a digital doorway or a drum could be considered a Set
//could also see a set as a Bag of equipment, using RDF semantic
//would be nice to have URI for those
DERIVED_CLASS(DDoorway,Set,std::tuple<>);
DERIVED_CLASS(DDrum,Set,std::tuple<>);
DERIVED_CLASS(Uniport,Set,std::tuple<>);
DERIVED_CLASS(Kiosk,Set,std::tuple<>);

//equipment
/*what kind of ID shall we use? 
*
* MAC address? nice because easily accessible 
* 	for instance a server can go through all the clients attached and post a sparql query, without knowing the actual ID
* 	we can get the MAC address from the BIOS menu, which will have to be modified for each new machine anyway
* 	INSERT {?x :status :on .} WHERE {?x :mac '00:11:22:33:44:55' .}
* 	we can also have some kind of `auto-discovery'
* 	INSERT DATA {<00:11:22:33:44:55> :a :Laptop . <SET_ID> :part <00:11:22:33:44:55> .}
* 	needs to be attached to the SET because equipment can be replaced
* 	MAC could be stored as an int in memory for fast look-up
*
* pseudo_ptr? nice because very fast access, we can attach that new ID to the piece of equipment (bar code/QR code,..)
*
* serial number? useful when dealing with manufacturer
*/
CLASS(Version,std::tuple<>);
//PROPERTY(version,pseudo_ptr<base_resource>);//most general but problem with persistence
PROPERTY(version,pseudo_ptr<Version>);
DERIVED_CLASS(Drive,Equipment,std::tuple<version>);//information about OS/software version
//could define versions with some explanations and link to them
//eg ubuntu-desktop-11.10-amd64
DERIVED_CLASS(Laptop,Equipment,std::tuple<mac>);//problem: won't show up in schema, we can work-around in stylesheet
DERIVED_CLASS(Server,Equipment,std::tuple<mac>);
DERIVED_CLASS(UPS,Equipment,std::tuple<>);
/*
 *	is the modem attached to a service provider, if not it means it is unlocked
 */
CLASS(Telco,std::tuple<>); //Orange, MTN, Airtel, Warid
PROPERTY(telco,Telco::allocator::pointer);
PROPERTY(number,long);//otherwise conflict with phone
CLASS(Sim,std::tuple<number,telco>);
PROPERTY(sim,pseudo_ptr<Sim>);//because we will start moving sim cards around
DERIVED_CLASS(Modem,Equipment,std::tuple<sim,telco>);
PROPERTY(power,int);
PROPERTY(voltage,int);
DERIVED_CLASS(PV_panel,Equipment,std::tuple<power,voltage>);
DERIVED_CLASS(Battery,Equipment,std::tuple<>);
DERIVED_CLASS(Charge_Controller,Equipment,std::tuple<>);
DERIVED_CLASS(Relay_Driver,Equipment,std::tuple<>);
DERIVED_CLASS(Inverter,Equipment,std::tuple<>);
int main(int argc,char* argv[]){
	Site::get_class();
	Person::get_class();
	Set::get_class();
	DDoorway::get_class();
	DDrum::get_class();
	Kiosk::get_class();
	Uniport::get_class();
	Equipment::get_class();
	Drive::get_class();
	Laptop::get_class();
	Server::get_class();
	UPS::get_class();
	Modem::get_class();	
	PV_panel::get_class();
	Battery::get_class();
	Charge_Controller::get_class();
	Relay_Driver::get_class();
	Inverter::get_class();
	Organization::get_class();
	Model::get_class();	
	Report::get_class();
	/*
	//how do I get access to the pool?
	//auto i=POOL_PTR::help<Set>();
	//pseudo_ptr<Set> p(i->allocate());
	//p.construct(Set(uri("jm")));	
	auto p=pseudo_ptr<Set>::construct(uri("jm"));
	cerr<<"start test..."<<endl;
	cerr<<sizeof(Logger)<<" bytes"<<endl;
	for(;;){
		//auto i=POOL_PTR::help<Logger>();
		pseudo_ptr<Logger> l(pseudo_ptr<Logger>::get_pool()->allocate());//broken!!!!
		//auto l=pseudo_ptr<Logger>::allocate();
		ostringstream os;
		l._print(os);
		uri u(os.str());
		u.index=1;
		new(l) Logger(u);
		//l.construct(u);	
		p->get<array<logger>>().push_back(logger(l));
	}
	//RESOURCE_PTR p=create_by_type(Set::get_class(),uri("test"));
	*/
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		rdf_xml_parser r(in);
		cerr<<"parsing file `"<<argv[1]<<"'"<<endl;
		r.go();
	}
	to_rdf_xml(cout);
	//start web server
	objrdf::httpd h;
	h.run();
	/*
	//h.start();
	cin.exceptions(iostream::eofbit);
	sparql_parser sp(std::cin);
	bool r=sp.go();
	if(r){
		cerr<<"parsing success"<<endl;
		sp.out(cout);
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
	*/
};
