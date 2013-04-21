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
 *	prefix :<http://monitor.unicefuganda.org/#>
 *	select ?type ?model ?equipment ?set ?site ?manufacturer 
 *	where {	?equipment a :Equipment;:partOf ?set;a ?type;:model ?model .
 *		?set :located ?site . 
 *		?model :manufacturer ?manufacturer .
 *	} order by ?type
 *
 * 	query to get all information about modems at the Warchild sites
 *
  	prefix :<http://monitor.unicefuganda.org/#> 
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
//typedef int BASE_CLASS; does not work!
namespace monitor{
	RDFS_NAMESPACE("http://monitor.unicefuganda.org/#","mon")//modify the prefix seems to break executable
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
	//how much work to support xhtml?
	PROPERTY(text,my_string);
	PROPERTY(time_stamp,time_t);
	//should move that out of monitor namespace
	PSEUDO_PROPERTY(date_stamp_v,xsd::date);//[-]CCYY-MM-DD]
	PSEUDO_PROPERTY(time_stamp_v,xsd::dateTime);//[-]CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm]
	//create generic class to time-stamp resources
	char _Timed[]="Timed";
	class Timed:public resource<
		rdfs_namespace,
		_Timed,
		std::tuple<time_stamp>,
		Timed
	>{
	public:
		//it could be up to sub-class to decide which pseudo_property to implement, but does not work
		typedef std::tuple<time_stamp_v> PSEUDO_PROPERTIES;
		Timed(uri id):SELF(id){
			get<time_stamp>().t=time(0);	
		}
		Timed(const Timed& t):SELF(t){
			//time_stamp is the construction date
			get<time_stamp>().t=time(0);	
		}
		Timed& operator=(const Timed& t){
			return *this;
		}
		void in_p(time_stamp_v,istream& is){
			time_t rawtime;
			time (&rawtime);
			tm *timeinfo=localtime(&rawtime);
			char c;//pretty forgiving parser
			is>>timeinfo->tm_year>>c>>timeinfo->tm_mon>>c>>timeinfo->tm_mday>>c>>timeinfo->tm_hour>>c>>timeinfo->tm_min>>c>>timeinfo->tm_sec;	
			timeinfo->tm_year-=1900;
			timeinfo->tm_mon-=1;
			if(!is.fail()) get<time_stamp>().t=mktime(timeinfo);
		}
		void out_p(time_stamp_v,ostream& os) const{
			char buffer[80];
			tm* timeinfo=localtime(&cget<time_stamp>().t);
			strftime (buffer,80,"%FT%T",timeinfo);	
			os<<buffer;
		}
		void in_p(date_stamp_v,istream& is){
			cerr<<"ignoring date!"<<endl;
		}
		void out_p(date_stamp_v,ostream& os) const{
			os<<"date:"<<ctime(&cget<time_stamp>().t);
		}
	};
	DERIVED_PERSISTENT_CLASS(Report,Timed,std::tuple<text>);
	PROPERTY(report,Report::allocator::pointer);//what about const_pointer?

	PROPERTY(name,my_string);
	PERSISTENT_CLASS(Organization,std::tuple<>);
	PROPERTY(organization,Organization::allocator::pointer);
	//Site could be a sub class of Timed
	DERIVED_PERSISTENT_CLASS(Site,geo::Point,std::tuple<name,organization,array<report>>);
	PROPERTY(located,Site::allocator::pointer);
	//maybe should use friend of a friend ontology for people
	//people
	PROPERTY(phone,long);
	PROPERTY(site,Site::allocator::pointer);
	PROPERTY(first_name,my_string);
	PROPERTY(last_name,my_string);
	PROPERTY(email,my_string);//could define a type instead
	//store name in ID, not great, should move to blank nodes, 
	PERSISTENT_CLASS(Person,std::tuple<first_name,last_name,email,array<phone,STORE>,array<site>>);//same person can be responsible for more than one site
	PROPERTY(status,int);//needs to be developed: working/not working,...history
	PROPERTY(_mac_,long);//MAC address 48 bytes, there might be multiple adapters, let's pick the smallest mac
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
	PERSISTENT_CLASS(Manufacturer,std::tuple<>);
	PROPERTY(manufacturer,Manufacturer::allocator::pointer);
	PERSISTENT_CLASS(Model,std::tuple<manufacturer>);
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
	 *	prefix :<http://monitor.unicefuganda.org/#> insert data {
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
	class Logger:public resource<
		rdfs_namespace,
		_Logger,
		std::tuple<uptime,n_client,volt,data>,
		Logger,
		Timed,
		pool_allocator<
			Logger,
			persistent_store,
			uint16_t
		>
	>{
	public:
		typedef std::tuple<uptime_v> PSEUDO_PROPERTIES;
		Logger(uri id):SELF(id){}
		static void do_index(RESOURCE_PTR p){}//so instances do not get indexed
		void out_p(uptime_v,ostream& os) const{
			time_t t=cget<uptime>().t;
			int m=(t/60)%60;	
			int h=(t/3600)%24;	
			int d=t/(3600*24);	
			os<<d<<"d "<<h<<"h "<<m<<"m";
		}
	};
	PROPERTY(on,bool);
	PROPERTY(tot_uptime,time_t);//how long has the kiosk been up
	PROPERTY(logger,Logger::allocator::pointer);
	PSEUDO_PROPERTY(plot,rdfs::XML_Literal);
	//PROPERTY(plot,objrdf::NIL);
	//PROPERTY(svg_plot,rdfs::XMLLiteral);//we can choose to make it real property or pseudo
	/*
	 *	data package management:
	 *		up to 4 Gb
	 *		how much data used so far and how much available,
	 *		we must have used_data < available_data < reserved_data
	 */
	PROPERTY(used_data,unsigned int);
	PROPERTY(available_data,unsigned int);
	PROPERTY(reserved_data,unsigned int);
	PROPERTY(ip_add,my_string);//would be better as unsigned int but then parsing needed
	/*
	 *	how do we track changes: eg new location
	 *
	 */
	char _Set[]="Set";
	class Set:public resource<
		rdfs_namespace,
		_Set,
		std::tuple<
			located,
			ip_add,
			on,
			/*uptime*/
			array<logger>,
			used_data,
			available_data,
			reserved_data
		>,
		Set,/*very important!*/
		//would it make sense to derive Set from Equipment? 
		//base_resource,
		Timed,
		pool_allocator<
			Set,
			persistent_store,
			uint16_t//could be made smaller, we wont have more than 256 kiosks?
		>
	>{
		time_t start;
	public:
		typedef array<logger> loggers;
		typedef std::tuple<plot/*,time_stamp_v*/> PSEUDO_PROPERTIES;
		//typedef on TRIGGER;
		//typedef located TRIGGER;
		typedef loggers TRIGGER;
		//typedef located VERSION;
		Set(uri id):SELF(id),start(time(0)){}
		static bool comp(const logger& a,time_t b){return a->cget<time_stamp>().t<b;}
		static bool comp_data(const logger& a,const logger& b){return a->cget<data>().t<b->cget<data>().t;}
		void in_p(plot,istream& is){}
		void out_p(plot,ostream& os) const{
		/*
		*	display of uptime, power, bandwidth,....
		*	get the time at midnight or display the last 24 hours, or the last 24 hours recorded
		*	it might make sense to cache it, but the function is declared const
		*/ 
			if(cget<loggers>().size()==0) return;
			int width=300,label_width=20,height=150;//labels on both side
			//should be made a parameter
			time_t duration=10*24*3600;//10 day
			float scale_x=static_cast<float>(width)/duration;
			//time_t stop=cget<loggers>().back()->cget<time_stamp>().t;	
			time_t stop=time(0)+3600;//in 1 hour so plot is not hidden by frame
			time_t start=stop-duration; 
			//find the index in get<array>
			auto j=lower_bound(cget<loggers>().cbegin(),cget<loggers>().cend(),start,Set::comp);
			os<<"<svg width='"<<label_width+width+label_width<<"' height='"<<height+20<<"' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>";
			os<<"<g transform='translate(0,10)'>";
			//voltage
			//need to distinguish 12V/24V
			//we could query db or guess from voltage
			float min_v=0,max_v=0,dscn_v=0;
			auto k=j;
			while(k!=cget<loggers>().cend()&&(*k)->cget<volt>().t==-1) ++k;
			if((*k)->cget<volt>().t>20){
				min_v=24;
				max_v=29;
				dscn_v=24.5;
			}else{
				min_v=10;
				max_v=15;
				dscn_v=12.25;
			}
			float range_v=max_v-min_v;
			float scale_y=static_cast<float>(height)/range_v;

			//data
			//find maximum data in this range
			auto max_data=max_element(j,cget<loggers>().cend(),Set::comp_data);
			//next range multiple of 10: 1K, 10K, 100K, 1G,...
			auto max_data_10=pow(10,ceil(log10((*max_data)->cget<data>().t)));
			if((*max_data)->cget<data>().t< max_data_10/2)
				max_data_10=max_data_10/2;
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
				os<<"<g class='left_labels' transform='translate("<<label_width<<",0)'>";
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
				os<<"<rect class='frame' x='0' y='0' width='"<<width<<"' height='"<<height<<"'/>";
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
				//is the machine on? how many clients connected?, let's draw rectangles
				//backwards
				auto first=*j;
				for(auto i=j;i<cget<loggers>().cend()-1;++i){
					auto current=*i,next=*(i+1);
					if((next->cget<time_stamp>().t-first->cget<time_stamp>().t)>next->cget<uptime>().t || next->cget<n_client>().t!=first->cget<n_client>().t || i==cget<loggers>().cend()-2){
						float x=(current->cget<time_stamp>().t-start)*scale_x;
						float w=(current->cget<time_stamp>().t-first->cget<time_stamp>().t)*scale_x;
						int h=0.1*height*(1+current->cget<n_client>().t);//maximum 9 clients
						os<<"<rect class='uptime' x='"<<x-w<<"' y='0' width='"<<w<<"' height='"<<h<<"'/>"<<endl;		
						first=next;
					}
				}	
				/*
				*	improvement: avoid long interpolation during down-time 
				*
				*/ 
				os<<"<path class='dscn' d='M0 "<<((dscn_v-min_v)*scale_y)<<"h"<<width<<"'/>"<<endl; //disconnect voltage
				{
					auto i=j;
					int x=((*i)->cget<time_stamp>().t-start)*scale_x;
					int y=((*i)->cget<volt>().t-min_v)*scale_y;
					os<<"<path class='trace' d='M"<<x<<" "<<y<<"L";
					++i;
					time_t prev=start;
					for(;i<cget<loggers>().cend();++i){
						if((*i)->cget<volt>().t!=-1){
							time_t current=(*i)->cget<time_stamp>().t;
							if((current-prev)>(*i)->cget<uptime>().t) os<<x<<" "<<y<<"M";
							/*int*/ x=((*i)->cget<time_stamp>().t-start)*scale_x;
							/*int*/ y=((*i)->cget<volt>().t-min_v)*scale_y;
							os<<x<<" "<<y<<" ";
							if((current-prev)>(*i)->cget<uptime>().t) os<<"L";
							prev=current;
						}
					}	
					os<<"'/>"<<endl;
				}
				//data usage plot
				{
					auto i=j;
					int x=((*i)->cget<time_stamp>().t-start)*scale_x;
					int y=((*i)->cget<data>().t-min_d)*scale_d;
					os<<"<path class='trace_data' d='M"<<x<<" "<<y<<"L";
					++i;
					time_t prev=start;
					for(;i<cget<loggers>().cend();++i){
						time_t current=(*i)->cget<time_stamp>().t;
						if((current-prev)>(*i)->cget<uptime>().t) os<<x<<" "<<y<<"M";
						/*int*/ x=((*i)->cget<time_stamp>().t-start)*scale_x;
						/*int*/ y=((*i)->cget<data>().t-min_d)*scale_d;
						os<<x<<" "<<y<<" ";
						if((current-prev)>(*i)->cget<uptime>().t) os<<"L";
						prev=current;
					}	
					os<<"'/>"<<endl;
				}
			}
			//os<<"<rect class='frame' x='0' y='0' width='"<<width<<"' height='"<<height<<"'/>";
			os<<"</g>";
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
			if(cget<loggers>().size()==1){
				get<used_data>().t+=p->get<data>().t;
			}else{
				auto previous=cget<loggers>()[cget<loggers>().size()-2];
				cerr<<"current:"<<endl;
				objrdf::to_rdf_xml(p,cerr);	
				if(previous->cget<time_stamp>().t < (p->cget<time_stamp>().t-p->cget<uptime>().t))
					get<used_data>().t+=p->get<data>().t;
				else
					get<used_data>().t+=p->get<data>().t-previous->get<data>().t;
			}
			/*
			cerr<<"trigger!"<<endl;
			get<loggers>().pop_back(); //ugly
			if(cget<loggers>().size()==0){
				get<loggers>().push_back(p);
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
					get<loggers>().push_back(p);
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

	/*
	 *	ensures that a piece of equipment is only part of one Set
	 */
	PROPERTY(partOf,Set::allocator::derived_pointer);
	/*
	 *	we could have manufacturer & model but it could become inconsistent, so only model is used
	 *	add some time information: when was the resource created, also used for versioning
	 *	we want to keep version on changes to partOf and maybe status
	 */
	//PERSISTENT_CLASS(Equipment,std::tuple<partOf,model,status,objrdf::next>);
	char _Equipment[]="Equipment";
	class Equipment:public objrdf::resource<
		rdfs_namespace,
		_Equipment,
		std::tuple<
			partOf,
			model,
			status,
			objrdf::prev,//version control
			objrdf::next//version control
		>,
		Equipment,
		Timed,
		pool_allocator<
			Equipment,
			persistent_store,
			uint16_t
		>
	>{
	public:
		typedef partOf VERSION;//then objrdf::prev and objrdf::next must be present
		Equipment(uri u):SELF(u){}
	};		
	/* 
 	* example of query to track inventory movement:
 	
 		prefix :<http://monitor.unicefuganda.org/#> 
 		select ?x ?from ?to ?date where {
 			?x obj:prev ?y . 
 			?x :partOf ?_to . 
 			?_to :located ?to . 
 			?y :partOf ?_from . 
 			?_from :located ?from .
 			?x :time_stamp_v ?date .
		}order by ?from 
	*/
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
	PERSISTENT_CLASS(Version,std::tuple<>);
	//PROPERTY(version,pseudo_ptr<base_resource>);//most general but problem with persistence
	PROPERTY(version,pseudo_ptr<Version>);
	//can we get rid of Drive? they will usually stay attached to computer
	DERIVED_CLASS(Drive,Equipment,std::tuple<version>);//information about OS/software version
	//could define versions with some explanations and link to them
	//eg ubuntu-desktop-11.10-amd64
	DERIVED_CLASS(Computer,Equipment,std::tuple<mac>);
	DERIVED_CLASS(Laptop,Computer,std::tuple<>);
	DERIVED_CLASS(Server,Computer,std::tuple<>);
	DERIVED_CLASS(UPS,Equipment,std::tuple<>);
	/*
	 *	is the modem attached to a service provider, if not it means it is unlocked
	 */
	PERSISTENT_CLASS(Telco,std::tuple<>); //Orange, MTN, Airtel, Warid, UTL
	PROPERTY(telco,Telco::allocator::pointer);
	PROPERTY(number,long);//otherwise conflict with phone
	PERSISTENT_CLASS(Sim,std::tuple<number,telco>);
	PROPERTY(sim,Sim::allocator::pointer);//because we will start moving sim cards around
	DERIVED_CLASS(Modem,Equipment,std::tuple<sim,telco>);
	PROPERTY(power,int);
	PROPERTY(voltage,int);
	DERIVED_CLASS(PV_panel,Equipment,std::tuple<power,voltage>);
	DERIVED_CLASS(Battery,Equipment,std::tuple<>);
	DERIVED_CLASS(Charge_Controller,Equipment,std::tuple<>);
	DERIVED_CLASS(Relay_Driver,Equipment,std::tuple<>);
	DERIVED_CLASS(Inverter,Equipment,std::tuple<>);
}
using namespace monitor;
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
