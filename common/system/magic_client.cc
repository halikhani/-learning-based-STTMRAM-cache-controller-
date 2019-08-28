#include "magic_client.h"
#include "magic_server.h"
#include "sim_api.h"
#include "simulator.h"
#include "core.h"
#include "core_manager.h"
#include "thread.h"
#include "thread_manager.h"

static UInt64 handleMagic(thread_id_t thread_id, UInt64 cmd, UInt64 arg0 = 0, UInt64 arg1 = 0)
{
   Thread *thread = (thread_id == INVALID_THREAD_ID) ? NULL : Sim()->getThreadManager()->getThreadFromID(thread_id);
   Core *core = thread == NULL ? NULL : thread->getCore();
   return Sim()->getMagicServer()->Magic(thread_id, core ? core->getId() : INVALID_CORE_ID, cmd, arg0, arg1);
}

void setInstrumentationMode(UInt64 opt)
{
   handleMagic(INVALID_THREAD_ID, SIM_CMD_INSTRUMENT_MODE, opt);
}

UInt64 handleMagicInstruction(thread_id_t thread_id, UInt64 cmd, UInt64 arg0, UInt64 arg1)
{
   int table_return = -1;
   switch(cmd)
   {
   case SIM_CMD_ROI_TOGGLE:
   case SIM_CMD_ROI_START:
   case SIM_CMD_ROI_END:
   case SIM_CMD_MHZ_SET:
   case SIM_CMD_MARKER:
   case SIM_CMD_NAMED_MARKER:
   case SIM_CMD_USER:
   //AMHM Start
   case AMHM_APPROX:
       table_return = Sim()->approx_table_search((unsigned long long int) arg0);
       if(table_return != -1) {
           printf("AMHM: A quality level is already assigned to a data starts from address 0x%llx.\n\
                   If you want to assign another quality level to this data, please first remove it\n\
                   with AMHM_accurate command and then add it again to approximation table with a\n\
                   AMHM_approx following with AMHM_qual commands.\n", (unsigned long long int) arg0);
       }
       else {
            Sim()->approx_table_entry = (Sim()->approx_table_entry + 1) % approx_table_max_entry;
            Sim()->approx_table[Sim()->approx_table_entry].start_address = arg0;
            Sim()->approx_table[Sim()->approx_table_entry].end_address = arg1;
            Sim()->approx_table[Sim()->approx_table_entry].quality_level = 0;
            //Aligning addresses considering 64 byte cache block size for fault injection
            Sim()->approx_table[Sim()->approx_table_entry].start_address = Sim()->approx_table[Sim()->approx_table_entry].start_address + 0xD0;
            Sim()->approx_table[Sim()->approx_table_entry].start_address = Sim()->approx_table[Sim()->approx_table_entry].start_address & 0xFFFFFFFFFFC0;
            Sim()->approx_table[Sim()->approx_table_entry].end_address = Sim()->approx_table[Sim()->approx_table_entry].end_address & 0xFFFFFFFFFFC0;
       }
       return 0;
   case AMHM_QUAL:
       Sim()->approx_table[Sim()->approx_table_entry].quality_level = *(double*) &arg0;
       printf("AMHM: Start Address Fed to Sniper: 0x%llx, End Address Fed to Sniper: 0x%llx, and Quality Level Fed to Sniper: %e\n",\
              Sim()->approx_table[Sim()->approx_table_entry].start_address, Sim()->approx_table[Sim()->approx_table_entry].end_address,\
              Sim()->approx_table[Sim()->approx_table_entry].quality_level);
       return 0;
   case AMHM_ACCURATE:
       table_return = Sim()->approx_table_del((unsigned long long int) arg0);
       if(table_return != -1) {
            Sim()->approx_table[table_return].quality_level = 0;
            printf("AMHM: Address 0x%llx is set to accurate mode\n", (unsigned long long int) arg0);
       }
       else
            printf("AMHM: Address 0x%llx is not found in approximation table\n", (unsigned long long int) arg0);
       return 0;     
   //AMHM End
   case SIM_CMD_INSTRUMENT_MODE:
   case SIM_CMD_MHZ_GET:
   case SIM_CMD_SET_THREAD_NAME:
      return handleMagic(thread_id, cmd, arg0, arg1);
   case SIM_CMD_PROC_ID:
   {
      Core *core = Sim()->getCoreManager()->getCurrentCore();
      return core->getId();
   }
   case SIM_CMD_THREAD_ID:
      return thread_id;
   case SIM_CMD_NUM_PROCS:
      return Sim()->getConfig()->getApplicationCores();
   case SIM_CMD_NUM_THREADS:
      return Sim()->getThreadManager()->getNumThreads();
   case SIM_CMD_IN_SIMULATOR:
      return 0;
   default:
      LOG_PRINT_WARNING_ONCE("Encountered unknown magic instruction cmd(%u)", cmd);
      return 1;
   }
}
