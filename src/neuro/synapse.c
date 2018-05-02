#include "synapse.h"

struct spikedb_struct{							// Could alternatively use NeuroSpike structure from IOStack.h if time element is renabled as int
  double time;
  int core;
  int axon;
};

struct spikedb_struct* spikedb;
struct spikedb_struct db_ref;									// used to get offsets to make MPI struct so input spikes can be broadcast

int loadOrSaveDb(const char *zFilename, int isSave) {
  int rc = 0;                   /* Function return code */
  int mpi_myrank;
  int max_spikes = 2097152;								// initially allocate 2M entries, or 24MB to store spike input data. Can expand like C++ vector if necessary
  int num_spikes = 0;
  MPI_Comm_rank( MPI_COMM_WORLD, &mpi_myrank);

  //MPI_File fileheader;
  //MPI_File_open(MPI_COMM_WORLD, zFilename, MPI_MODE_RDONLY,MPI_INFO_NULL, &fileheader);		// HANGS FOR ABSOLUTELY NO REASON WITH MORE THAN 1 MPI RANK

  //if (mpi_myrank == 0)
  {
    printf("\nStarting to read %s.\n",zFilename);

    spikedb = malloc(sizeof(struct spikedb_struct) * max_spikes);
    int read_size = 4096;										// read this many bytes at a time. should be set equal to the IO system block size
    char* read_buf = malloc(sizeof(char) * read_size * 2);
    int bytes_read;
    char* token;
    char* line;
    int read_buf_offset = 0;
    char* read_buf_start = read_buf;

    //MPI_Status status;
    //MPI_File fileheader;
    //MPI_File_open(MPI_COMM_WORLD, zFilename, MPI_MODE_RDONLY,MPI_INFO_NULL, &fileheader);		// HANGS FOR ABSOLUTELY NO REASON WITH MORE THAN 1 MPI RANK
    FILE *fp = fopen(zFilename,"r");

    do
    {
      //rc = MPI_File_read (fileheader, read_buf + read_buf_offset, read_size, MPI_UNSIGNED_CHAR, &status);			//STATUS IS BUSTED WITH MPI_FILE_READ_ALL. always says bytes_read is read_size, even if it's not
      //MPI_Get_elements(&status, MPI_UNSIGNED_CHAR, &bytes_read);
      bytes_read = read(fileno(fp),read_buf + read_buf_offset,read_size);							// for some reason bytes_read is wrong

      //printf("Rank %d read %d bytes at %d.\n",mpi_myrank,bytes_read,read_buf_offset);
      read_buf[bytes_read + read_buf_offset] = '\0';
      while ((line = strsep(&read_buf,"\n")) != NULL)
      {
        if (read_buf == NULL)
          break;
        //printf("Rank %d read line: %s\n",mpi_myrank,line);
        spikedb[num_spikes].time = atof(strsep(&line,","));
        spikedb[num_spikes].core = atoi(strsep(&line,","));
        spikedb[num_spikes].axon = atoi(line);					// I could add checks to make sure the neuron was written with non-null data, but that needs branches and will slow things down a ton
        //printf("Rank %d read time %f for core %d axon %d.\n",mpi_myrank,spikedb[num_spikes].time,spikedb[num_spikes].core,spikedb[num_spikes].axon);
        num_spikes++;
        if (num_spikes == max_spikes)
        {
          max_spikes *= 2;
          spikedb = realloc(spikedb, sizeof(struct spikedb_struct) * max_spikes);					// for some reason dynamic memory allocation does not work with MPI, but does with C file IO
        }
      }
      read_buf = read_buf_start;
      read_buf_offset = read_size + read_buf_offset - (line - read_buf_start);
      strcpy(read_buf,line);
    } while (bytes_read == read_size && rc != -1);

    spikedb[num_spikes].core = -1;						// used to flag that this is the end of the array
    printf("Done with file read. Read %d spikes.\n",num_spikes);
    //num_spikes++;					//increment to bcast all bytes
    fclose(fp);
    //MPI_File_close(&fileheader);
    free(read_buf_start);
  }
  //MPI_File_close(&fileheader);
  /*MPI_Bcast(&max_spikes, 1, MPI_INT, 0,MPI_COMM_WORLD);
  if (mpi_myrank != 0)
    spikedb = malloc(sizeof(struct spikedb_struct) * max_spikes);

  MPI_Datatype spikedb_struct_mpi;						// I'm not sure how this works for the receiving ranks since they don't have anything in spikedb to reference. it seems to though
  MPI_Aint disp[3];
  disp[0] = &db_ref.time - (int*)&db_ref;
  disp[1] = &db_ref.core - (int*)&db_ref;
  disp[2] = &db_ref.axon - (int*)&db_ref;
  //disp[0] = &spikedb[0].time - (int*)&spikedb[0];
  //disp[1] = &spikedb[0].core - (int*)&spikedb[0];
  //disp[2] = &spikedb[0].axon - (int*)&spikedb[0];
  int blocklen[3] = {1,1,1};
  MPI_Datatype type[3] = {MPI_INT,MPI_INT,MPI_INT};
  MPI_Type_create_struct(3,blocklen,disp,type,&spikedb_struct_mpi);
  MPI_Type_commit(&spikedb_struct_mpi);

  MPI_Bcast(spikedb, max_spikes, spikedb_struct_mpi, 0,MPI_COMM_WORLD);*/		// changing max_spikes to num_spikes crashes for no reason
  
  return rc;
}

uint64_t numScheduledEvents = 0;
void scheduleSp(tw_stime time, id_type axonID, tw_lp *lp) {
  tw_event *synapsePrime = tw_event_new(lp->gid, time, lp);
  messageData *outData = tw_event_data(synapsePrime);
  outData->axonID = axonID;
  outData->eventType = AXON_OUT;
  tw_event_send(synapsePrime);
  numScheduledEvents++;
  //printf("Just scheduled event with time %f and axon %d.\n",time,axonID);
}

double parse_start_time;
void loadSynapseSpikesFile(synapseState *s, tw_lp *lp) {
  static int time_set = 0;
  if (time_set == 0)
  {
    time_set = 1;
    parse_start_time = MPI_Wtime();
  }
  //printf("now loading up synapses\n");
  id_type myCore = s->myCore;
  int spike_index = 0;									// iterating through a million+ elements each time is very slow
  while (spikedb[spike_index].core != -1) {
    //printf("Core %d vs %u.\n",myCore,spikedb[spike_index].core);
    if (myCore == spikedb[spike_index].core) {						// this requires that spikedb is in scope of synapse.c despite it beind declared in spike_db_reader.c. kinda ugly
      //printf("Core %d found spike time %f axon %u.\n",myCore,spikedb[spike_index].time,spikedb[spike_index].axon);
      //This call schedules the input spike - for all
      //input spikes in the input spike file (that are supposed to
      //go to this core), scheduleSP needs to be called.
      //time and axid are the pulled data fields from the config file,
      // lp is the LP variable (passed in at the top of the function
      scheduleSp(spikedb[spike_index].time, spikedb[spike_index].axon, lp);
    }
    spike_index++;
  }
  double parse_end_time = MPI_Wtime();
  printf("Core %d done with input spikes with file read time %f, parse time %f, and total time %f.\n",myCore,file_read_end_time-file_read_start_time,parse_end_time-parse_start_time,parse_end_time-overall_start_time);
}

void synapse_init(synapseState *s, tw_lp *lp) {

  s->msgSent = 0;
  s->lastBigTick = 0;
  s->myCore = getCoreFromGID(lp->gid);
  //printf("running synapse init core ID is %d.\n", s->myCore);
  if (FILE_IN) {
    loadSynapseSpikesFile(s, lp);
  }

  if (DEBUG_MODE) {
    printf("Super Synapse Created - GID is %llu\n", lp->gid);
  }
}

void sendSynapseHB(synapseState *s, tw_bf *bf, messageData *M, tw_lp *lp, unsigned long count) {

  tw_event *synapseHB = tw_event_new(lp->gid, getNextSynapseHeartbeat(lp), lp);
  messageData *outData = tw_event_data(synapseHB);
  outData->synapseCounter = count - 1;
  outData->axonID = M->axonID;
  outData->localID = M->localID;
  outData->eventType = SYNAPSE_HEARTBEAT;

  tw_event_send(synapseHB);

}
void reverseSynapseHB(synapseState *s, tw_bf *bf, messageData *M, tw_lp *lp) {
  M->synapseCounter++;
}

void synapse_event(synapseState *s, tw_bf *bf, messageData *M, tw_lp *lp) {
  unsigned long randCount = lp->rng->count;

  if (M->axonID > AXONS_IN_CORE)
    tw_error(TW_LOC, "Invalid AXON value within synapse system.");

  if (M->eventType==SYNAPSE_HEARTBEAT) {
    //Heartbeat message
    if (M->synapseCounter!=0) {
      //unsigned long sc = M->synapseCounter - 1;
      sendSynapseHB(s, bf, M, lp, M->synapseCounter);
    }

    tw_lpid neuron = getNeuronGlobal(s->myCore, M->synapseCounter);
    tw_event *sout = tw_event_new(neuron, getNextEventTime(lp), lp);
    messageData *outData = tw_event_data(sout);
    outData->axonID = M->axonID;
    outData->localID = M->axonID;
    outData->eventType = SYNAPSE_OUT;
    tw_event_send(sout);
    ++s->msgSent;

  } else if (M->eventType==AXON_OUT) {
    sendSynapseHB(s, bf, M, lp, NEURONS_IN_CORE);
  }

  M->rndCallCount = lp->rng->count - randCount;

}
//	static int hasRun = 0;
//	
//	if (! hasRun) {
//		for (int i = 0; i < NEURONS_IN_CORE; i++){
//			for(int j = 0; j < NEURONS_IN_CORE; j ++){
//				s->connectionGrid[i][j] = tw_getlp(i)->cur_state->synapticConnectivity[j];
//			}
//		}
//	}
//	
//	long rc = lp->rng->count;
//	//run a loop that calls the "forward event handler" of each neuron in this core:
//	tw_lp * cNeuron;
//	
//	/** @TODO: See if localID is still needed ! */
//	
//	//Create a "message" that is "sent" to this neuron
//	messageData *outM = (messageData *) tw_calloc(TW_LOC, "Synapse", sizeof(messageData), 1);
//	//set up the message for the neurons
//	outM->axonID = M->axonID;
//	outM->eventType = SYNAPSE_OUT;
//	outM->localID = M->axonID;
//	
//	6
//	id_type axonID = M->axonID;
//	for(int i = 0; i < AXONS_IN_CORE; i ++){
//		//check to see if the neuron is connected to the axon that sent the message
//		//get the neuron GID
//		tw_lpid nid = getNeuronGlobal(s->myCore,i);
//		//get the LP @todo: look at changing this to direct array access
//		cNeuron = tw_getlp(nid);
//		
//synapse_pre_run

//		
//		//if(cNeuron->connectionGrid[axonID] != 0){
//			
//
//
//			//call the neuron's forward event handler
//			/** @todo: This is a bandaid until proper reverse computation can be determined. */
//			cNeuron->type->event(cNeuron->cur_state,&s->neuronBF[i],outM,cNeuron);
//			s->randCount[i] = outM->rndCallCount;
//			
//		//}
//
//	}



void synapse_reverse(synapseState *s, tw_bf *bf, messageData *M, tw_lp *lp) {

  if (M->eventType==AXON_OUT) {

  } else if (M->eventType==SYNAPSE_HEARTBEAT) {
    --s->msgSent;
  }
  unsigned long count = M->rndCallCount;
  while (count--) {
    tw_rand_reverse_unif(lp->rng);
  }
}

void synapse_final(synapseState *s, tw_lp *lp) {
  //do some stats here if needed.
  static int announce = 0;
  if (!announce && g_tw_mynode==0) {
    tw_printf(TW_LOC, "Scheduled %llu events from file.\n", numScheduledEvents);
    announce = 1;
  }
  if (g_tw_synchronization_protocol==OPTIMISTIC_DEBUG) {
    char *shdr = "Synapse Error\n";

    if (s->msgSent!=0) {
      printf("%s ", shdr);
      char *m = "Message Sent Val ->";
      //debugMsg(m, s->msgSent);
    }
  }

}
void synapse_pre_run(synapseState *s, tw_lp *lp) {
  static int should_close = 1;
  if (should_close) {
    //closeSpikeFile();
    should_close = 0;
  }
}

