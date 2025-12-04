/**
 * @file p5-regalloc.c
 * @brief Compiler phase 5: register allocation
 */
/**
 * Authors: Alex Boyce-Draeger and Jerome Donfack
 * AI Statement: Used ChatGPT to create tests for the project. Used CoPilot
 * for autocompletion and suggestions, as well as helping to debug issues with spill code.
 */
#include "p5-regalloc.h"
#include <limits.h>
#define INVALID_VR      -1
#define INVALID_OFFSET  -1
#define INF_DIST        INT_MAX

int ensure(int vr, int* physical_regs, int* spill_offsets, int num_physical_registers, ILOCInsn* prev_insn, ILOCInsn* insn, ILOCInsn* local_allocator);
int allocate(int vr, int* physical_regs, int* spill_offsets, int num_physical_registers, ILOCInsn* prev_insn, ILOCInsn* insn, ILOCInsn* local_allocator);
int spill(int pr, int* physical_regs, int* spill_offsets, ILOCInsn* prev_insn, ILOCInsn* local_allocator);
int distance(int vr, ILOCInsn* insn);

/**
 * @brief Replace a virtual register id with a physical register id
 * 
 * Every virtual register operand with ID "vr" will be replaced by a physical
 * register operand with ID "pr" in the given instruction.
 * 
 * @param vr Virtual register id that should be replaced
 * @param pr Physical register id that it should be replaced by
 * @param isnsn Instruction to modify
 */
void replace_register(int vr, int pr, ILOCInsn* insn)
{
    for (int i = 0; i < 3; i++) {
        if (insn->op[i].type == VIRTUAL_REG && insn->op[i].id == vr) {
            insn->op[i].type = PHYSICAL_REG;
            insn->op[i].id = pr;
        }
    }
}

/**
 * @brief Insert a store instruction to spill a register to the stack
 * 
 * We need to allocate a new slot in the stack from for the current
 * function, so we need a reference to the local allocator instruction.
 * This instruction will always be the third instruction in a function
 * and will be of the form "add SP, -X => SP" where X is the current
 * stack frame size.
 * 
 * @param pr Physical register id that should be spilled
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 * @param local_allocator Reference to the local frame allocator instruction
 * @returns BP-based offset where the register was spilled
 */
int insert_spill(int pr, ILOCInsn* prev_insn, ILOCInsn* local_allocator)
{
    /* adjust stack frame size to add new spill slot */
    int bp_offset = local_allocator->op[1].imm - WORD_SIZE;
    local_allocator->op[1].imm = bp_offset;

    /* create store instruction */
    ILOCInsn* new_insn = ILOCInsn_new_3op(STORE_AI,
            physical_register(pr), base_register(), int_const(bp_offset));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;

    return bp_offset;
}

/**
 * @brief Insert a load instruction to load a spilled register
 * 
 * @param bp_offset BP-based offset where the register value is spilled
 * @param pr Physical register where the value should be loaded
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 */
void insert_load(int bp_offset, int pr, ILOCInsn* prev_insn)
{
    /* create load instruction */
    ILOCInsn* new_insn = ILOCInsn_new_3op(LOAD_AI,
            base_register(), int_const(bp_offset), physical_register(pr));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;
}

int num_virtual_registers(InsnList* list)
{
    int max_vr = -1;
    FOR_EACH(ILOCInsn*, insn, list) {
        for (int i = 0; i < 3; i++) {
            if (insn->op[i].type == VIRTUAL_REG) {
                if (insn->op[i].id > max_vr) {
                    max_vr = insn->op[i].id;
                }
            }
        }
    }
    return max_vr + 1;
}
    
void allocate_registers (InsnList* list, int num_physical_registers)
{
    if(list == NULL) {
        return;
    }
    int num_virtual_regs = num_virtual_registers(list);
    int* physical_regs = calloc(num_physical_registers, sizeof(int));
    int* spill_offsets = calloc(num_virtual_regs, sizeof(int));
    CHECK_MALLOC_PTR(physical_regs);
    CHECK_MALLOC_PTR(spill_offsets);
    for (int i = 0; i < num_physical_registers; ++i) physical_regs[i] = INVALID_VR;
    for(int i = 0; i < num_virtual_regs; i++){
        spill_offsets[i] = INVALID_OFFSET; //NO SPILL
    }
    ILOCInsn* local_allocator = NULL;
    ILOCInsn* prev_insn = NULL;
    FOR_EACH(ILOCInsn*, insn, list) {
        
        //save reference to stack allocator instruction if i is a call label
        if(insn->form == LABEL){
            ILOCInsn* potential_allocator = insn->next->next->next; //assumes standard function prologue
            if(potential_allocator != NULL &&
               potential_allocator->form == ADD_I &&
               potential_allocator->op[0].type == STACK_REG &&
               potential_allocator->op[1].type == INT_CONST &&
               potential_allocator->op[2].type == STACK_REG){
                local_allocator = potential_allocator;
            }
        }

        // allocate registers for read operands
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(insn);
        for(int i = 0; i < 3; i++){
            if(read_regs->op[i].type == VIRTUAL_REG){
                int virtual_reg = read_regs->op[i].id;
                int physical_reg = ensure(virtual_reg, physical_regs, spill_offsets, num_physical_registers, prev_insn, insn, local_allocator);
                replace_register(virtual_reg, physical_reg, insn);

                if(distance(virtual_reg, insn) == INF_DIST){ //INFINITY
                    physical_regs[physical_reg] = INVALID_VR; //INVALID
                }
            }
        }
        ILOCInsn_free(read_regs);
        
        // allocate register for write operand
        Operand write_reg = ILOCInsn_get_write_register(insn);
        if(write_reg.type == VIRTUAL_REG) {
            int virtual_reg = write_reg.id;
            
            int physical_reg = allocate(virtual_reg, physical_regs, spill_offsets, num_physical_registers, prev_insn, insn, local_allocator);
            replace_register(virtual_reg, physical_reg, insn);
        }
        
        // spill all registers before a call instruction
        if(insn->form == CALL){
            for(int i = 0; i < num_physical_registers; i++){
                if(physical_regs[i] != INVALID_VR){ //INVALID
                    spill(i, physical_regs, spill_offsets, prev_insn, local_allocator);
                }
            }
        }
        
        // save reference to i to facilitate spilling before next instruction
        prev_insn = insn;
    }
    free(physical_regs);
    free(spill_offsets);
}

int ensure(int vr, int* physical_regs, int* spill_offsets, int num_physical_registers, ILOCInsn* prev_insn, ILOCInsn* insn, ILOCInsn* local_allocator)
{
    // check if already allocated
    for(int i = 0; i < num_physical_registers; i++){
        if(physical_regs[i] == vr){
            return i;
        }
    }
    
    // allocate new register
    int pr = allocate(vr, physical_regs, spill_offsets, num_physical_registers, prev_insn, insn, local_allocator);
    
    // load from spill if necessary
    if(spill_offsets[vr] != INVALID_OFFSET){ //SPILLED
        insert_load(spill_offsets[vr], pr, prev_insn);
        spill_offsets[vr] = INVALID_OFFSET;
    }
    return pr;
}

int allocate(int vr, int* physical_regs, int* spill_offsets, int num_physical_registers, ILOCInsn* prev_insn, ILOCInsn* insn, ILOCInsn* local_allocator)
{
    // check for free register
    for(int i = 0; i < num_physical_registers; i++){
        if(physical_regs[i] == INVALID_VR){ //INVALID
            physical_regs[i] = vr;
            return i;
        }
    }
    
    // need to spill a register
    int fartherst_pr = -1;
    int fartherst_distance = INT_MIN;
    for(int i = 0; i < num_physical_registers; i++){
        int dist = distance(physical_regs[i], insn);
        if(dist > fartherst_distance){
            fartherst_distance = dist;
            fartherst_pr = i;
        }
    }
    spill(fartherst_pr, physical_regs, spill_offsets, prev_insn, local_allocator);
    physical_regs[fartherst_pr] = vr;
    return fartherst_pr;
}

int spill(int pr, int* physical_regs, int* spill_offsets, ILOCInsn* prev_insn, ILOCInsn* local_allocator)
{
    int vr = physical_regs[pr];
    int bp_offset = insert_spill(pr, prev_insn, local_allocator);
    spill_offsets[vr] = bp_offset;
    physical_regs[pr] = INVALID_VR; //INVALID
    return bp_offset;
}

int distance(int vr, ILOCInsn* insn)
{
    int dist = 0;
    ILOCInsn* current = insn->next ? insn->next : NULL;
    while(current != NULL){
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(current);
        for(int i = 0; i < 3; i++){
            if(read_regs->op[i].type == VIRTUAL_REG && read_regs->op[i].id == vr){
                ILOCInsn_free(read_regs);
                return dist;
            }
        }
        ILOCInsn_free(read_regs);
        Operand write_reg = ILOCInsn_get_write_register(current); // if overwitten, then distance is infinity
        if(write_reg.type == VIRTUAL_REG && write_reg.id == vr){
            return INF_DIST; //INFINITY
        }
        current = current->next;
        dist++;
    }
    return INF_DIST; //INFINITY
}
