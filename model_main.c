	//
	//  model_main.c
	//  ROSS_TOP
	//
	//  Created by Mark Plagge on 6/17/15.
	//
	//

#include "model_main.h"

	// tw_lptype model_lps[] = {
	//    {

	//     (init_f)neuron_init, (pre_run_f)pre_run, (event_f)neuron_event,
	//     (revent_f)neuron_reverse, (final_f)neuron_final, (map_f)getPEFromGID,
	//     sizeof(neuronState)},
	//    {

	//     (init_f)synapse_init, (pre_run_f)pre_run, (event_f)synapse_event,
	//     (revent_f)synapse_reverse, (final_f)synapse_final, (map_f)getPEFromGID,
	//     sizeof(synapseState)},

	//    {(init_f)axon_init, (pre_run_f)pre_run, (event_f)axon_event,
	//     (revent_f)axon_reverse, (final_f)axon_final, (map_f)getPEFromGID,
	//     sizeof(axonState)},
	//    {0}

	//};
tw_lptype model_lps[] = {
	{
	(init_f)axon_init,
	(pre_run_f)pre_run,
	(event_f)axon_event,
	(revent_f)axon_reverse,
	(final_f)axon_final,
	(map_f)lGidToPE,
	sizeof(axonState) },
	{
	(init_f)synapse_init, (pre_run_f)pre_run,
	(event_f)synapse_event,
	(revent_f)synapse_reverse,
	(final_f)synapse_final,
	(map_f)lGidToPE, sizeof(synapseState)
	},
	{
	(init_f)neuron_init,
	(pre_run_f)pre_run,
	(event_f)neuron_event,
	(revent_f)neuron_reverse,
	(final_f)neuron_final,
	(map_f)lGidToPE, sizeof(neuronState)
	}
	,
	{ 0 } };



void saveNetwork(neuronState *nglobal,tw_lpid gid){
	static bool created = false;
    //copy-pasted code band-aid
    neuronState *n = nglobal;

	char* filenamet = "neuron_output-rank-";

	char* filename = calloc(sizeof(char),128); //magic number - max size of filename
	sprintf(filename,"%s%li.csv",filenamet,g_tw_mynode);

	char* wts = "neuron_weight_table-rank-";
	char* wtfn = calloc(sizeof(char), 256);

	sprintf(wtfn,"%s%llu.csv",wts,g_tw_mynode);


	if(created == false) {
		FILE *f = fopen("neuron_output_headers.csv","w");
		fprintf(f,"Core,NeuronLocal,NeuronGID,AxonLocalID,AxonCore,AxonGID\n");
		created = true;
		fclose(f);
        f = fopen("neuron_weight_table_headers.csv","w");
        fprintf(f,"NeuronCORE,NeuronLocal,Axon(synapse)Local,Axon(synapse)Global,Connectivity,Weight");
        fclose(f);
	}
	FILE *f = fopen(filename,"a");

	fprintf(f,"%llu,%llu,%lli,%i,%llu,%lli\n",n->myCoreID,n->myLocalID,gid,n->dendriteLocal,n->dendriteCore,n->dendriteGlobalDest);
	fclose(f);


	


	f = fopen(wtfn,"a");

	//fprintf(f,"Neuron:%llu-%llu weight table\n",nglobal->myLocalID,nglobal->myCoreID);
	//fprintf(f,"Axon(synapse)Local,Axon(synapse)Global,Connectivity,Weight\n");
	for(int axon = 0; axon < AXONS_IN_CORE; axon ++) {
		tw_lpid axGID = lGetAxonFromNeu(nglobal->myCoreID,axon);
		int conn = nglobal->synapticConnectivity[axon];
		int weight = nglobal->axonTypes[axon];
		weight = nglobal->synapticWeight[weight];
		fprintf(f, "%llu,%llu,%i,%lli,%i,%i\n",n->myCoreID,n->myLocalID,axon,axGID,conn,weight );

	}
	fclose(f);
	free(wtfn);
	free(filename);




}
int main(int argc, char *argv[])
{
		///VALIDATION SETUP


	tw_opt_add(app_opt);
	//g_tw_gvt_interval = 512;
	tw_init(&argc, &argv);

		//	// set up core sizes.
	AXONS_IN_CORE = NEURONS_IN_CORE;
	SYNAPSES_IN_CORE = (NEURONS_IN_CORE * AXONS_IN_CORE);
	CORE_SIZE = SYNAPSES_IN_CORE + NEURONS_IN_CORE + AXONS_IN_CORE;
	SIM_SIZE = (CORE_SIZE * CORES_IN_SIM);// / tw_nnodes();
	tnMapping = LLINEAR;


	/** g_tw_nlp set here to CORE_SIZE.
	 * @todo check accuracy of this
	 * */
	LPS_PER_PE = SIM_SIZE / tw_nnodes();
	LP_PER_KP = LPS_PER_PE / g_tw_nkp;
		//
	//g_tw_events_per_pe = LPS_PER_PE + 50000;//eventAlloc * 9048;//g_tw_nlp * eventAlloc + 4048;
	g_tw_events_per_pe = 65536;
	//	///@todo enable custom mapping with these smaller LPs.

	g_tw_lp_typemap = &tn_linear_map;
	g_tw_lp_types = model_lps;


		//if (tnMapping == LLINEAR) {
		//g_tw_mapping = CUSTOM;
		//g_tw_lp_types = model_lps;
		//g_tw_lp_typemap = &tn_linear_map;
		//g_tw_lp_typemap = &clLpTypeMapper;

		//g_tw_custom_initial_mapping = &clMap;
		//g_tw_custom_lp_global_to_local_map = &clLocalFromGlobal;
		// g_tw_1types = model_lps;
		//g_tw_lp_typemap = &lpTypeMapper;
		//g_tw_lp_typemap = &clLpTypeMapper;
		//g_tw_custom_initial_mapping = &clMap;
		//g_tw_custom_lp_global_to_local_map = &clLocalFromGlobal;
		//
		//	}
		//
	g_tw_lookahead = LH_VAL;
		//	// g_tw_clock_rate = CL_VAL;
	g_tw_nlp = LPS_PER_PE;
		//
		//	g_tw_memory_nqueues = 16;  // give at least 16 memory queue event
		//
	tw_define_lps(LPS_PER_PE, sizeof(Msg_Data));
	tw_lp_setup_types();
		//
		//	///@todo do we need a custom lookahedad parameter?
		//
		//sim_size is
		//	// scatterMap();
		//	// createLPs();
		//
		//	// printf("\nCreated %i ax, %i ne, %i se", a_created, n_created, s_created);
	if (g_tw_mynode == 0) {
		displayModelSettings();
	}
		//		//testTiming();
		//
		//
	validation = PHAS_VAL || DEPOLAR_VAL || TONIC_BURST_VAL || TONIC_SPK_VAL;



	tw_run();
		//	// Stats Collection ************************************************************************************88

	tw_statistics s = statsOut();
	csv_model_stats(s);

	tw_end();
		//

	return (0);
}
int csv_model_stats(tw_statistics s){

		//
		//printf("\n\n %i", s.s_pe_event_ties);
		//tabular data:
		//NP  - CORES - Neurons per core - Net Events - Rollbacks - Running Time	- SOP

		//Neuron Specific Stats:
	totalSOPS = 0;
	totalSynapses = 0;
	stat_type totalNFire = 0;
	MPI_Reduce(&neuronSOPS, &totalSOPS, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&synapseEvents, &totalSynapses, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&fireCount, &totalNFire, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	if (g_tw_mynode == 0) {  // master node for outputting stats.
		printf("\n ------ TN Benchmark Stats ------- \n");
		printf("Total SOP(integrate and/or fire) operations: %llu\n", totalSOPS);
		printf("Total spikes fired by all neurons: %llu\n", totalNFire);
		printf("This PE's SOP: %llu\n", neuronSOPS);
		printf("Total Synapse MSGs sent: %llu\n", totalSynapses);
		printf("\n\n");
		printf("Nodes,CORES,Neurons/Core,Net Events,Rollbacks,Run Time,Total SOP,Threshold Min,Threshold Max"
			   ",NegativeThresholdMin,NegativeThresholdMax,Synapse Weight Min,Synapse Weight Max,EvtTies\n");
		printf("%u,%i,%i,%llu,%llu,%f,%llu,", tw_nnodes(), CORES_IN_SIM, NEURONS_IN_CORE, s.s_net_events, s.s_rollback, s.s_max_run_time, totalSOPS);
		printf("%u,"
			   "%u,"
			   "%u,"
			   "%u,"
			   "%d,"
			   "%d,"
			   "%llu\n", THRESHOLD_MIN, THRESHOLD_MAX, NEG_THRESHOLD_MIN, NEG_THRESHOLD_MAX, SYNAPSE_WEIGHT_MIN, SYNAPSE_WEIGHT_MAX, s.s_pe_event_ties);
		if (BULK_MODE) {
			fprintf(stderr, "%u,%i,%i,%llu,%llu,%f,%llu,%u,%u,%u,%u,%u,%u,", tw_nnodes(), CORES_IN_SIM,
					NEURONS_IN_CORE, s.s_net_events, s.s_rollback, s.s_max_run_time, totalSOPS, THRESHOLD_MIN, THRESHOLD_MAX,
					NEG_THRESHOLD_MIN, NEG_THRESHOLD_MAX, SYNAPSE_WEIGHT_MIN, SYNAPSE_WEIGHT_MAX);
			fprintf(stderr, "%llu", s.s_pe_event_ties);
		}

			//save these stats for records:

		struct supernStats *m = calloc(sizeof(struct supernStats), 1);
		m->neuronSpikes =totalNFire;
		m->npe = g_tw_npe;
		m->runtime = s.s_max_run_time;
		m->totalTime = s.s_total;
		m->SOP = totalSOPS;
		m->totalSynapseMsgs = totalSynapses;
			//Generate a unique CSV file name. Based on Cores, NPE, and the time.
		tw_wtime t;
		tw_wall_now(&t);

		char* output;
		asprintf(&output, "snb_run_%lu_np%lu_cores%i.csv",t.tv_sec,g_tw_npe,CORES_IN_SIM);

		int rv =   write_csv(m, output);
		free(m);
		return rv;
	}
	return -1;

}
tw_statistics statsOut()
{
	tw_pe *me = g_tw_pe[0];
	tw_statistics s;
	tw_pe *pe;
	tw_kp *kp;
	tw_lp *lp = NULL;


	int i;

	size_t m_alloc, m_waste;

	if (me != g_tw_pe[0]) {
		return s;
	}

	if (0 == g_tw_sim_started) {
		return s;
	}

	tw_calloc_stats(&m_alloc, &m_waste);
	bzero(&s, sizeof(s));

	for (pe = NULL; (pe = tw_pe_next(pe)); )
		{
		tw_wtime rt;

		tw_wall_sub(&rt, &pe->end_time, &pe->start_time);

		s.s_max_run_time = ROSS_MAX(s.s_max_run_time, tw_wall_to_double(&rt));
		s.s_nevent_abort += pe->stats.s_nevent_abort;
		s.s_pq_qsize += tw_pq_get_size(me->pq);

		s.s_nsend_net_remote += pe->stats.s_nsend_net_remote;
		s.s_nsend_loc_remote += pe->stats.s_nsend_loc_remote;

		s.s_nsend_network += pe->stats.s_nsend_network;
		s.s_nread_network += pe->stats.s_nread_network;
		s.s_nsend_remote_rb += pe->stats.s_nsend_remote_rb;

		s.s_total += pe->stats.s_total;
		s.s_net_read += pe->stats.s_net_read;
		s.s_gvt += pe->stats.s_gvt;
		s.s_fossil_collect += pe->stats.s_fossil_collect;
		s.s_event_abort += pe->stats.s_event_abort;
		s.s_event_process += pe->stats.s_event_process;
		s.s_pq += pe->stats.s_pq;
		s.s_rollback += pe->stats.s_rollback;
		s.s_cancel_q += pe->stats.s_cancel_q;
		s.s_pe_event_ties += pe->stats.s_pe_event_ties;
		s.s_min_detected_offset = g_tw_min_detected_offset;
		s.s_avl += pe->stats.s_avl;
		s.s_buddy += pe->stats.s_buddy;
		s.s_lz4 += pe->stats.s_lz4;
		s.s_events_past_end += pe->stats.s_events_past_end;

		for (i = 0; i < g_tw_nkp; i++)
			{
			kp = tw_getkp(i);
			s.s_nevent_processed += kp->s_nevent_processed;
			s.s_e_rbs += kp->s_e_rbs;
			s.s_rb_total += kp->s_rb_total;
			s.s_rb_secondary += kp->s_rb_secondary;
			}

		for (i = 0; i < g_tw_nlp; i++)
			{
			lp = tw_getlp(i);
			if (lp->type->final) {
				(*lp->type->final)(lp->cur_state, lp);
			}
			}
		}

	s.s_fc_attempts = g_tw_fossil_attempts;
	s.s_net_events = s.s_nevent_processed - s.s_e_rbs;
	s.s_rb_primary = s.s_rb_total - s.s_rb_secondary;

	s = *(tw_net_statistics(me, &s));



		//
	return s;

}


int write_csv(struct supernStats *stats, char const *fileName){
	FILE *f = fopen(fileName, "a");
	if (f == NULL) return -1;
	fprintf(f,"\"npe\",\"total_sop\",\"neuron_spikes\",\"total_synapse_msgs\",\"runtime\",\"total\"\n");
	fprintf(f, "%u,%llu,%llu,%llu,%f,%f \n", stats->npe, stats->SOP,
			stats->neuronSpikes, stats->totalSynapseMsgs,
			stats->runtime, stats->totalTime);


	fclose(f);
	return 0;
}


	///
	/// \details createLPs currently assigns a core's worth of LPs to the PE.
	/// @todo need to create better mappingassert(lp->pe->delta_buffer[0] && "increase --buddy_size argument!");.
	///
void createLPs()
{
		//tw_define_lps(CORE_SIZE, sizeof(Msg_Data));
	int neurons = 0;
	int synapses = 0;
	int axons = 0;
	int soff = AXONS_IN_CORE + SYNAPSES_IN_CORE;
		// int noff = CORE_SIZE - NEURONS_IN_CORE;
	for (tw_lpid i = 0; i < g_tw_nlp; i++)
		{
		if (i < AXONS_IN_CORE) {
			tw_lp_settype(i, &model_lps[2]);
			axons++;
		} else if (i < soff) {
			tw_lp_settype(i, &model_lps[1]);
			synapses++;
		} else {
			tw_lp_settype(i, &model_lps[0]);
			neurons++;
		}
		}
		// printf("Created %i axons, %i synapses,  %i neurons", axons, synapses,
		// neurons);
}

void createDisconnectedNeuron(neuronState *s, tw_lp *lp){
	bool synapticConnectivity[NEURONS_IN_CORE];
	short G_i[NEURONS_IN_CORE];
	short sigma[4];
	short S[4];
	bool b[4];

	bool epsilon = 0;
	bool sigma_l = 0;
	short lambda = 0;
	bool c = false;
	short TM = 0;

	short VR = 0;

	short sigmaVR = 1;
	short gamma = 0;

	bool kappa = false;
	int signalDelay = 0;
		//per synapse weight / connectivity gen:
	for(int i = 0; i < NEURONS_IN_CORE; i ++) {
			//s->synapticConnectivity[i] = tw_rand_integer(lp->rng, 0, 1);
		s->axonTypes[i] = 0;
		G_i[i] = 0; //tw_rand_integer(lp->rng, 0, 0);
		synapticConnectivity[i] = 0;

			//synapticConnectivity[i] = tw_rand_integer(lp->rng, 0, 1);

	}
	weight_type alpha = 50;
	weight_type beta = -1;
	S[0] = 0;
	b[0] = 0;
	sigma[0] = 0;
	initNeuron(lGetCoreFromGID(lp->gid), lGetNeuNumLocal(lp->gid), synapticConnectivity, G_i, sigma, S, b, epsilon, sigma_l, lambda, c, alpha, beta, TM, VR, sigmaVR, gamma, kappa, s, signalDelay,0,0);
	s->dendriteCore = tw_rand_integer(lp->rng, 0, CORES_IN_SIM - 1);
	s->dendriteLocal = tw_rand_integer(lp->rng, 0, AXONS_IN_CORE - 1);

		//     if (tnMapping == LLINEAR) {
	s->dendriteGlobalDest = lGetAxonFromNeu(s->dendriteCore, s->dendriteLocal);
		//     }
		//     else {
		//     s->dendriteGlobalDest = getAxonGlobal(s->dendriteCore, s->dendriteLocal);
		//     }
		//    s->neuronTypeDesc = tw_calloc(TW_LOC,"", sizeof(char), 5);
	s->neuronTypeDesc = "DISC.";
}


	// neuron gen function helpers
void createSimpleNeuron(neuronState *s, tw_lp *lp){
		//Rewrote this function to have a series of variables that are easier to read.
		//Since init time is not so important, readability wins here.
		//AutoGenerated test neurons:
	bool synapticConnectivity[NEURONS_IN_CORE];
	short G_i[NEURONS_IN_CORE];
	short sigma[4];
	short S[4];
	bool b[4];
	bool epsilon = 0;
	bool sigma_l = 0;
	short lambda = -1;
	bool c = false;
	short TM = 0;
	short VR = 1;
	short sigmaVR = 1;
	short gamma = 0;
	bool kappa = false;
	int signalDelay = 1;//tw_rand_integer(lp->rng, 0,5);



	for(int i = 0; i < NEURONS_IN_CORE; i ++) {
			//s->synapticConnectivity[i] = tw_rand_integer(lp->rng, 0, 1);
		s->axonTypes[i] = 1;
		G_i[i] = 1;
		synapticConnectivity[i] = 0;
			//synapticConnectivity[i] = tw_rand_integer(lp->rng, 0, 1)
	}

	synapticConnectivity[lGetNeuNumLocal(lp->gid)] = 1;
		G_i[lGetNeuNumLocal(lp->gid)] = 0; //set the 1->1 mapping type to 0
		//(creates an "ident. matrix" of neurons.
	for(int i = 0; i < 4; i ++){
		//int ri = tw_rand_integer(lp->rng, -1, 0);
		//unsigned int mk = tw_rand_integer(lp->rng, 0, 1);

		//sigma[i] = (!ri * 1) + (-1 & ri))
		//sigma[i] = (mk ^ (mk - 1)) * 1;
		sigma[i] = 1;
		S[i] = 1;
		b[i] = 0;
	}
		S[0] = 2;//(short) tw_rand_binomial(lp->rng,10,.5);
		S[1] = 0;
		S[2] = 0;//((short) tw_rand_binomial(lp->rng,5, .2) * -1);
		S[3] = 0;


	//weight_type alpha = tw_rand_integer(lp->rng, THRESHOLD_MIN, THRESHOLD_MAX);
	//weight_type beta = tw_rand_integer(lp->rng, (NEG_THRESH_SIGN * NEG_THRESHOLD_MIN), NEG_THRESHOLD_MAX);
	weight_type alpha = 1;
	weight_type beta = -1;

		initNeuronEncodedRV(lGetCoreFromGID(lp->gid), lGetNeuNumLocal(lp->gid), synapticConnectivity,
			   G_i, sigma, S, b, epsilon, sigma_l, lambda, c, alpha, beta,
			   TM, VR, sigmaVR, gamma, kappa, s, signalDelay,0,0);
		//we re-define the destination axons here, rather than use the constructor.

	float remoteCoreProbability = .905;
	
	//This neuron's core is X. There is a 90.5% chance that my destination will be X - and a 10% chance it will be a different core.
	if(tw_rand_unif(lp->rng) < remoteCoreProbability){
//		long dendriteCore = s->myCoreID;
//		dendriteCore = tw_rand_integer(lp->rng, 0, CORES_IN_SIM - 1);	
		s->dendriteCore =  tw_rand_integer(lp->rng, 0, CORES_IN_SIM - 1);
	}else {
		s->dendriteCore = s->myCoreID; //local connection.
	}

	/**@note This random setup will create neurons that have an even chance of getting an axon inside thier own core
	 * vs an external core. The paper actually capped this value at something like 20%. @todo - make this match the
	 * paper if performance is slow. * */
	//s->dendriteCore = tw_rand_integer(lp->rng, 0, CORES_IN_SIM - 1);
	s->dendriteLocal = s->myLocalID;// tw_rand_integer(lp->rng, 0, AXONS_IN_CORE - 1);
		//     if (tnMapping == LLINEAR) {
	s->dendriteGlobalDest = lGetAxonFromNeu(s->dendriteCore, s->dendriteLocal);
	
		if (s->dendriteGlobalDest != lGetAxonFromNeu(s->dendriteCore,s->dendriteLocal))
		{

			tw_error(TW_LOC,"Invalid Axon Destination - neuron %i set gid %i",lp->gid,s->dendriteGlobalDest);
		}
		//     }
		//     else {
		//     s->dendriteGlobalDest = getAxonGlobal(s->dendriteCore, s->dendriteLocal);
		//     }



}
/** creates a neuron - uses global vars for special construction setups. Generally,
 *  this is used for neuron validation, since the model construction with validation
 *  assumes that cores are identical. When input is handled, file pointers will be
 *  used to manage per process neuron creation. */

/** announced - simple global flag that outputs a neuron creation once. Debug tracer */
bool annouced = false;

bool pc = false;


void neuron_init(neuronState *s, tw_lp *lp) {
    static int pairedNeurons = 0;
	s->neuronTypeDesc = "SIMPLE";
	if(DEBUG_MODE && ! annouced)
		printf("Creating neurons\n");

	if(PHAS_VAL) {
		if(!pc){
			crPhasic(s, lp);
			pc = true;
		}
		else {
			createDisconnectedNeuron(s, lp);
		}

    } else if(TONIC_BURST_VAL) {
        if(pairedNeurons < 2) {
            crTonicBursting(s, lp);
            pairedNeurons ++;
        }
        else {
            createDisconnectedNeuron(s, lp);
        }
    } else {
		createSimpleNeuron(s, lp);
	}
		//createDisconnectedNeuron(s, lp);
	annouced = true;



	if (DEBUG_MODE) {
		printf("Neuron type %s, num: %llu checking in with GID %llu and dest %llu \n", s->neuronTypeDesc, s->myLocalID, lp->gid, s->dendriteGlobalDest);
	}
	if(SAVE_NEURON_OUTS) {
		saveNetwork(s,lp->gid);
	}
}


void setSynapseWeight(neuronState *s, tw_lp *lp, int synapseID)
{
}

neuEvtLog * nlset(neuronState *s, tw_lp *lp) {
	neuEvtLog * log = (neuEvtLog *) calloc(sizeof(neuEvtLog), 1);
	log->cbt = getCurrentBigTick(tw_now(lp));
	log->cid = s->myCoreID;
	log->nid = s->myLocalID;
	log->timestamp = tw_now(lp);
	log->next = NULL;
	return log;
}
	//!Validation variable -
	///!@TODO: Move this to a better location. Only using this for sequential sim atm.
void neuron_event(neuronState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{


	long start_count = lp->rng->count;
		//if delta is on or basic mode is on, take a snapshot for delta encoding
	if (TW_DELTA &&
		(g_tw_synchronization_protocol == OPTIMISTIC || g_tw_synchronization_protocol == OPTIMISTIC_DEBUG)) {
		tw_snapshot(lp, lp->type->state_sz );
			//printf("Neuron snapshot saved");
	}

	if ((validation || SAVE_MEMBRANE_POTS)   && s->neuronTypeDesc[0]!='D') { //&& M->eventType == NEURON_HEARTBEAT

		//if(s->neuronTypeDesc[0] == 'P' && s->neuronTypeDesc[1] == 'H')
		//char* x = s->neuronTypeDesc;
		//int xu = 3;
		saveValidationData(s->myLocalID, s->myCoreID, tw_now(lp), s->membranePotential);
	}
	bool fired = neuronReceiveMessage(s, M, lp,CV);//#fired = (g_tw_synchronization_protocol == SEQUENTIAL || g_tw_synchronization_protocol==CONSERVATIVE) && fired;

		if ((SAVE_SPIKE_EVTS || validation) && fired == true){
			if (nlog == NULL) {
				nlog = nlset(s, lp);
			}
			else {
				addEntry(nlset(s,lp), nlog, getCurrentBigTick(tw_now(lp)));
			}
		}

	




		//basic mode removes leak and stochastic reverse threshold functions.
		//	if (BASIC_SOP) {
		//		neuronReceiveMessageBasic(s, tw_now(lp), M, lp);
		//	}else {
		//		neuronReceiveMessage(s, tw_now(lp), M, lp);
		//}

		//again, only take the delta in basic neuron mode or in delta mode.
	if (TW_DELTA &&
		(g_tw_synchronization_protocol == OPTIMISTIC || g_tw_synchronization_protocol == OPTIMISTIC_DEBUG)) {
		tw_snapshot_delta(lp, lp->type->state_sz);
	}
	M->rndCallCount = lp->rng->count - start_count;
}


void neuron_reverse(neuronState *s, tw_bf *CV, Msg_Data *MCV, tw_lp *lp)
{
	long count = MCV->rndCallCount;

	if (TW_DELTA) {
			//tw_snapshot_restore(lp, lp->type->state_sz, lp->pe->cur_event->delta_buddy, lp->pe->cur_event->delta_size);

	}
	else { //ReverseState is needed when not using delta encoding. Since basic mode implies delta, this only runs when delta is off and neurons are in normal sim mode.
		neuronReverseState(s,CV,MCV,lp);
			//neuronReverseState()
	}

		//Roll Back random calls
	while (count--)
		{
		tw_rand_reverse_unif(lp->rng);
		}

}


void neuron_final(neuronState *s, tw_lp *lp)
{
	neuronSOPS += s->SOPSCount;
		//printf("neuron %i has %i SOPS \n", lp->gid, s->SOPSCount);
	fireCount += s->fireCount;
	if(nlog != NULL){ //nlog should always be not null if we want debugging (the flag allows nlog to be created)
		saveLog(nlog, "seq_neuron_spike_log.csv");
			//saveLog(nlog, fn);
	}
	if(DEBUG_MODE) {
		printf("neuron num %llu was type %s\n", s->myLocalID, s->neuronTypeDesc);
	}
}


	// synapse function

void synapse_init(synapseState *s, tw_lp *lp)
{
	s->destNeuron = lGetNeuronFromSyn(lp->gid);
	s->destSynapse = lGetNextSynFromSyn(lp->gid);
	s->mySynapseNum = lGetSynNumLocal(lp->gid);
		//if (tnMapping == LLINEAR) {
		//s->destNeuron = clGetNeuronFromSynapse(lp->gid);
		//s->destSynapse = 0;
		//s->destSynapse = clGetSynapseFromSynapse(lp->gid);
		//		s->mySynapseNum = clGetSynapseFromSynapse\(lp->gid);

		//GlobalID g;
		//g.raw = lp->gid;
		//s->mySynapseNum = g.local;

		//}

	/**
	 *  @todo Fix this - there are some logic errors here.
	 */
		//	else {
		//		s->destNeuron = getNeuronFromSynapse(lp->gid);
		//		s->destSynapse = 0;
		//		int16_t local = LOCAL(lp->gid);
		//		s->mySynapseNum = ISIDE(local);
		//
		//		//@todo make this a matrix map - still have linear style of mapping!!!!!
		//		if (ISIDE(local) == NEURONS_IN_CORE) {
		//			s->destSynapse = getSynapseFromSynapse(lp->gid);
		//		}
		//	}

	s->msgSent = 0;
		//	if (DEBUG_MODE) {
		//		printf("Synapse %i checking in with GID %llu and n-dest %llu, s-dest %llu on "
		//		    "PE %lu , CPE %lu\n", s->mySynapseNum, lp->gid, s->destNeuron, s->destSynapse, lp->pe->id,
		//		    lGidToPE(lp->gid));
		//	}
}

void testSynapse(synapseState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp) {
	if (s->destSynapse != 0) {
			// generate event to send to next synapse
			//s->msgSent++;
			//CV->c0 = 1;
		tw_event *axe = tw_event_new(s->destSynapse, getNextEventTime(lp), lp);
		Msg_Data *data = (Msg_Data *)tw_event_data(axe);
		data->eventType = SYNAPSE_OUT;
		data->localID = lp->gid;
		data->axonID = M->axonID;

		tw_event_send(axe);
	}


}

void synapse_event(synapseState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
		//testSynapse(s, CV, M, lp);

	long rc = lp->rng->count;
		//s->destNeuron = clGetNeuronFromSynapse(lp->gid);
		//s->destSynapse = clGetSynapseFromSynapse(lp->gid);
		//s->destNeuron = lGetNeuronFromSyn(lp->gid);
		//s->destSynapse = lGetNextSynFromSyn(lp->gid);
		//*(int *)CV = (int)0;
		// printf("Synapse rcvd msg\n");
	if (s->destSynapse != 0) {
			// generate event to send to next synapse
			//s->msgSent++;
			//CV->c0 = 1;
		tw_event *axe = tw_event_new(s->destSynapse, getNextEventTime(lp), lp);
		Msg_Data *data = (Msg_Data *)tw_event_data(axe);
		data->eventType = SYNAPSE_OUT;
		data->localID = lp->gid;
		data->axonID = M->axonID;

		tw_event_send(axe);
	}
		// generate event to send to neuron.
	s->msgSent++;
		//CV->c1 = 1;

	tw_event *axe = tw_event_new(s->destNeuron, getNextEventTime(lp), lp);
	Msg_Data *data = (Msg_Data *)tw_event_data(axe);
	data->eventType = SYNAPSE_OUT;
	data->localID = s->mySynapseNum;
	data->axonID = M->axonID;
	tw_event_send(axe);
	M->rndCallCount = lp->rng->count - rc;
}


void synapse_reverse(synapseState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
	s->msgSent --;

		//s->msgSent --;

	long count = M->rndCallCount;
	while (count--)
		{
		tw_rand_reverse_unif(lp->rng);
		}

}


void synapse_final(synapseState *s, tw_lp *lp)
{
	synapseEvents += s->msgSent;
}


	// Axon function.
id_type curAxon = 0;

int specAxons = 0;
void axon_init(axonState *s, tw_lp *lp)
{
    //TODO: Maybe switch this to a switch/case later, since it's going to get
    //big.
	s->axtype = "NORM";
	if(PHAS_VAL) {//one phasic axon:
		if (specAxons == 0){
			crPhasicAxon(s, lp);
			specAxons ++;
		}

		else {
			s->sendMsgCount = 0;
			s->axonID = lGetAxeNumLocal(lp->gid);
			s->destSynapse = lGetSynFromAxon(lp->gid);
		}

    }else if(TONIC_BURST_VAL){

		crTonicBurstingAxon(s, lp);
		specAxons ++;

    }
	else {
		s->sendMsgCount = 0;
		s->axonID = lGetAxeNumLocal(lp->gid);
		s->destSynapse = lGetSynFromAxon(lp->gid);
		tw_stime r = getNextEventTime(lp);
		tw_event *axe = tw_event_new(lp->gid, r, lp);
		Msg_Data *data = (Msg_Data *)tw_event_data(axe);
		data->eventType = AXON_OUT;
		data->axonID = s->axonID;
		tw_event_send(axe);

			//if (tnMapping == LLINEAR) {
		//	s->destSynapse = clgetSynapseFromAxon(lp->gid);
		//GlobalID g;
		//g.raw = lp->gid;
		//s->axonID = g.local;

		//		s->axonID = lGetAxeNumLocal(lp->gid);
		//	} else {
		//		s->destSynapse = getSynapseFromAxon(lp->gid);
		//		id_type l = LOCAL(lp->gid);
		//
		//		// tw_printf(TW_LOC, "Axon %i sending message to GID %llu", JSIDE(l),
		//		// s->destSynapse );
		//		if (DEBUG_MODE) {
		//			printf("Axon %i checking in (custom) with gid %llu and dest synapse %llu\n ", JSIDE(l), lp->gid,
		//			    s->destSynapse);
		//		}
		//	}
		//printf("Axon GIDVAL:(%i,%i), INTVAL(%i),  init message sending to gid %i ...\n",g.local, g.core, s->axonID,lp->gid);

	}



	if (DEBUG_MODE) {

		printf("Axon type - %s, #%llu checking in with dest synapse %llu\n",s->axtype, lp->gid, s->destSynapse);
	}
		//printf("message ready at %f",r);
}


void axon_event(axonState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
		// send a message to the attached synapse

	long rc = lp->rng->count;

	tw_event *axe = tw_event_new(s->destSynapse, getNextEventTime(lp), lp);
	Msg_Data *data = (Msg_Data *)tw_event_data(axe);

	data->localID = lp->gid;
	data->eventType = AXON_OUT;
	data->axonID = s->axonID;
		//printf("Axon sending message to synapse %zu\n", s->destSynapse);
	tw_event_send(axe);
	s->sendMsgCount ++;
	M->rndCallCount = lp->rng->count - rc;
}


void axon_reverse(axonState *s, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
	if (DEBUG_MODE) {
		printf("axe reverse \n");
	}
	s->sendMsgCount--;


	long count = M->rndCallCount;
	while (count--)
		{
		tw_rand_reverse_unif(lp->rng);
		}
}


void axon_final(axonState *s, tw_lp *lp)
{
}


void pre_run()
{
}


void displayModelSettings()
{
	for (int i = 0; i < 30; i++)
		{
		printf("*");
		}
	double cores_per_node = CORES_IN_SIM / tw_nnodes() ;
	printf("\n");
	char *sopMode = BASIC_SOP ? "simplified Neuron Model" : "normal TN Neuron Model";
	printf("* \tNeurons set to %s.\n", sopMode);
	printf("* \t %i Neurons per core, %i cores in sim.\n", NEURONS_IN_CORE, CORES_IN_SIM);
	printf("* \t %f cores per PE, giving %llu LPs per pe.\n", cores_per_node, g_tw_nlp);
	printf("* \t Neuron stats:\n");
	printf("* \tCalculated sim_size is %lu\n", SIM_SIZE);

		//printf("%-10s", "title");


		//printf("* \tTiming - Big tick occurs at %f\n", getNextBigTick(0));

	for (int i = 0; i < 30; i++)
		{
		printf("*");
		}
	printf("\n");
}

	/**
	 * Creates a network that has an average weight set by B_TH_MIN and B_TH_MAX.
	 * Neurons are simple reset type neurons - reset to 0.
	 * Leak is off by default, controlled with B_LEAK_ON. Leak is negative for this sim.
	 * Leak params are tuned by B_LEAK_WEIGHT.and B_LEAK_MAX and B_LEAK_MIN.
	 * Negative reset threshold is set to B_NEG_THRESHOLD. Negative reset is set by
	 * B_NEG_RESET_MODE
	 * The first two axon types are exitor, the last two are suppresion.
	 * Neuron types are assigned to a crossbar with a  chance of connection set by B_CROSSBAR_PROB.

	 *
	 * Default values should produce a fully connected network that has a 50% chance of connection to an axon,
	 * a 75% chance of an axon being an excitor, an average wieght of 10, and an average threshold of 30, with leak
	 * off and negative reset at -10.
	 *
	 */
	void createProbNeuron(neuronState *s, tw_lp *lp) {
		int thresholdMin = B_TH_MIN;
		int thresholdMax = B_TH_MAX;

		int posAxWeightMax = B_P_AX_WT_MAX;
		int posAxWeightMin = B_P_AX_WT_MIN;

		int negAxWeightMin = 0;
		int negAxWeightMax = B_N_AX_WT_MAX;
		//leak weight
		int c = B_LEAK_WEIGHT * B_LEAK_ON;



		int gamma = 0;
		int kappa = 0;
		short sigma[4];
		bool b[4];
		b[0] = 0;
		b[1] = 0;
		b[2] = 0;
		b[3] = 0;

		float remoteConnectionProb = .5;

		//weight_type alpha = tw_rand_binomial()


	}

	void createProbAxon(neuronState *s, tw_lp *lp) {

	}
