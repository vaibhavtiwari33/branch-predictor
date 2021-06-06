//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
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
uint32_t *gshare_BHT;  // gshare branch history table
uint32_t global_history;  //global history
int gshare_BHT_index;
uint32_t gshare_result;


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

  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      global_history = 0;
      gshare_BHT = malloc((1<<ghistoryBits)*sizeof(uint32_t));
      memset(gshare_BHT, WN, sizeof(uint32_t)*(1<<ghistoryBits));
      break;
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
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
      //XOR the program counter with global history and only keep ghistory amount of bits
      gshare_BHT_index = (pc ^ global_history) & ((1 << ghistoryBits) - 1);

      if (gshare_BHT[gshare_BHT_index] == WN || gshare_BHT[gshare_BHT_index] == SN) {
        gshare_result = NOTTAKEN;
      }
      else {
        gshare_result = TAKEN;
      }
      
      return gshare_result;
    case TOURNAMENT:
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

    global_history <<= 1;                           //Right shift global history
    global_history  &= ((1 << ghistoryBits) - 1);   //Only keep ghistoryBits amount of history
    global_history |= outcome;                       //Fill the new last bit of global history with outcome
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
      //tournament_update(pc, outcome);
      break;
    case CUSTOM:
      //per_train(pc, outcome);
      break;
    default:
      break;
    }
}
