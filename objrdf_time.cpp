#include "objrdf_time.h"
#include "introspect.h"
#include <iomanip>
#include <parser/parser.h>
#include <parser/char_iterator.h>
using namespace parser;
using namespace std;
using namespace objrdf;
#if 0
namespace objrdf{
	template<> PROPERTY_PTR introspect<ts>::get_property(){
		/*
		static PROPERTY_PTR c=rdf::Property::allocator_type::construct_allocate(
			objrdf::get_uri<objrdf::objrdf_rdfs_ns,NAME>(),
			rdfs::range(introspect<typename selector<RANGE>::ResultT>::get_class()),
			property<objrdf::objrdf_rdfs_ns,NAME,RANGE,BASE_PROPERTY>::TYPE&LITERAL
		);
		return c;
		*/
		return nullptr;
	}
}
#endif
ostream& objrdf::operator<<(ostream& os,const std::chrono::system_clock::time_point& _t){
	std::time_t t=std::chrono::system_clock::to_time_t(_t);
	char buffer[80];
	tm* timeinfo=localtime(&t);
	strftime (buffer,80,"%FT%T",timeinfo);	
	char tz_buffer[20];
	strftime (tz_buffer,20,"%z",timeinfo);	
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(_t.time_since_epoch());
	char ms_buffer[10];	
	snprintf(ms_buffer,10,".%03lli",(ms.count()%1000));
	return os<<buffer<<ms_buffer<<tz_buffer;
}
struct date_time_parser{
	typedef _rc<'0','9'> digit;
	typedef _pl<digit> digits;
	typedef _sq<digits,_or<_sq<_c<'.'>,digits>,_t>> sec;
	typedef _sq<digits,_c<'-'>,digits,_c<'-'>,digits,_c<'T'>,digits,_c<':'>,digits,_c<':'>,sec> _date_time;
	typedef _sq<_date_time,_t> date_time;
	template<typename ITERATOR> struct my_handler:parser::handler<
		ITERATOR,
		std::tuple<_date_time>
	>{
		std::chrono::system_clock::time_point t;
		void start(_date_time){}
		void stop(_date_time,ITERATOR begin,ITERATOR end,bool v){
			if(v){
				tm _t={};float s=0;
				/*auto r=*/sscanf(string(begin,end).c_str(),
					"%i-%i-%iT%i:%i:%f",
					&_t.tm_year,&_t.tm_mon,&_t.tm_mday,
					&_t.tm_hour,&_t.tm_min,&s);
				_t.tm_year-=1900;
				_t.tm_mon-=1;
				_t.tm_sec=floor(s);
				time_t tt=mktime(&_t);
				t=chrono::system_clock::from_time_t(tt);
				t+=chrono::milliseconds(static_cast<int>(round((s-floor(s))*1000)));
			}
		}
	};
};
istream& objrdf::operator>>(istream& is,std::chrono::system_clock::time_point& t){
	//let's fill string: cannot do that it's inside XML!!!!
	string ts;
	is>>ts;
	//getline(is,ts,'<');
	date_time_parser::my_handler<string::const_iterator> h;
	date_time_parser::date_time::go(ts.cbegin(),ts.cend(),h);
	/*
	cerr<<"parsing `"<<(char)is.peek()<<"'"<<endl;
	date_time_parser::my_handler<parser::char_iterator> h;
	date_time_parser::date_time::go(parser::char_iterator(is),parser::char_iterator(),h);
	cerr<<"end parsing `"<<(char)is.peek()<<"'"<<endl;
	*/
	t=h.t;
	//only supported in gcc 5.0, shall we build small parser?, still have to deal with ms
	//2016-02-17T22:27:33.508-0800
	//is>>get_time(&timeinfo,"%FT%T");	
	return is;
}
//https://www.w3.org/TR/xmlschema11-2/#duration
//PnYnMnDTnHnMnS
struct duration_parser{
	typedef _rc<'0','9'> digit;
	struct digits:_pl<digit>{};
	typedef _sq<digits,_c<'Y'>> year;
	typedef _sq<digits,_c<'M'>> month;
	typedef _sq<digits,_c<'D'>> day;
	typedef _sq<digits,_c<'H'>> hour;
	struct minute:_sq<digits,_c<'M'>>{};//because identical to month otherwise
	//typedef _sq<digits,_or<_sq<_c<'.'>,digits>,_t>,_c<'S'>> sec;
	typedef _sq<digits,_c<'S'>> sec;
	typedef _sq<
		_c<'P'>,_or<year,_t>,_or<month,_t>,_or<day,_t>,
		_or<
			_sq<_c<'T'>,_or<hour,_t>,_or<minute,_t>,_or<sec,_t>>,
			_t
		>> duration;	
	template<typename ITERATOR> struct my_handler:parser::handler<
		ITERATOR,
		std::tuple<
			digits,
			year,
			month,
			day,
			hour,
			minute,
			sec
		>
	>{
		std::chrono::milliseconds d{};
		int last_digit=0;
		void start(digits){}
		void stop(digits,ITERATOR begin,ITERATOR end,bool v){
			if(v){
				LOG_DEBUG<<"digits:`"<<string(begin,end)<<"'\n";
				last_digit=stoi(string(begin,end));
			}	
		}
		void start(year){}
		void stop(year,ITERATOR begin,ITERATOR end,bool v){
			constexpr auto _year = 31556952ll; // seconds in average Gregorian year
			if(v) d+=chrono::seconds(_year)*last_digit;
		}
		void start(month){}
		void stop(month,ITERATOR begin,ITERATOR end,bool v){
			constexpr auto _month = 31556952ll/12; // seconds in average Gregorian year
			if(v) d+=chrono::seconds(_month)*last_digit;
		}
		void start(day){}
		void stop(day,ITERATOR begin,ITERATOR end,bool v){
			if(v) d+=chrono::hours(24*last_digit);
		}
		void start(hour){}
		void stop(hour,ITERATOR begin,ITERATOR end,bool v){
			if(v) d+=chrono::hours(last_digit);
		}
		void start(minute){}
		void stop(minute,ITERATOR begin,ITERATOR end,bool v){
			if(v) d+=chrono::minutes(last_digit);
		}
		void start(sec){}
		void stop(sec,ITERATOR begin,ITERATOR end,bool v){
			if(v) d+=chrono::seconds(last_digit);
		}
	};
};
ostream& objrdf::operator<<(ostream& os,const std::chrono::milliseconds& d){
	
	return os<<d.count();
}
istream& objrdf::operator>>(istream& is,std::chrono::milliseconds& d){
	string ts;
	is>>ts;
	//getline(is,ts,'<');
	duration_parser::my_handler<string::const_iterator> h;
	auto r=duration_parser::duration::go(ts.cbegin(),ts.cend(),h);
	LOG_DEBUG<<"parsing duration:"<<r.first<<"\t"<<h.d<<"\n";
	d=h.d;	
	return is;
}

