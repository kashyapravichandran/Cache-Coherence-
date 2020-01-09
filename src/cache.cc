/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b,int p )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
	procs = p;
   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
	BusRdx=0;
	invalidation=0;
	intervention=0;
	memtransaction=0;
	flushes=0;  
  	cache2cache=0;
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(DIRTY);
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MODIFIED||victim->getFlags()==Sm) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int i,int protocol)
{ 
	printf("===== Simulation results (Cache %d)   =====\n",i);
	int m=0;
	if(protocol==0)
	m=memtransaction+readMisses+writeMisses+writeBacks;
	else if (protocol==1)
	m=readMisses+writeMisses+writeBacks-cache2cache;
	else if(protocol == 2)
	m=readMisses+writeMisses+writeBacks;	
	
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	printf("01. number of reads:\t%lu",reads);
	printf("\n02. number of read misses:\t%lu",readMisses);
	printf("\n03. number of writes:\t%lu", writes);
	printf("\n04. number of write misses:\t%lu",writeMisses);
	printf("\n05. total miss rate:\t%0.2f",float((readMisses+writeMisses))/(reads+writes)*100);
	cout<<"%";
	printf("\n06. number of writebacks:\t%lu",getWB());
	printf("\n07. number of cache-to-cache transfers:\t%lu", getc2c());
	printf("\n08. number of memory transactions:\t%d",m);
	printf("\n09. number of interventions:\t%lu",intervention);
	printf("\n10. number of invalidations:\t%lu",invalidation);
	printf("\n11. number of flushes:\t%lu",flushes);
	printf("\n12. number of BusRdX:\t%lu",BusRdx);


}



// THis is where I have implemented my stuff 

void Cache::MSI(int p, ulong addr, char op, Cache **cachesArray)
{
   //self = p;            //Denotes self processor number.
   currentCycle++;     /*per cache global counter to maintain LRU order 
			               among cache ways, updated on every cache access*/
    //cout<<"\n"<<addr<<"\t"<<p<<"\t"<<op;
	    	
	if( op == 'w')
		writes++;
	else if ( op == 'r' ) 
		reads++;

/*** Processor requests ***/
if ( op == 'r' || op == 'w' )   
{   
	cacheLine * line = findLine(addr);

	if(line == NULL)	/*miss, means it is INVALID state */
	{
		if( op == 'w' ) 
			writeMisses++;
		else 
			readMisses++;

      cacheLine *newline = fillLine(addr);
      //newLine is the MRU now.  

      if (op == 'w')
      {
         newline->setFlags(MODIFIED);

         //Issue BusRdX
         BusRdx++;
         
		 for(int i=0;i<procs;i++)
         {
           if(i!=p) 
            cachesArray[i]->MSI(i, addr, 'x',cachesArray);
         }
      }

      if ( op == 'r' )
      {
         newline->setFlags(SHARED);
		
         for(int i=0;i<procs;i++)
         {
            if(i!=p)
            cachesArray[i]->MSI(i, addr, 'd',cachesArray);
         }
      }  
		
	}  //MISS end

	else     /*hit*/
	{
		/**since it's a hit, update LRU**/
		updateLRU(line);

      //MODIFIED state
      if( line->getFlags() == MODIFIED )
      {
          ; 
		 
      }  
      else if( line->getFlags() == SHARED)
      {
         if( op == 'r')
         {
            line->setFlags(SHARED);
         }

         if( op == 'w')
         {
            line->setFlags(MODIFIED);

            //Issue BusRdX
            BusRdx++;
            memtransaction++;
            for(int i=0;i<procs;i++)
            {
              if(i!=p)
			    cachesArray[i]->MSI(i, addr,'x',cachesArray) ;
            }
         }
      }  

	}  
} 
else if ( op == 'd' || op == 'x' )
{
   cacheLine * line = findLine(addr);

   if(line == NULL)  
   {
		;       
   } 

   else    /** Hit **/
   {
      //MODIFIED state
      if ( line->getFlags() == MODIFIED )
      {
         if ( op == 'd')
         {
            line->setFlags(SHARED);
		    intervention++;	     
	        writeBacks++;	
           flushes++;
         }

         if( op == 'x' )
         {
            line->setFlags(INVALID);
		    invalidation++;
			writeBacks++;
            flushes++;
         }
      } 
      else if ( line->getFlags() == SHARED )
      {
         if ( op == 'd')
         {
       			;
         }

         if ( op == 'x')
         {
            line->setFlags(INVALID);
		    invalidation++;
         }
      } 

   }  
      

} 

} 

void Cache::MESI(int proc,ulong addr, char op, Cache **cachesArray)
{
	currentCycle++;
	int is_present=0;
	int i;
	cacheLine *line;
	for(i=0;i<procs;i++)
	{
		if(i!=proc)
		{
			line=cachesArray[i]->findLine(addr);
			if(line)
			{
				is_present=1;
				break;
			}
		}
	}
	
	if(op=='w')
		writes++;
	else if(op=='r')
		reads++;
	
	if(op=='w'||op=='r')
	{
		line=findLine(addr);
		if(line == NULL)
		{
			if(op=='w')
				writeMisses++;
			else 
				readMisses++;
				
			cacheLine *newline = fillLine(addr);
			if(op=='w')
			{
				// go to modified and c2c must be increased
				newline->setFlags(MODIFIED);
				BusRdx++;
				if(is_present)
					cache2cache++;
				for(i=0;i<procs;i++)
				{
					if(i!=proc)
						cachesArray[i]->MESI(i, addr,'x', cachesArray);
				}
				 
			}
			else 
			{
				// check is present and go to exclusive if necessary
				if(is_present==0)
				{
					newline->setFlags(EXCLUSIVE);
					for(i=0;i<procs;i++)
					{
						if(i!=proc)
							cachesArray[i]->MESI(i,addr,'d',cachesArray);
						
					}
				}
				else 
				{
					newline->setFlags(SHARED);
					cache2cache++;
					for(i=0;i<procs;i++)
					{
						if(i!=proc)
							cachesArray[i]->MESI(i,addr,'d',cachesArray);
					}
				}
			}
		}
		else //hit 
		{
			updateLRU(line);
			if(line->getFlags()==MODIFIED)
				line->setFlags(MODIFIED);
			else if (line->getFlags()==SHARED)
			{
				if(op=='w')
				{
					line->setFlags(MODIFIED);
					//BusRdx++; cause upgrade
					for(i=0;i<procs;i++)
						if(i!=proc)
							cachesArray[i]->MESI(i,addr,'u',cachesArray);
				}
				else 
				{
					line->setFlags(SHARED);
				}
			}
			else if(line->getFlags()==EXCLUSIVE)
			{
				if(op=='r')
					line->setFlags(EXCLUSIVE);
				else if(op=='w')
				{
					line->setFlags(MODIFIED);
				}
			}
		}
	}
	else if(op=='d'||op=='x'||op=='u')
	{
		line=findLine(addr);
		if(line == NULL)
		{
			;	
		}
		else 
		{
			if(line->getFlags()==MODIFIED)
			{
				if(op=='d')
				{
					line->setFlags(SHARED);
					intervention++;
					writeBacks++;
					flushes++;
				}
				else if(op=='x')
				{
					line->setFlags(INVALID);
					invalidation++;
					writeBacks++;
					flushes++;	
				}
			}
			else if(line->getFlags()==SHARED)
			{
				if(op=='d')
					line->setFlags(SHARED);
				else if (op=='x')
				{
					invalidation++;
					line->setFlags(INVALID);
				}
				else if (op=='u')
				{
					invalidation++;
					line->setFlags(INVALID);
				}
			}
			else if(line->getFlags()==EXCLUSIVE)
			{
				if(op=='d')
				{
					line->setFlags(SHARED);
					intervention++;
				}
				else if(op=='x')
				{
					line->setFlags(INVALID);
					invalidation++;
				}
				else if(op=='u')
				{
					;
				}			
				
			}
		}	
	}
	
			
}

void Cache::dragon(int proc,ulong addr,char op, Cache **cachesArray)
{
	currentCycle++;
	int is_present=0;
	int i;
	cacheLine *line;
	for(i=0;i<procs;i++)
	{
		if(i!=proc)
		{
			line=cachesArray[i]->findLine(addr);
			if(line)
			{
				is_present=1;
				break;
			}
		}
	}
	
	if(op=='w')
		writes++;
	else if(op=='r')
		reads++;
	if(op=='r'||op=='w')
	{
		line=findLine(addr);
		if(line==NULL)
		{
			cacheLine *newline = fillLine(addr); 
			if(op=='r')
				readMisses++;
			else 
				writeMisses++;
			
			if(op=='r')
			{
				if(is_present)
				{
					newline->setFlags(Sc);
					for(i=0;i<procs;i++)
						if(i!=proc)
						{
							cachesArray[i]->dragon(i,addr,'d',cachesArray);
						}
				}
				else 
				{
					newline->setFlags(EXCLUSIVE);
					for(int i=0;i<procs;i++)
						if(i!=proc)
						{
							cachesArray[i]->dragon(i,addr,'d',cachesArray);
						}
				}
			}
			else 
			{
				if(is_present)
				{
					newline->setFlags(Sm);
					for(int i=0;i<procs;i++)
						if(i!=proc)
						{
							cachesArray[i]->dragon(i,addr,'d',cachesArray);
							cachesArray[i]->dragon(i,addr,'u',cachesArray);
						}	
				}
				else 
				{
					newline->setFlags(MODIFIED);
					for(int i=0;i<procs;i++)
						if(i!=proc)
						{
							cachesArray[i]->dragon(i,addr,'d',cachesArray);
						}
				}
			}
		}
		else 
		{
			updateLRU(line);
			if(line->getFlags()==EXCLUSIVE)
			{
				if(op=='r')
				{
					line->setFlags(EXCLUSIVE);
				}
				else if (op=='w')
				{
					line->setFlags(MODIFIED);
				}
			}
			else if(line->getFlags()==Sc)
			{
				if(op=='r')
				{
					line->setFlags(Sc);	
				}
				else 
				{
					if(is_present)
					{
						line->setFlags(Sm);
						for(i=0;i<procs;i++)
						{
							if(i!=proc)
								cachesArray[i]->dragon(i,addr,'u',cachesArray);
						}
					}
					else 
					{
						line->setFlags(MODIFIED);
						for(i=0;i<procs;i++)
						{
							if(i!=proc)
								cachesArray[i]->dragon(i,addr,'u',cachesArray);
						}
					}
				}	
			}
			else if(line->getFlags()==Sm)
			{
				if(op=='r')
				{
					line->setFlags(Sm);
				}
				else if(op=='w')
				{
					if(is_present)
					{
						line->setFlags(Sm);
						for(i=0;i<procs;i++)
						{
							if(i!=proc)
								cachesArray[i]->dragon(i,addr,'u',cachesArray);
						}
					}
					else 
					{
						line->setFlags(MODIFIED);
						for(i=0;i<procs;i++)
						{
							if(i!=proc)
								cachesArray[i]->dragon(i,addr,'u',cachesArray);
						}
					}
				}
			}
			else if(line->getFlags()==MODIFIED)
			{
				if(op=='r')
				{
					line->setFlags(MODIFIED);
				}
				else if(op=='w')
				{
					line->setFlags(MODIFIED);
				}
			}
		}
	}
	else if(op=='d'||op=='x'||op=='u')
	{
		line=findLine(addr);
		if(line==NULL)
		{
			;
		}
		else 
		{
			if(line->getFlags()==EXCLUSIVE)
			{
				if(op=='d')
				{
					line->setFlags(Sc);
					intervention++;
				}
				else if(op=='u')
				{
					;
				}
				
			
			}
			else if (line->getFlags()==Sc)
			{
				if(op=='d')
				{
					line->setFlags(Sc);
				}
				if(op=='u')
				{
					;
				}	
			}
			else if (line->getFlags()==Sm)
			{
				if(op=='d')
				{
					line->setFlags(Sm);
					flushes++;
					//writeBacks++;
				}
				else if(op=='u')
				{
					line->setFlags(Sc);
				}	
			}
			else if(line->getFlags()==MODIFIED)
			{
				if(op=='d')
				{
					intervention++;
					flushes++;
					//writeBacks++;
					line->setFlags(Sm);
				}
				else if(op=='u')
				{
					;
				}
			}
		}
	}
}

