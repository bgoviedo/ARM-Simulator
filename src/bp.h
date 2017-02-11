/* Bijan Oviedo
 * ARM pipeline timing simulator
 */


#ifndef _BP_H_
#define _BP_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>
#include "pipe.h"

typedef struct branch_target_entry { // BTB contains 1024 entries
    int64_t address_tag; //64 bit pc the whole thing from fetch stage
    int valid_bit; // 1 bit where 1 == valid entry. this is not required but rec.
    int conditional_bit; // 1 bit where 1 == conditional
    int64_t target_branch; //64 bits, w/ last two bits being 0. branch target address
} branch_target_entry;

typedef struct branch_target_buffer {
    //int index[1024]; // bits [11:2] of PC
    branch_target_entry bt_entry[1024];
} branch_target_buffer;

typedef struct pattern_history_table {
    int two_bit_counter[256]; // using GHR ^ PC[9:2]
} pattern_history_table;

typedef struct
{
    int GHR;
    branch_target_buffer BTB;
    pattern_history_table PHT;
} bp_t;

bp_t bpt;

void bpt_init();

void bp_predict(int64_t PC, int is_squash);
void bp_update(int64_t PC, instruction i, int64_t nPC_m, int is_branch_taken, int* is_correct_branch);

#endif