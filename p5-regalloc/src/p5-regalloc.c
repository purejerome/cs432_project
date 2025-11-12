/**
 * @file p5-regalloc.c
 * @brief Compiler phase 5: register allocation
 */
#include "p5-regalloc.h"

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

void allocate_registers (InsnList* list, int num_physical_registers)
{
    FOR_EACH(ILOCInsn*, insn, list) {
        printf("Allocating registers for instruction\n");
        ILOCInsn_print(insn, stdout);
        // for each read vr in i:
        //     pr = ensure(vr)                     // make sure vr is in a phys reg
        //     replace vr with pr in i             // change register id

        ILOCInsn* read_regs = ILOCInsn_get_read_registers(insn);
        for(int i = 0; i < 3; i++){
            if(read_regs->op[i].type == VIRTUAL_REG){
                int virtual_reg = read_regs->op[i].id;
                int physical_reg = virtual_reg; //TODO: implement ensure(vr)
                replace_register(virtual_reg, physical_reg, insn);
                //TODO: this stuff
                //     if dist(vr) == INFINITY:            // if no future use
                //         name[pr] = INVALID              // then free pr
            }
        }
        ILOCInsn_free(read_regs);

        // written vr in i:
        //     pr = allocate(vr)                   // make sure phys reg is available
        //     replace vr with pr in i             // change register id and type
        Operand write_reg = ILOCInsn_get_write_register(insn);
        if(write_reg.type == VIRTUAL_REG) {
            int virtual_reg = write_reg.id;
            
            //TODO: implement pr = allocate(vr)
            int physical_reg = virtual_reg;
            replace_register(virtual_reg, physical_reg, insn);
        }
        if(write_reg.type != EMPTY) {
            printf("\n");
            printf("write:");
            Operand_print(write_reg, stdout);
        }
        printf("\n");
    }
}
