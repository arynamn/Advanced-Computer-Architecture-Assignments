#include<stdio.h>
#include<string.h>
#include "pin.H"

FILE * trace;
PIN_LOCK pinLock;


int countM = 0;
int countX = 0;
unsigned int maxSize = 0;

VOID Convertx86ToMachineAddress(char instype, unsigned long long addr, UINT32 size, THREADID tid)
{
    	
    	unsigned long long limit = addr | 0x000000000000003F;
	countX++;
	while(size > 0)
	{
		while( (addr+7) <= limit && size > 7)
		{
			printf("%d \r",countM++);
			
			fprintf(trace,"%c  %d  %llu \n", instype, tid, addr);
            		addr += 8;
            		size -= 8;
		}
		while( (addr+3) <= limit && size > 3)
		{
			printf("%d \r",countM++);
			
			fprintf(trace,"%c  %d  %llu \n", instype, tid, addr);
            		addr += 4;
            		size -= 4;
		}
		while( (addr+1) <= limit && size > 1)
		{
			printf("%d \r",countM++);
			
			fprintf(trace,"%c  %d  %llu \n", instype, tid, addr);
            		addr += 2;
            		size -= 2;
		}
		while( (addr) <= limit && size > 0)
		{
			printf("%d \r",countM++);
			
			fprintf(trace,"%c  %d  %llu \n", instype, tid, addr);
            		addr += 1;
            		size -= 1;
		}
		if (size > 0)
            	{
            		limit += 64;
            	}
          }
}
		

VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID tid)
{
    	PIN_GetLock(&pinLock, tid+1);
    	Convertx86ToMachineAddress('R',(unsigned long long)addr, size, tid);
    	fflush(trace); 
    	PIN_ReleaseLock(&pinLock);
}


VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID tid)
{
   	PIN_GetLock(&pinLock, tid+1);
    	Convertx86ToMachineAddress('W',(unsigned long long)addr, size, tid);
    	fflush(trace);    
    	PIN_ReleaseLock(&pinLock);
}


VOID Instruction(INS ins, VOID *v)
{
    	UINT32 memOperands = INS_MemoryOperandCount(ins);
	
    	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    	{
    		UINT32 memOpSize = INS_MemoryOperandSize(ins, memOp);
    		if( memOpSize > maxSize )
    			maxSize = memOpSize;
        	if (INS_MemoryOperandIsRead(ins, memOp))
        	{
            		INS_InsertPredicatedCall
            		(
                		ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                		IARG_INST_PTR,
                		IARG_MEMORYOP_EA, memOp,
                		IARG_UINT32, memOpSize,
                		IARG_THREAD_ID,
                		IARG_END
                	);
        	}

        	if (INS_MemoryOperandIsWritten(ins, memOp))
        	{
            		INS_InsertPredicatedCall
                	(
                		ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                		IARG_INST_PTR,
                		IARG_MEMORYOP_EA, memOp,
                		IARG_UINT32, memOpSize,
                		IARG_THREAD_ID,
                		IARG_END
                	);
        	}
        	
    	}
}

VOID Fini(INT32 code, VOID *v)
{
	printf("\n Machine Address : %d \n", countM);
	printf(" x86 Address : %d \n", countX);
	printf(" Max Block Size : %u \n", maxSize);
    	fprintf(trace, "#eof\n");
    	fclose(trace);
}

INT32 Usage()
{
    	PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    	return -1;
}

int main(int argc, char *argv[])
{
    	if (PIN_Init(argc, argv)) 
    		return Usage();
    		
	
    	trace = fopen("addrtrace.out", "w");

    	INS_AddInstrumentFunction(Instruction, 0);

    	PIN_AddFiniFunction(Fini, 0);

    	PIN_StartProgram();
    
    	return 0;
}
