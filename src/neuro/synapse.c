#include "synapse.h"

struct spikedb_struct{							// Could alternatively use NeuroSpike structure from IOStack.h if time element is renabled as int
  int time;
  int core;
  int axon;
};

spikedb_struct* spikedb;

uint64_t numScheduledEvents = 0;
void scheduleSp(tw_stime time, id_type axonID, tw_lp *lp) {

  tw_event *synapsePrime = tw_event_new(lp->gid, time, lp);
  messageData *outData = tw_event_data(synapsePrime);
  outData->axonID = axonID;
  outData->eventType = AXON_OUT;
  tw_event_send(synapsePrime);
  numScheduledEvents++;
}

void loadSynapseSpikesFile(synapseState *s, tw_lp *lp) {
  id_type myCore = s->myCore;
  int spike_index = 0;
  uint32_t time;
  uint32_t axid;

  while (spikedb[spike_index].core != -1)) {
    if (myCore == spikedb[spike_index].core) {						// this requires that spikedb is in scope of synapse.c despite it beind declared in spike_db_reader.c. kinda ugly
      time = spikedb[spike_index].time;
      axid = spikedb[spike_index].axon;

      //This call schedules the input spike - for all
      //input spikes in the input spike file (that are supposed to
      //go to this core), scheduleSP needs to be called.
      //time and axid are the pulled data fields from the config file,
      // lp is the LP variable (passed in at the top of the function
      scheduleSp(time, axid, lp);
    }
    spike_index++;
  }
  free(spikedb);
}

void synapse_init(synapseState *s, tw_lp *lp) {
  s->msgSent = 0;
  s->lastBigTick = 0;
  s->myCore = getCoreFromGID(lp->gid);

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
//

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
/*void synapse_pre_run(synapseState *s, tw_lp *lp) {
  static int should_close = 1;
  if (should_close) {
    closeSpikeFile();
    should_close = 0;
  }
}*/

