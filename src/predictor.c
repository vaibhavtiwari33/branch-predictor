//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Vaibhav Kant Tiwari";
const char *studentID   = "A59005342";
const char *email       = "vktiwari@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

//gshare:BHT, history
uint8_t *gshare_BHT;  // gshare branch history table
int gshare_BHT_index;
unsigned int global_history;  //global history
unsigned int gshare_result;

//tournament: local BHT, local PHT, global history, global BHT, selector
uint8_t *local_BHT;  // tournament local BHT 
uint32_t *local_PHT;  // tournament local PHT 
uint8_t *global_BHT;  // tournament global BHT 
uint8_t *selector;  // selector fot tournament
uint8_t local_outcome; //result form local predict
uint8_t global_outcome; //result form global predict
int PHT_index;
int global_BHT_index;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  global_history = 0;
  switch (bpType) {
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
    default:
      break;
  }
}

unsigned int gshare_prediction(uint32_t pc) {
  //XOR the program counter with global history and only keep ghistory amount of bits
  gshare_BHT_index = (pc ^ global_history) & ((1 << ghistoryBits) - 1);

  if (gshare_BHT[gshare_BHT_index] == WN || gshare_BHT[gshare_BHT_index] == SN) {
    gshare_result = NOTTAKEN;
  }
  else {
    gshare_result = TAKEN;
  }
  
  return gshare_result;
}


uint8_t tournament_prediction(uint32_t pc) {
  PHT_index = pc & ((1 << pcIndexBits) - 1);
  
  uint8_t local_prediction = local_BHT[local_PHT[PHT_index]];
  if(local_prediction == WT || local_prediction == ST) {
    local_outcome = TAKEN;
  } else {
    local_outcome = NOTTAKEN;
  }

  global_BHT_index = global_history & ((1 << ghistoryBits) - 1);
  uint8_t global_prediction = global_BHT[global_BHT_index];
  if(global_prediction == WT || global_prediction == ST) {
    global_outcome = TAKEN;
  } else {
    global_outcome = NOTTAKEN;
  }
  
  // predictor's selection
  // if selector[global_BHT_index] == WT(2) or ST(3) => choose local predictor
  // else choose global predictor
  if (selector[global_BHT_index] == WT || selector[global_BHT_index] == ST) {
    return local_outcome;
  } else {
    return global_outcome;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType (Branch Predictor type)
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_prediction(pc);
    case TOURNAMENT:
      return tournament_prediction(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is no compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train gShare predictor
void 
train_gshare(uint32_t pc, uint8_t outcome) {
    int BHTindex = (pc ^ global_history) & ((1 << ghistoryBits) - 1);

    // If the outcome was NOTTAKEN and the predictor hasn't saturated to SN (strongly not taken)
    if (outcome == NOTTAKEN && gshare_BHT[BHTindex] != SN) {
        gshare_BHT[BHTindex]--;
    } 
    // If the outcome was TAKEN and the predictor hasn't saturated to ST (strongly taken)
    else if (outcome == TAKEN && gshare_BHT[BHTindex] != ST) {
        gshare_BHT[BHTindex]++;
    } 

    //Right shift global history
    //Only keep ghistoryBits amount of history
    //Fill the new last bit of global history with outcome
    global_history = (((global_history << 1) | (outcome)) & ((1 << ghistoryBits) - 1));
}

void 
update_tournament_predictors(uint32_t pc, uint8_t outcome) {
    PHT_index = pc & ((1 << pcIndexBits) - 1);

    // Update local predictor
    int local_BHT_index = local_PHT[PHT_index];
    if (outcome == NOTTAKEN && local_BHT[local_BHT_index] != SN) {
      local_BHT[local_BHT_index]--;
    } else if (outcome == TAKEN && local_BHT[local_BHT_index] != ST) {
      local_BHT[local_BHT_index]++;
    } 

    // Update global predictor
    global_BHT_index = global_history & ((1 << ghistoryBits) - 1);
    if (outcome == NOTTAKEN && global_BHT[global_BHT_index] != SN) {
      global_BHT[global_BHT_index]--;
    } else if (outcome == TAKEN && global_BHT[global_BHT_index] != ST) {
      global_BHT[global_BHT_index]++;
    }
}

void 
update_tournament_selector(uint8_t outcome) {
    global_BHT_index = global_history & ((1 << ghistoryBits) - 1);

    if (local_outcome != outcome && selector[global_BHT_index] != SN) {
      // If local outcome was wrong && selector isn't saturated then shift the selector towards global predictor
      selector[global_BHT_index]--;
    } else if (local_outcome == outcome && selector[global_BHT_index] != ST) {
      // else shift the selector towards local predictor if selector isn't saturated 
      selector[global_BHT_index]++;    
    }
}

void 
train_tournament(uint32_t pc, uint8_t outcome) {
    if (global_outcome != local_outcome) {
        update_tournament_selector(outcome);
    }
    update_tournament_predictors(pc, outcome);

    global_history = (((global_history << 1) | (outcome)) & ((1 << ghistoryBits) - 1));

    int PHT_index = pc & ((1 << pcIndexBits) - 1);
    local_PHT[PHT_index] = (((local_PHT[PHT_index] << 1) | (outcome)) & ((1 << lhistoryBits) - 1));
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      train_gshare(pc, outcome);
      break;
    case TOURNAMENT:
      train_tournament(pc, outcome);
      break;
    case CUSTOM:
      //per_train(pc, outcome);
      break;
    default:
      break;
    }
}
