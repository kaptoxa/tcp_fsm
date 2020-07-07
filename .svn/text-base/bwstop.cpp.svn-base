#include "stdafx.h"
#include "bwstop.h"

namespace tcp_fsm
{

bwstop::bwstop(uint32_t limit):
	max_bw(limit)
{
	emulator = false;
}

bwstop::~bwstop()
{
}

void bwstop::set_limit(uint32_t nbytes)
{
	max_bw = nbytes;
}

bw_records_t* bwstop::find(tcp_conn_t conn)
{
	bw_states_t::iterator it = states.find(conn);
	if( it==states.end() )
	{
		bw_state state;
		state.active = true;
		state.cur_bw = 0;
		state.records = new bw_records_t();
		states.insert(std::make_pair(conn,state));
		return state.records;
	}
	return it->second.records;
}

void bwstop::on(tcp_conn_t conn)
{
	if( !max_bw )
		return;

	bw_states_t::iterator it = states.find(conn);
	if( it!=states.end() )
		it->second.active = true;
}

void bwstop::off(tcp_conn_t conn)
{
	if( !max_bw )
		return;

	bw_states_t::iterator it = states.find(conn);
	if( it!=states.end() )
		it->second.active = false;
}

void bwstop::insert(tcp_conn_t conn, uint32_t size)
{
	if( !max_bw )
		return;
	bw_records_t* records = find(conn);

	bw_struct_t record;
	ftime(&record.t);
	record.size = size;

	//printf("[%d:", record.t.time );
	//printf(":%d] insert to ", record.t.millitm );
	//printf("%p - ", conn);
	//printf("%d\n", size);

	records->push_back(record);
}

void bwstop::erase(tcp_conn_t conn)
{
	bw_states_t::iterator it = states.find(conn);
	if( it!=states.end())
		states.erase(it);
}

bool bwstop::recalc()
{
	if( !max_bw )
		return false;

	timeb since;
	ftime(&since);
	since.time = since.time - valid_period;

	uint32_t min_bw = max_bw; // let's find the connection which has the minimal bandwidth
	uint32_t min_count = 0;

	cur_bw = 0;
	bw_records_t::iterator zed;
	bool unused = false;
	for(bw_states_t::iterator sit = states.begin(),end = states.end();sit!=end;sit++)
	{
		bw_state *state = &sit->second;
		state->cur_bw = state->guess = 0;

		// to calculate the current bandwidth for every connection
		unused = false;
		for(bw_records_t::iterator rit = state->records->begin(),end=state->records->end();rit!=end;rit++)
			if( !( rit->t.time<since.time || (rit->t.time==since.time && since.millitm>rit->t.millitm)) )
			{
				cur_bw += rit->size ;
				state->cur_bw += rit->size;
			}
			else
			{
				zed = rit;
				unused = true;
			}
		if( unused ) state->records->erase(state->records->begin(),zed);

		// looking for the minimal bandwidth
		if( !state->active ) continue;
		if( min_bw==state->cur_bw ) min_count ++;
		if( min_bw>state->cur_bw )
		{
			min_bw = state->cur_bw;
			min_count = 1;
		}
	}

	// to do some guess for every connection
	uint32_t makebw = (int)(0.1 * (max_bw-cur_bw) + 0.5); // + 10 %
	uint32_t to_excess = max_bw - cur_bw;
	if( min_count>0 ) to_excess -= makebw;
    for(bw_states_t::iterator sit = states.begin(),end = states.end();sit!=end;sit++)
		if( sit->second.active )
		{
			sit->second.guess = uint32_t (((double)sit->second.cur_bw/(double)cur_bw)*to_excess);
			if( min_bw==sit->second.cur_bw )
				sit->second.guess += (makebw/min_count);
		}
	// we can't help increase the bandwidth (f weak connection

	//if( cur_bw<max_bw )
	//{
	//	//printf("\n");
	//        for(bw_states_t::iterator sit = states.begin(),end = states.end();sit!=end;sit++)
	//			if( sit->second.cur_bw )
	//				printf("<<<%d(%d),%d,%d>>>\n",sit->second.cur_bw,sit->second.guess,cur_bw,max_bw);
	//			else
	//				printf(".");
	//	//printf("\n");
	//}

	return cur_bw>max_bw;
}

uint32_t bwstop::guess(tcp_fsm::tcp_conn_t conn)
{
	bw_states_t::iterator it = states.find(conn);
	if( it!=states.end() )
		return it->second.guess;

	return 0;
}

bool bwstop::can(tcp_fsm::tcp_conn_t conn,uint32_t* guess)
{
	bw_states_t::iterator it = states.find(conn);
	if( it!=states.end() && it->second.active )
	{
		*guess = it->second.guess;
		return true;
	}
	return false;
}

timeb bwstop::get_time()
{
	if( emulator )
		return vtime;

	timeb res;
	ftime(&res);
	return res;
}

inline uint32_t mod(uint32_t value,uint32_t factor) { return value - (value%factor)*factor; }
void bwstop::turn(uint32_t msecs)
{
	vtime.time += msecs%1000;
	msecs = mod(msecs,1000);

	if( (vtime.millitm+msecs)>1000 )
		vtime.time += 1;
	
	vtime.millitm = mod(vtime.millitm+msecs,1000);
}

}




