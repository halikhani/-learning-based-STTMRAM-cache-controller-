#include "fault_injector_random.h"
#include "rng.h"
#include "simulator.h"

FaultInjectorRandom::FaultInjectorRandom(UInt32 core_id, MemComponent::component_t mem_component)
   : FaultInjector(core_id, mem_component)
   , m_rng(rng_seed(0))
{
   if (mem_component == MemComponent::L2_CACHE)
      m_active = true;
   else
      m_active = false;
}

void
FaultInjectorRandom::preRead(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time)
{
   // Data at virtual address <addr> is about to be read with size <data_size>.
   // <location> corresponds to the physical location (cache line) where the data lives.
   // Update <fault> here according to errors that have accumulated in this memory location.
//    if(addr == (IntPtr) Sim()->approx_table[Sim()->approx_table_entry].start_address) {
//           printf("AMHM: Yaftam.\n");
//       }
    signed int entry = Sim()->approx_table_search(addr);
    if(entry != -1)
           Sim()->approx_table[entry].numberOfReads++;
    
}

void
FaultInjectorRandom::postWrite(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time)
{
   // Data at virtual address <addr> has just been written to.
   // Update <fault> here according to errors that occured during the writing of this memory location.
       // Dummy random fault injector
   if (m_active)
   {
       double random_number = 0;
       signed int entry = Sim()->approx_table_search(addr);
       if(entry != -1)
           Sim()->approx_table[entry].numberOfWrites++;
        //printf("error rate is %f\n",Sim()->get_error_rate(addr));
        for(UInt32 i = 0; i < data_size * 8; i++) {
            random_number = (double) rand() / RAND_MAX;
            if(random_number < Sim()->get_error_rate(addr)) {
                //printf("Man Injam FI, random number= %e Error rate= %e.\n", random_number, Sim()->get_error_rate(addr));
                if(entry != -1)
                    Sim()->approx_table[entry].numberOfInjectedFaults++;
                fault[i / 8] |= 1 << (i % 8);
//                    printf("Inserting bit %d flip at address %" PRIxPTR " on read access by core %d to component %s\n",
//                    i, addr, m_core_id, MemComponentString(m_mem_component));
            }
        }
   }
}
