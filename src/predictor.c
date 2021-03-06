//========================================================//
//	predictor.c																						//
//	Source file for the Branch Predictor									//
//																												//
//	Implement the various branch predictors below as			//
//	described in the README																//
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//------------------------------------//
//			Predictor Configuration				//
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare", "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;	// Number of bits used for PC index
int bpType;				// Branch Prediction Type
int verbose;

//------------------------------------//
//			Predictor Data Structures			//
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

//gshare:BHT, history
uint8_t *gshare_BHT;	// gshare branch history table
int gshare_BHT_index;
unsigned int global_history;	//global history
unsigned int gshare_result;

//tournament: local BHT, local PHT, global history, global BHT, selector
uint8_t *local_BHT;	 // tournament local BHT 
uint32_t *local_PHT;	// tournament local PHT 
uint8_t *global_BHT;	// tournament global BHT 
uint8_t *selector;	// selector fot tournament
uint8_t local_outcome; //result form local predict
uint8_t global_outcome; //result form global predict
int PHT_index;
int global_BHT_index;

//perceptron predictor 
int table_length_perceptron;
int num_weights;
int **weight_table;
int *bias_table;
int theta;
int max_weight_limit;
int min_weight_limit;

//------------------------------------//
//				Predictor Functions					//
//------------------------------------//

// Initialize the predictor
//
void init_predictor()
{
	//
	//TODO: Initialize Branch Predictor Data Structures
	//
	global_history = 0;
	switch (bpType) 
	{
		case STATIC:
			break;
		case GSHARE:
			gshare_BHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
			memset(gshare_BHT, WN, sizeof(uint8_t) * (1 << ghistoryBits));
			break;
		case TOURNAMENT:
			local_BHT = malloc((1 << lhistoryBits) * sizeof(uint8_t));
			local_PHT = malloc((1 << pcIndexBits) * sizeof(uint32_t));
			global_BHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
			selector = malloc((1 << ghistoryBits) * sizeof(uint8_t));
			memset(local_BHT, WN, sizeof(uint8_t) * (1 << lhistoryBits));
			memset(local_PHT, 0, sizeof(uint32_t) * (1 << pcIndexBits));
			memset(global_BHT, WN, sizeof(uint8_t) * (1 << ghistoryBits));
			memset(selector, WN, sizeof(uint8_t) * (1 << ghistoryBits));
			break;
		case CUSTOM:
			//Use perceptron predictor with @table_length_perceptron table length, @num_weights weights,
			//1 bias term and 8 bits for weight values
			table_length_perceptron = 256;
			num_weights = 31;
			max_weight_limit = 127;
			min_weight_limit = -128;
			weight_table = (int**) malloc(sizeof(int*) * table_length_perceptron);
			for(int i=0; i<table_length_perceptron; i++)
				weight_table[i] = (int*) calloc(num_weights, sizeof(int));
			bias_table = (int*) calloc(table_length_perceptron, sizeof(int));
			
			//Set training threshold as @theta
			theta = 32;
			break;
		default:
			break;
	}
}

//Calculate the index of the perceptron table by using the last @table_length_perceptron bits of @pc
int calculate_weight_index(uint32_t pc)
{
	return pc & (table_length_perceptron - 1);
}

//Calculate the perceptron sum of a perceptron indexed by pc
int calculate_perceptron_sum(uint32_t pc)
{
	int weight_index = calculate_weight_index(pc);
	int sum = bias_table[weight_index];
	for(int i=0; i<num_weights; i++)
		sum = ((global_history>>i)&1) ? sum + weight_table[weight_index][i] : sum - weight_table[weight_index][i];
	return sum;
}

//Limit the weight values between -128 and 127 so that a max 8 bits are used for weight
int set_weight_limit(int *weight_value)
{
	if(*weight_value > max_weight_limit)
		*weight_value--;
	else if(*weight_value < min_weight_limit)
		*weight_value++;
}

uint8_t gshare_prediction(uint32_t pc) 
{
	//XOR the program counter with global history and only keep ghistory amount of bits
	gshare_BHT_index = (pc ^ global_history) & ((1 << ghistoryBits) - 1);

	if (gshare_BHT[gshare_BHT_index] == WN || gshare_BHT[gshare_BHT_index] == SN) 
		gshare_result = NOTTAKEN;
	else 
		gshare_result = TAKEN;
	return gshare_result;
}


uint8_t tournament_prediction(uint32_t pc) 
{
	PHT_index = pc & ((1 << pcIndexBits) - 1);
	
	uint8_t local_prediction = local_BHT[local_PHT[PHT_index]];
	if(local_prediction == WT || local_prediction == ST) 
		local_outcome = TAKEN;
	else 
		local_outcome = NOTTAKEN;

	global_BHT_index = global_history & ((1 << ghistoryBits) - 1);
	uint8_t global_prediction = global_BHT[global_BHT_index];
	if(global_prediction == WT || global_prediction == ST)
		global_outcome = TAKEN;
	else
		global_outcome = NOTTAKEN;
	
	// predictor's selection
	// if selector[global_BHT_index] == WT(2) or ST(3) => choose local predictor
	// else choose global predictor
	if (selector[global_BHT_index] == WT || selector[global_BHT_index] == ST)
		return local_outcome;
	else
		return global_outcome;
}

//Return TAKEN if the perceptron sum >=0, otherwise return NOTTAKEN
int make_perceptron_prediction(uint32_t pc)
{
	return (calculate_perceptron_sum(pc) >= 0) ? TAKEN : NOTTAKEN;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc)
{
	// Make a prediction based on the bpType (Branch Predictor type)
	switch (bpType) 
	{
		case STATIC:
			return TAKEN;
		case GSHARE:
			return gshare_prediction(pc);
		case TOURNAMENT:
			return tournament_prediction(pc);
		case CUSTOM:
			return make_perceptron_prediction(pc);
		default:
			break;
	}

	// If there is no compatable bpType then return NOTTAKEN
	return NOTTAKEN;
}

// Train gShare predictor
void train_gshare(uint32_t pc, uint8_t outcome) 
{
	int BHTindex = (pc ^ global_history) & ((1 << ghistoryBits) - 1);

	// If the outcome was NOTTAKEN and the predictor hasn't saturated to SN (strongly not taken)
	if (outcome == NOTTAKEN && gshare_BHT[BHTindex] != SN) 
		gshare_BHT[BHTindex]--;
	// If the outcome was TAKEN and the predictor hasn't saturated to ST (strongly taken)
	else if (outcome == TAKEN && gshare_BHT[BHTindex] != ST) 
		gshare_BHT[BHTindex]++;

	//Right shift global history
	//Only keep ghistoryBits amount of history
	//Fill the new last bit of global history with outcome
	global_history = (((global_history << 1) | (outcome)) & ((1 << ghistoryBits) - 1));
}

void update_tournament_predictors(uint32_t pc, uint8_t outcome) 
{
	PHT_index = pc & ((1 << pcIndexBits) - 1);

	// Update local predictor
	int local_BHT_index = local_PHT[PHT_index];
	if (outcome == NOTTAKEN && local_BHT[local_BHT_index] != SN) 
		local_BHT[local_BHT_index]--;
	else if (outcome == TAKEN && local_BHT[local_BHT_index] != ST)
		local_BHT[local_BHT_index]++;

	// Update global predictor
	global_BHT_index = global_history & ((1 << ghistoryBits) - 1);
	if (outcome == NOTTAKEN && global_BHT[global_BHT_index] != SN)
		global_BHT[global_BHT_index]--;
	else if (outcome == TAKEN && global_BHT[global_BHT_index] != ST)
		global_BHT[global_BHT_index]++;
}

void update_tournament_selector(uint8_t outcome) 
{
	global_BHT_index = global_history & ((1 << ghistoryBits) - 1);

	// If local outcome was wrong && selector isn't saturated then shift the selector towards global predictor
	if (local_outcome != outcome && selector[global_BHT_index] != SN)
		selector[global_BHT_index]--;
	// else shift the selector towards local predictor if selector isn't saturated 
	else if (local_outcome == outcome && selector[global_BHT_index] != ST)
		selector[global_BHT_index]++;		 
}

void train_tournament(uint32_t pc, uint8_t outcome) 
{
	if (global_outcome != local_outcome) 
		update_tournament_selector(outcome);

	update_tournament_predictors(pc, outcome);

	global_history = (((global_history << 1) | (outcome)) & ((1 << ghistoryBits) - 1));

	int PHT_index = pc & ((1 << pcIndexBits) - 1);
	local_PHT[PHT_index] = (((local_PHT[PHT_index] << 1) | (outcome)) & ((1 << lhistoryBits) - 1));
}

//Train the perceptron predictor
void train_perceptron(uint32_t pc, uint8_t outcome)
{
	int weight_index = calculate_weight_index(pc);
	int perceptron_sum = calculate_perceptron_sum(pc);
	int perceptron_outcome = perceptron_sum>=0;
	
	//Train predictor only if prediction != outcome or perceptron sum is less than threshold 
	if (abs(perceptron_sum) <= theta || perceptron_outcome != outcome) 
	{
		//Update bias table entries
		bias_table[weight_index] += (outcome==TAKEN) ? 1 : -1; 
		set_weight_limit(&bias_table[weight_index]);
    	
    	//Update perceptron table entries 
    	for (int i=0; i<num_weights; i++) 
    	{
    		weight_table[weight_index][i] += (((global_history>>i) & 1)==outcome) ? 1 : -1;
    		set_weight_limit(&weight_table[weight_index][i]);
    	}
  	}
  	global_history = (global_history<<1 | outcome) & ((1<<num_weights) - 1);
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome)
{
	switch (bpType) 
	{
		case STATIC:
			return;
		case GSHARE:
			train_gshare(pc, outcome);
			break;
		case TOURNAMENT:
			train_tournament(pc, outcome);
			break;
		case CUSTOM:
			train_perceptron(pc, outcome);
			break;
		default:
			break;
	}
}
