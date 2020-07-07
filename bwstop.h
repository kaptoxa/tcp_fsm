#pragma once

#include <sys/timeb.h>
#include <time.h>

#ifdef _WIN32
#define timeb _timeb
#define ftime _ftime
#endif


namespace tcp_fsm
{

const time_t valid_period = 1; //  the validity period of statistic records

struct tcp_conn_struct;
typedef tcp_conn_struct* tcp_conn_t;

//typedef std::map<time_t,uint32_t> bw_records_t;
struct bw_struct_t
{
	timeb t;
	uint32_t size;
};
typedef std::vector<bw_struct_t> bw_records_t;
struct bw_state
{
	bw_records_t* records;
	bool active;
	uint32_t cur_bw;
	uint32_t guess;				// number of bytes to send next time
};
typedef std::map<tcp_conn_t,bw_state> bw_states_t;

class bwstop
{
	bw_states_t states;

	uint32_t max_bw;			//bytes per second
	uint32_t cur_bw;

	tcp_conn_t next_conn;		// the next conn to send

	bw_records_t* find(tcp_conn_t);

	timeb get_time();

	bool emulator;
	timeb vtime; // virtual time;
	void turn(uint32_t msecs);


public:
	bwstop(uint32_t limit=0);
	~bwstop();

	void set_limit(uint32_t);
	bool is_enable(){return max_bw!=0;}

	void on(tcp_conn_t);
	void off(tcp_conn_t);
	void insert(tcp_conn_t,uint32_t);
	bool recalc();
	bool can(tcp_conn_t,uint32_t*);
	uint32_t guess(tcp_conn_t);
	void erase(tcp_conn_t);

	inline bool excess(){return max_bw?cur_bw>max_bw:false;}
};

}

