/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 * Reza Jokar, Gushu Li, 2016
 */


#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"
#include "stdbool.h"
#include <limits.h>

//typedef int bool;
#define TRUE 1
#define FALSE 0

typedef enum instruction {
    NO_INSTRUCTION,//0
    //R type
    ADD,
    ADDS,
    AND,
    ANDS,
    BR,
    EOR,
    MUL,
    ORR,
    SDIV,
    SUB,
    SUBS,
    CMP,
    UDIV,
    //CB-type
    CBNZ,
    CBZ,
    BEQ,
    BNE,
    BGT,
    BLT,
    BGE,
    BLE,
    // D-type
    LDUR,
    LDURB,
    LDURH,
    STUR,
    STURB,
    STURH,
    STURW,
    // I-type
    ADDI,
    ADDIS,
    SUBI,
    SUBIS,
    // Shift-Type
    LSL, 
    LSLI,
    LSRI,
    LSR,
    // B-type
    B,
    BL,
    // IW-type
    MOVZ,
    // HLT
    HLT
} instruction;

typedef struct Pipe_Reg_IFtoID {
    int64_t PC_d;
    uint32_t IR_d;
    uint32_t flag_n;
    uint32_t flag_z;
    uint32_t flag_v;
    uint32_t flag_c;
} Pipe_Reg_IFtoID;

typedef struct Pipe_Reg_IDtoEX {
    int64_t PC_e;
    int64_t A_e;
    int64_t B_e;
    uint32_t Imm_e;
    uint32_t write_register_number;
    uint32_t flag_n;
    uint32_t flag_z;
    uint32_t flag_v;
    uint32_t flag_c;
    uint32_t register_Rn1;
    uint32_t register_Rm2;
    uint32_t machine_code;
    instruction i;
} Pipe_Reg_IDtoEX;

typedef struct Pipe_Reg_EXtoMEM {
    int64_t nPC_m;
    int64_t Aout_m;
    int64_t B_m;
    uint32_t write_register_number;
    uint32_t flag_n;
    uint32_t flag_z;
    uint32_t flag_v;
    uint32_t flag_c;
    uint32_t will_write;
    uint32_t machine_code;
    instruction i;
} Pipe_Reg_EXtoMEM;

typedef struct Pipe_Reg_MEMtoWB {
    int64_t MDR_w;
    int64_t Aout_w;
    uint32_t write_register_number;
    uint32_t flag_n;
    uint32_t flag_z;
    uint32_t flag_v;
    uint32_t flag_c;
    uint32_t will_write;
    uint32_t machine_code;
    instruction i;
} Pipe_Reg_MEMtoWB;

typedef struct CPU_State {
    /* register file state */
    int64_t REGS[ARM_REGS];
    int FLAG_N;        /* flag N */
    int FLAG_Z;        /* flag Z */
    int FLAG_V;        /* flag V */
    int FLAG_C;        /* flag C */
    /* program counter in fetch stage */
    uint64_t PC;
} CPU_State;

typedef struct global_vars {
    uint32_t dcache_num_to_stall;
    uint32_t icache_num_to_squash;
    uint32_t machine_code;
    bool is_unconditional;
    bool should_squash_fetch;
    bool detected_load_hazard;
    bool detected_halt;
    bool did_forward_flags;
    bool should_squash_decode;
    bool should_update_i_cache;
    bool should_update_d_cache;
} global_vars;

int RUN_BIT;

Pipe_Reg_IFtoID PREGISTER_IF_ID;
Pipe_Reg_IDtoEX PREGISTER_ID_EX;
Pipe_Reg_EXtoMEM PREGISTER_EX_MEM;
Pipe_Reg_MEMtoWB PREGISTER_MEM_WB;

global_vars globals;
/* global variable -- pipeline state */
extern CPU_State CURRENT_STATE;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

#endif