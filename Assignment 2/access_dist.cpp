#include<bits/stdc++.h>
using namespace std;
typedef struct 
{
	char                type;
    	int                 threadid;
	unsigned long long  addr;
}trace_entry;

map< unsigned long long int , vector<unsigned long long int>> accesses;
map< unsigned long long int , unsigned long long int> distances;   

void increment(unsigned long long int d)
{
    if( distances.find(d) == distances.end() )
    {
        distances[d] = 1;
    }
    else
    {
        distances[d]++;
    }
}
int compute_distances()
{
    int N = 0;
    for (auto const &kv : accesses) 
    {
        vector<unsigned long long int> t = kv.second;
        for(unsigned long long int i = 1; i < t.size() ; i++)
        {
            unsigned long long int d = t[i] - t[i-1];
            increment(d);
            N++;
        }
    }
    return N;
}
void store_cdf_values(char *out,unsigned long long int N)
{
    unsigned long long int prev_count = 0;
    FILE *fp;

    string s = out;
    s.append("_dist.out");
    fp = fopen(s.c_str(),"w");
    for (auto const &kv : distances) 
    {
       prev_count += kv.second;
       double cdf_count = (double) prev_count /  (double)N;
       fprintf(fp,"%llu,%lf\n",kv.first, cdf_count);    
    }
}

int main(int argc , char **argv)
{	
   
    FILE *fp;
	int count = 0;
	trace_entry trace;	
	fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	while (!feof(fp)) 
	{
	    fscanf(fp,"%c  %d  %llu\n", &trace.type, &trace.threadid, &trace.addr);
        trace.addr  = trace.addr>>6;
        accesses[trace.addr].push_back(count);
        count++;
		cout<<count<<"\r";
	}
    unsigned long long int N = compute_distances();
    store_cdf_values(argv[1], N);
    return 0;
}
