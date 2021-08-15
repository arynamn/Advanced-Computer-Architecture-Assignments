#include <bits/stdc++.h>
#include<unistd.h>
using namespace std;

#define L1_NUMSET 64
#define L1_ASSOC 8
#define L2_NUMSET 512
#define L2_ASSOC 16
#define INVALID_TAG 0xfffffffffffffffULL
unsigned long long int microsecond = 1000000;

ofstream logger;

typedef struct br
{
	unsigned long long int gcount;
	unsigned long long int freq;
}breaker;

enum class msgtype
{
	get = 0,
	getx,
	put,
	putx,
	ack,
	inv,
	swb,
	ewb
};
string pmsgtype[] =
{
    "get",
	"getx",
	"put",
	"putx",
	"ack",
	"inv",
	"swb",
	"ewb"
};
enum class states
{
	M = 0,
	E,
	S,
	I
};
char pstates[] =
{
    'M','E','S','I'
};


typedef struct cll1
{
	unsigned long long tag;
	int no_of_sharers;
	states state;
} cache_line_l1;
typedef struct cll2
{
	unsigned long long tag;
	bool sharers[8];
	states state;
	bool dirty;
} cache_line_l2;

typedef struct tentry
{
	unsigned long long int gcount;
	char rwtype;
	int threadid;
	unsigned long long int addr;
	unsigned long long int accessed;
} trace_entry;

typedef struct msgformat
{
	msgtype msg;
	unsigned long long addr;
	unsigned long long int source;
	unsigned long long int dest;
	unsigned long long int initsource;
	unsigned long long int no_of_sharers;
} message;

typedef struct l1adr
{
	unsigned long long int tag;
	unsigned long long int setid;
} l1_addr;

typedef struct l2adr
{
	unsigned long long int tag;
	unsigned long long int setid;
	unsigned long long int bankid;
} l2_addr;

class LRU
{
	list<unsigned long long int> tags;

	public:
	LRU()
	{
	}
	void updateLRU(unsigned long long tag)
	{
		tags.remove(tag);
		tags.push_front(tag);
	}
	unsigned long long evictLRU()
	{
		if (tags.empty())
			return INVALID_TAG;
		unsigned long long tag = tags.back();
		tags.pop_back();
		return tag;
	}
	void remove_tag(unsigned long long int tag)
	{
		tags.remove(tag);
	}
	void printLRU()
	{
		cout<< " LRU ";
		for (auto p : tags)
		{
			cout << p << "  ";
		}
		cout << "\n";
	}
};

queue<trace_entry> input_queue[8];
queue<message> message_queue_l1[8];
queue<message> message_queue_l2[8];

void print_queues_stats()
{
	//logger <<"Input Queue:";
	for(int j = 0 ; j < 8 ; j++)
	{
		//logger <<input_queue[j].size() << " ";
	}
	//logger <<"ML1 Queue:";
	for(int j = 0 ; j < 8 ; j++)
	{
		//logger <<message_queue_l1[j].size() << " ";
	}
	//logger <<"ML2 Queue:";
	for(int j = 0 ; j < 8 ; j++)
	{
		//logger <<message_queue_l2[j].size() << " ";
	}
}

message create_new_message()
{
	message newm;
	newm.addr = INVALID_TAG;
	newm.dest = 8;
	newm.initsource = 8;
	newm.msg = msgtype::inv;
	newm.no_of_sharers = 0;
	newm.source = 8;
	return newm;
}

class cache_sim
{
	cache_line_l1 L1_Cache[8][L1_NUMSET][L1_ASSOC];
	cache_line_l2 L2_Cache[8][L2_NUMSET][L2_ASSOC];
	LRU L1_lru[8][L1_NUMSET];
	LRU L2_lru[8][L2_NUMSET];
	unsigned long long int l1hit, l1miss, l2hit, l2miss,l1accesses;
	unsigned long long int l1get,l1getx,l1put,l1putx,l1inv,l1ack;
	unsigned long long int l2get,l2getx,l2swb,l2ack,l2ewb;
	unsigned long long cycle_counter;
	breaker br;

	l1_addr getL1Addr(unsigned long long int inaddr)
	{
		l1_addr t;
		t.setid = (inaddr) & (63);
		t.tag = (inaddr >> (6));
		return t;
	}
	
	l2_addr getL2Addr(unsigned long long int inaddr)
	{
		l2_addr t;
		t.bankid = (inaddr) & (7);
		t.setid = (inaddr >> 3) & (511);
		t.tag = (inaddr >> (3 + 9));
		return t;
	}
	
	void clean_l1_cache(int processorid,int setid,int setindex)
	{
		L2_Cache[processorid][setid][setindex].dirty = 0;
		L2_Cache[processorid][setid][setindex].tag = INVALID_TAG;
		L2_Cache[processorid][setid][setindex].state = states::I;
		for(int i=0; i<L2_ASSOC; i++)
			L2_Cache[processorid][setid][setindex].sharers[i] = 0;
	}
	
	void clean_l2_cache(int bankid,int setid,int setindex)
	{
		L2_Cache[bankid][setid][setindex].dirty = 0;
		L2_Cache[bankid][setid][setindex].tag = INVALID_TAG;
		L2_Cache[bankid][setid][setindex].state = states::I;
		for(int i=0; i<L2_ASSOC; i++)
			L2_Cache[bankid][setid][setindex].sharers[i] = 0;
	}
	
	int check_gcounts(int gcount)
	{
		int flag = 0;
		////logger <<"  ---- GCOUNTER ---- "<< gcount<<" ";
		for(int i=0; i < 8 && !input_queue[i].empty() ; i++)
		{
			////logger <<i<<" "<<input_queue[i].front().gcount<<"  -";
			if(gcount > input_queue[i].front().gcount)
			{
				return 0;
			}
		}
		return 1;
	}


public:
	cache_sim()
	{
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < L1_NUMSET; j++)
			{
				for(int l1way = 0; l1way < L1_ASSOC ; l1way++)
				{
					L1_Cache[i][j][l1way].tag = INVALID_TAG;
					L1_Cache[i][j][l1way].state = states::I;
					L1_Cache[i][j][l1way].no_of_sharers = 0;
				}
			}
			for (int j = 0; j < L2_NUMSET; j++)
			{
				for(int l2way = 0; l2way < L2_ASSOC ; l2way++)
				{
					L2_Cache[i][j][l2way].tag = INVALID_TAG;
					L2_Cache[i][j][l2way].state = states::I;
					L2_Cache[i][j][l2way].dirty = false;
					for(int shid = 0; shid < 8; shid++)
						L2_Cache[i][j][l2way].sharers[shid] = false;
				}
			}
		}
		l1hit = 0;
		l1miss = 0;
		l2hit = 0;
		l2miss = 0;
		l1get  = 0;
		l1getx  = 0;
		l1put  = 0;
		l1putx  = 0;
		l1inv  = 0;
		l1ack  = 0;
		l2get  = 0;
		l2getx  = 0;
		l2swb  = 0;
		l2ack  = 0;
		l2ewb  = 0;
		//logger.open("logs/logfile.txt");

		br.gcount = 0;
		br.freq = 0;
	}
	
	~cache_sim()
	{
		
	}
	
	void print_report()
	{
		l1accesses = l1hit + l1miss;
		cout << "No. of simulated cycles    " << cycle_counter-1 << endl;
		cout << "L1 Accesses    " << l1accesses << endl;
		cout << "L1 MISS   " << l1miss << endl;
		cout << "L2 MISS   " << l2miss << endl;
		cout << "L1get     " <<  l1get  << endl;
		cout << "L1getx    " <<  l1getx  << endl;
		cout << "L1put     " <<  l1put  << endl;
		cout << "L1putx    " <<  l1putx  << endl;
		cout << "L1inv     " <<  l1inv  << endl;
		cout << "L1ack     " <<  l1ack  << endl;
		cout << "L2get     " <<  l2get  << endl;
		cout << "L2getx    " <<  l2getx  << endl;
		cout << "L2swb     " <<  l2swb  << endl;
		cout << "L2ack     " <<  l2ack  << endl;
		cout << "L2ewb     " <<  l2ewb  << endl;
	}

	int process_trace_l1(int pid, trace_entry tracetemp, message msgtemp)
	{															
		if (msgtemp.addr != INVALID_TAG)
		{
			//logger <<"L1Queue Accessed :	Src:"<<msgtemp.source<<"  Dest:"<<msgtemp.dest<<"  Interim:"<<msgtemp.initsource << "  NoSh:"<<msgtemp.no_of_sharers<<
					//"  Mtype:"<< pmsgtype[int(msgtemp.msg)]<<"\n";

			message t = msgtemp;																			// Get head message
			if (t.msg == msgtype::get)																		// Get message 
			{
				//logger <<" get\n";
				l1get++;
				l1_addr l1addr = getL1Addr(t.addr);
				int l1way;
				for (l1way = 0; l1way < L1_ASSOC; l1way++)													// Find location of tag in L1 Cache
				{
					if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
						break;
				}
				assert( l1way < L1_ASSOC );

				L1_Cache[pid][l1addr.setid][l1way].state = states::S;										// Set Shared State

				message swbt = create_new_message();
				swbt.msg = msgtype::swb;
				swbt.addr = t.addr;
				swbt.source = pid;
				swbt.dest = t.source;
				swbt.no_of_sharers = 2;

				message putt = create_new_message();
				putt.msg = msgtype::put ;
				putt.addr = t.addr;
				putt.source = pid;
				putt.dest = t.initsource;  
				putt.no_of_sharers = 1 ;

				message_queue_l2[swbt.dest].push(swbt);														// Sending SWB to L2
				message_queue_l1[putt.dest].push(putt);														// Send Data to L1
			}
			
			else if (t.msg == msgtype::getx)																// Getx message from another L1
			{
				//logger <<" getx\n";
				l1getx++;
				l1_addr l1addr = getL1Addr(t.addr);
				int l1way;
				for (l1way = 0; l1way < L1_ASSOC; l1way++)													// Find location of tag in L1 Cache
				{
					if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
						break;
				}
				assert( l1way < L1_ASSOC );

				L1_Cache[pid][l1addr.setid][l1way].tag = INVALID_TAG;										// Invalidate current cache value
				L1_Cache[pid][l1addr.setid][l1way].state = states::I;
				L1_lru[pid][l1addr.setid].remove_tag(l1addr.tag);

				message ackt = create_new_message();
				ackt.msg    = msgtype::ack;
				ackt.addr   = t.addr;
				ackt.source = pid;
				ackt.dest   = t.source;

				message putxt = create_new_message();
				putxt.msg = msgtype::putx ;
				putxt.addr = t.addr;
				putxt.source = pid;
				putxt.dest = t.initsource;
				putxt.initsource = t.source;
				putxt.no_of_sharers = 0;

				message_queue_l2[ackt.dest].push(ackt);														// Send ack to L2
				message_queue_l1[putxt.dest].push(putxt);													// Send Data to L1
			}
			
			else if (t.msg == msgtype::put)																	// Put Message
			{
				//logger <<" put\n";
				l1put++;
				l1_addr l1addr = getL1Addr(t.addr);
				unsigned long long int l1way,l1lrutag;
				for(l1way = 0; l1way < L1_ASSOC ; l1way++ )													// Find Empty Location
				{
					if (L1_Cache[pid][l1addr.setid][l1way].tag == INVALID_TAG)
						break;
				}
				if(l1way == L1_ASSOC)
				{
					l1lrutag = L1_lru[pid][l1addr.setid].evictLRU(); 											// Find LRU
					for (l1way = 0; l1way < L1_ASSOC; l1way++)											
					{
						if (L1_Cache[pid][l1addr.setid][l1way].tag == l1lrutag)
							break;
					}

					unsigned long long evictedaddr = l1lrutag*64 + l1addr.setid;
					l2_addr evictedl2addr = getL2Addr(evictedaddr);

					message ewbt = create_new_message();
					ewbt.msg = msgtype::ewb;
					ewbt.addr = evictedaddr;
					ewbt.source = pid;
					ewbt.dest = evictedl2addr.bankid;
					message_queue_l2[ewbt.dest].push(ewbt);

					clean_l1_cache(pid,l1addr.setid,l1way);													// Reset Cache
				}
				assert(l1way < L1_ASSOC);				
				L1_Cache[pid][l1addr.setid][l1way].tag = l1addr.tag;										// Fill Data
				L1_Cache[pid][l1addr.setid][l1way].no_of_sharers = t.no_of_sharers;							// Set no of sharers if received data is modified by others
				L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);

				if( t.no_of_sharers == 0 )
					L1_Cache[pid][l1addr.setid][l1way].state = states::E;
				else
					L1_Cache[pid][l1addr.setid][l1way].state = states::S;			
			}
			
			else if (t.msg == msgtype::putx)																// Recieved Data with Exclusive Access
			{
				//logger <<" putx\n";
				l1putx++;
				l1_addr l1addr = getL1Addr(t.addr);
				unsigned long long int l1way,l1lrutag;
				for(l1way = 0; l1way < L1_ASSOC ; l1way++ )													// Find Empty Location in L1
				{
					if ( L1_Cache[pid][l1addr.setid][l1way].tag == INVALID_TAG || L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag )
						break;
				}
				if(l1way == L1_ASSOC)																		// No empty Location , Evitct LRu
				{
					l1lrutag = L1_lru[pid][l1addr.setid].evictLRU();										// Find LRU
					for (l1way = 0; l1way < L1_ASSOC; l1way++)											
					{
						if (L1_Cache[pid][l1addr.setid][l1way].tag == l1lrutag)
							break;
					}

					unsigned long long evictedaddr = l1lrutag*64 + l1addr.setid;
					l2_addr evictedl2addr = getL2Addr(evictedaddr);

					message ewbt = create_new_message();
					ewbt.msg = msgtype::ewb;
					ewbt.addr = evictedaddr;
					ewbt.source = pid;
					ewbt.dest = evictedl2addr.bankid;
					message_queue_l2[ewbt.dest].push(ewbt);

					clean_l1_cache(pid,l1addr.setid,l1way);													// Reset Cache
				}
				//logger<<"\n Error : l1way:"<<l1way<<" lrutag:"<<l1lrutag<<" ::::";
				for(int i = 0; i< L1_ASSOC; i++)
				{
					//logger<<L1_Cache[pid][l1addr.setid][l1way].tag<<"   ";
				}
				//logger<<"\n";
				assert( l1way < L1_ASSOC );

				L1_Cache[pid][l1addr.setid][l1way].tag = l1addr.tag;										// Fill Data in L1 Cache, Recieved Exclusive Access
				L1_Cache[pid][l1addr.setid][l1way].no_of_sharers = t.no_of_sharers;
				L1_Cache[pid][l1addr.setid][l1way].state = states::M;
				L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);
			}
			
			else if (t.msg == msgtype::inv)																	// Invalidate Value in L1
			{
				//logger <<" inv\n";
				l1inv++;
				l1_addr l1addr = getL1Addr(t.addr);
				unsigned long long int l1way;
				for(l1way = 0; l1way < L1_ASSOC ; l1way++ )													// Find location in set
				{
					if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
						break;
				}
				assert( l1way < L1_ASSOC );

				if( t.initsource < 8 )
				{
					message ackt = create_new_message();
					ackt.msg = msgtype::ack;
					ackt.addr = t.addr;
					ackt.source = pid;
					ackt.dest = t.initsource;

					message_queue_l1[ackt.dest].push(ackt);
				}

				L1_Cache[pid][l1addr.setid][l1way].tag = INVALID_TAG;										// Invalidate
				L1_Cache[pid][l1addr.setid][l1way].state = states::I;
				L1_lru[pid][l1addr.setid].remove_tag(l1addr.tag);
			}

			else if (t.msg == msgtype::ack)
			{
				l1ack++;
				l1_addr l1addr = getL1Addr(t.addr);
				int l1way;
				for(l1way = 0; l1way < L1_ASSOC ; l1way++ )													// Find location in set
				{
					if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
						break;
				}
				assert( l1way < L1_ASSOC );
				L1_Cache[pid][l1addr.setid][l1way].no_of_sharers--;
			}
		}
		
		if(tracetemp.addr != INVALID_TAG )
		{
			trace_entry t = tracetemp;
			l1_addr l1addr = getL1Addr(t.addr);
			int can_start_flag = check_gcounts(t.gcount);														// Check if all previous gcount is processeds
			if(can_start_flag == 1)	
			{
				if( br.gcount == t.gcount)																		// Early Stopper
				{
					br.freq++;
				}
				else
				{
					br.freq = 0;
					br.gcount = t.gcount;
				}

				//logger <<"Input Queue Accessed : "<<"RW:"<<tracetemp.rwtype<<"  AccF:"<<tracetemp.accessed<<"  Tid:"<<tracetemp.threadid<<"  GC:"<<tracetemp.gcount;
				//logger <<" CAF:"<<can_start_flag<<"\n";

				if(t.accessed == 1)																				// If the entry is accessed previously
				{
					int l1way;
					for (l1way = 0; l1way < L1_ASSOC; l1way++)													// Find location of tag in L1 Cache
					{
						if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
							break;
					}
					if(l1way < L1_ASSOC)																		// If data filled in L1 Cache
					{
						if(t.rwtype == 'W')
						{
							if ( (L1_Cache[pid][l1addr.setid][l1way].state == states::E || 						// Check for permission in case of write
										L1_Cache[pid][l1addr.setid][l1way].state == states::M) &&
											(L1_Cache[pid][l1addr.setid][l1way].no_of_sharers == 0) )
							{
								
								L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);
								return 1;
							}				
						}
						else																					// Read Permisson Check
						{
							L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);
							return 1;
						}
					}
				}
				else																							// Fresh Access
				{
					int l1way;
					for (l1way = 0; l1way < L1_ASSOC; l1way++)													// Find location of tag in L1 Cache
					{
						if (L1_Cache[pid][l1addr.setid][l1way].tag == l1addr.tag)
							break;
					}

					l2_addr l2addr = getL2Addr(t.addr);
					if(l1way == L1_ASSOC)																		// If does not exist in Cache 
					{
						l1miss++;
						if(t.rwtype == 'W')																		// If write then request getx
						{
							message getxt = create_new_message();
							getxt.msg = msgtype::getx ;
							getxt.addr = t.addr;
							getxt.source = pid;
							getxt.dest = l2addr.bankid;
							message_queue_l2[getxt.dest].push(getxt);
							input_queue[pid].front().accessed = 1;
						}
						else																					// if read request get
						{
							message gett = create_new_message();
							gett.msg = msgtype::get ;
							gett.addr = t.addr;
							gett.source = pid;
							gett.dest = l2addr.bankid;
							message_queue_l2[gett.dest].push(gett);
							input_queue[pid].front().accessed = 1;
						}
					}
					else																						// If exist in Cache
					{
						l1hit++;
						if(t.rwtype == 'W')																		// If write access, check for permission else request from l2
						{	
							if (  (L1_Cache[pid][l1addr.setid][l1way].state == states::M)  
								|| (L1_Cache[pid][l1addr.setid][l1way].state == states::E)   )
							{									
								L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);
								return 1;
							}
							else
							{
								message getxt = create_new_message();
								getxt.msg = msgtype::getx;
								getxt.addr = t.addr;
								getxt.source = pid;
								getxt.dest = l2addr.bankid;
								message_queue_l2[getxt.dest].push(getxt);
								input_queue[pid].front().accessed = 1;
							}
						}
						else																					
						{
							L1_lru[pid][l1addr.setid].updateLRU(l1addr.tag);
							return 1;
						}
					}
				}
			}
		}
		return 0;
	}

	int process_l2(int bankid, message msgtemp)
	{
		if(msgtemp.addr == INVALID_TAG)
			return 0;

		//logger <<"L2Queue Accessed :	Src:"<<msgtemp.source<<"  Dest:"<<msgtemp.dest<<"  NoSh:"<<msgtemp.no_of_sharers<< "  Mtype:"<< pmsgtype[int(msgtemp.msg)]<<"\n";

		message t = msgtemp;
		if (t.msg == msgtype::get)
		{
			//logger <<" get\n";
			l2get++;
			l2_addr l2addr = getL2Addr(t.addr);
			unsigned long long l2way,l2lrutag;

			for (l2way = 0; l2way < L2_ASSOC; l2way++)														// Find tag in L2 Cache
			{
				if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2addr.tag)
					break;
			}
			if(l2way < L2_ASSOC)
			{
				l2hit++;
			}
			else																							// Find Empty Block if not found in L2
			{
				l2miss++;
				for (l2way = 0; l2way < L2_ASSOC; l2way++)													// Find Invalid Block
				{
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == INVALID_TAG)
						break;
				}
				if( l2way == L2_ASSOC)																		// Remove LRU BLock if no invalid block
				{
					while(l2way == L2_ASSOC)
					{
						l2lrutag = L2_lru[l2addr.bankid][l2addr.setid].evictLRU();			// Find LRU
						for (l2way = 0; l2way < L2_ASSOC; l2way++)											
						{
							if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2lrutag)
								break;
						}
					}
					message invt = create_new_message();
					invt.msg    = msgtype::inv ;
					invt.addr   = l2lrutag*4096 + l2addr.setid*8 + l2addr.bankid;
					invt.source = bankid;

					for(int i=0; i < 8; i++)																// Send Invalidation Request to Processors 
					{
						if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
							message_queue_l1[i].push(invt);
					}
					clean_l2_cache(l2addr.bankid,l2addr.setid,l2way);										// Reset Cache
				}
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag = l2addr.tag;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
			}
			if( l2way == L2_ASSOC )
			{
				cout<< l2lrutag << "  :";
				for(int  i = 0 ; i < L2_ASSOC ; i++)
					cout<< L2_Cache[l2addr.bankid][l2addr.setid][i].tag <<"  ";
				L2_lru[l2addr.bankid][l2addr.setid].printLRU();
			}
			assert( l2way < L2_ASSOC );
			L2_lru[l2addr.bankid][l2addr.setid].updateLRU(l2addr.tag);										// Update LRU with new tag

			if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty == 0)									// If Cache Line is not dirty/modified
			{
				message putt = create_new_message();
				putt.msg = msgtype::put ;
				putt.addr = t.addr;
				putt.source = bankid;
				putt.dest = t.source;
				putt.no_of_sharers = 0;

				for (int i = 0; i < 8; i++)																	// Calculate no of sharers
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
						putt.no_of_sharers++;

				message_queue_l1[t.source].push(putt);														// Send Data return to Processor from get.
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source] = 1; 
				if (putt.no_of_sharers == 0)																// Set State
				{
					L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::M;
					L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 1;
				}
				else
				{
					L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::S;
					L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
				}
			}
			else
			{
				message gett = create_new_message();
				gett.msg    = msgtype::get ;
				gett.addr   = t.addr;
				gett.source = bankid;
				gett.initsource  = t.source;
				gett.no_of_sharers = 1 ;

				for (int i = 0; i < 8; i++)
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
						gett.dest = i;
				message_queue_l1[gett.dest].push(gett);
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::S;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source] = 1;
			}
		}

		else if (t.msg == msgtype::getx)
		{
			//logger <<" getx\n";
			l2getx++;
			l2_addr l2addr = getL2Addr(t.addr);
			unsigned long long l2way,l2lrutag;
			for (l2way = 0; l2way < L2_ASSOC; l2way++)													// Find tag in L2 Cache
			{
				if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2addr.tag)
					break;
			}
			if(l2way < L2_ASSOC)
			{
				l2hit++;
			}
			else																							// Find Empty Block if not found in L2
			{
				l2miss++;
				for (l2way = 0; l2way < L2_ASSOC; l2way++)												// Find Invalid Block
				{
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == INVALID_TAG)
						break;
				}
				if( l2way == L2_ASSOC)																	// Remove LRU BLock if no invalid block
				{
					while(l2way == L2_ASSOC)
					{
						l2lrutag = L2_lru[l2addr.bankid][l2addr.setid].evictLRU();			// Find LRU
						for (l2way = 0; l2way < L2_ASSOC; l2way++)											
						{
							if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2lrutag)
								break;
						}
					}
					/*l2lrutag = L2_lru[l2addr.bankid][l2addr.setid].evictLRU(); 							// Find LRU
					for (l2way = 0; l2way < L2_ASSOC; l2way++)											
					{
						if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2lrutag)
							break;
					}*/

					//logger<<"\n LRU Eviction:"<<l2lrutag<<"  pos:"<<l2way<<"   \n";
					for(int i = 0 ; i < L2_ASSOC; i++)
					{
						//logger<<L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag<< "   :";
					}
					//logger<<"\n";
					assert( l2way < L2_ASSOC );
					message invt = create_new_message();
					invt.msg    = msgtype::inv ;
					invt.addr   = l2lrutag*4096 + l2addr.setid*8 + l2addr.bankid;
					invt.source = bankid;

					for(int i=0; i < 8 ; i++)																// Send Invalidation Request to Processors 
					{
						if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
							message_queue_l1[i].push(invt);
					}

					L2_lru[l2addr.bankid][l2addr.setid].remove_tag(l2lrutag);

					clean_l2_cache(l2addr.bankid,l2addr.setid,l2way);										// Reset Cache
				}
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag = l2addr.tag;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
				//L2_lru[l2addr.bankid][l2addr.setid].updateLRU(l2addr.tag);
			}
			assert( l2way < L2_ASSOC );

			L2_lru[l2addr.bankid][l2addr.setid].updateLRU(l2addr.tag);										// Update LRU with new tag

			if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty == 0)									// If Cache Line is not dirty/modified
			{
				message putxt = create_new_message();
				putxt.msg = msgtype::putx ;
				putxt.addr = t.addr;
				putxt.source = bankid;
				putxt.dest = t.source;
				putxt.no_of_sharers = 0;

				message invt = create_new_message();
				invt.msg    = msgtype::inv ;
				invt.addr   = t.addr;
				invt.source = bankid;
				invt.initsource = t.source;

				for (int i = 0; i < 8 && i != t.source ; i++)												// Calculate no of sharers
				{
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
					{
						putxt.no_of_sharers++;
						message_queue_l1[i].push(invt);
						L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] = 0;
					}
				}
				message_queue_l1[putxt.dest].push(putxt);														// Send Data return to Processor from get.	
																	
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::M;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 1;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source] = 1;
			}
			else
			{
				message getxt = create_new_message();
				getxt.msg          = msgtype::getx ;
				getxt.addr         = t.addr;
				getxt.source       = bankid;
				getxt.initsource = t.source;

				for (int i = 0; i < 8; i++)
					if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
						getxt.dest = i;

				message_queue_l1[getxt.dest].push(getxt);

				L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[getxt.dest] = 0;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source]   = 1;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 1;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::M;
			}
		}

		else if (t.msg == msgtype::swb)
		{
			//logger <<" swb\n";
			l2swb++;
			l2_addr l2addr = getL2Addr(t.addr);
			unsigned long long l2way;
			for (l2way = 0; l2way < L2_ASSOC; l2way++)													// Find tag in L2 Cache
			{
				if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2addr.tag)
					break;
			}
			assert( l2way < L2_ASSOC );
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source] = 1; 
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::S;
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
		}

		else if (t.msg == msgtype::ack)
		{
			//logger <<" ack\n";
			l2ack++;
			l2_addr l2addr = getL2Addr(t.addr);
			unsigned long long l2way;
			for (l2way = 0; l2way < L2_ASSOC; l2way++)													// Find tag in L2 Cache
			{
				if (L2_Cache[l2addr.bankid][l2addr.setid][l2way].tag == l2addr.tag)
					break;
			}
			assert( l2way < L2_ASSOC );
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::M;
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 1;
		}

		else if (t.msg == msgtype::ewb)
		{
			l2ewb++;
			l2_addr l2addr = getL2Addr(t.addr);
			unsigned long long l2way;
			for (l2way = 0; l2way < L2_ASSOC; l2way++)													// Find tag in L2 Cache
			{
				if (L2_Cache[bankid][l2addr.setid][l2way].tag == l2addr.tag)
					break;
			}
			//assert( l2way < L2_ASSOC );
			L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[t.source] = 0;
			
			if( L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 1)
			{
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::S;
			}
			/*
			int counter = 0;
			for(int i = 0 ; i < 8; i++)
			{
				if( L2_Cache[l2addr.bankid][l2addr.setid][l2way].sharers[i] == 1)
					counter++;
			}
			if(counter == 0)
			{
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].dirty = 0;
				L2_Cache[l2addr.bankid][l2addr.setid][l2way].state = states::S;
			}
			*/
		}
		return 1;
	}

	void run()
	{
		cycle_counter = 0;
		while (1)
		{	
			trace_entry tracetemp[8];
			message msgl1temp[8],msgl2temp[8];
			int flag = 0;
			for (int pid = 0; pid < 8; pid++)
			{
				if(!input_queue[pid].empty())
				{
					tracetemp[pid] = input_queue[pid].front();
					flag = 1;
				}
				else
					tracetemp[pid].addr = INVALID_TAG;
				
				if( !message_queue_l1[pid].empty())
				{
					msgl1temp[pid] = message_queue_l1[pid].front();
					message_queue_l1[pid].pop();
					flag = 1;
				}
				else
					msgl1temp[pid].addr = INVALID_TAG;

				if( !message_queue_l2[pid].empty())
				{
					msgl2temp[pid] = message_queue_l2[pid].front();
					message_queue_l2[pid].pop();
					flag = 1;
				}
				else
					msgl2temp[pid].addr = INVALID_TAG;
			}

			for (int pid = 0; pid < 8; pid++ )
			{
				int processed_entry_flag = process_trace_l1(pid, tracetemp[pid], msgl1temp[pid]);
				process_l2(pid, msgl2temp[pid]);

				if(processed_entry_flag == 1)
					input_queue[pid].pop();
			}
			cout<<cycle_counter++ <<"\r";
			if ( br.freq > 20 || flag == 0 )
				break;
		}
	}
};

int main(int argc, char **argv)
{
	FILE *fp;
	int count = 0;
	trace_entry trace;
	cache_sim mycache;

	fp = fopen(argv[1], "rb");
	assert(fp != NULL);

	while (!feof(fp))
	{
		fscanf(fp, "%c  %d  %llu\n", &trace.rwtype, &trace.threadid, &trace.addr);
		trace.gcount = count;
		trace.accessed = 0;
		trace.addr = trace.addr >> 6;

		cout<<count<<"\r";
		input_queue[trace.threadid].push(trace);
		count++;
	}
	cout<<"File read Complete:";
	cout<<"\n";
	mycache.run();
	mycache.print_report();
	//logger << count << endl;
	return 0;
}