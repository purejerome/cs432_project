/**
 * @file p4-codegen.c
 * @brief Compiler phase 4: code generation
 */
#include "p4-codegen.h"


/**
 * @brief State/data for the code generator visitor
 */
typedef struct CodeGenData
{
    /**
     * @brief Reference to the epilogue jump label for the current function
     */
    Operand current_epilogue_jump_label;
    bool gen_location_code;

    /* add any new desired state information (and clean it up in CodeGenData_free) */
} CodeGenData;

/**
 * @brief Allocate memory for code gen data
 * 
 * @returns Pointer to allocated structure
 */
CodeGenData* CodeGenData_new (void)
{
    CodeGenData* data = (CodeGenData*)calloc(1, sizeof(CodeGenData));
    CHECK_MALLOC_PTR(data);
    data->current_epilogue_jump_label = empty_operand();
    data->gen_location_code = true;
    return data;
}

/**
 * @brief Deallocate memory for code gen data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void CodeGenData_free (CodeGenData* data)
{
    /* free everything in data that is allocated on the heap */

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the error list inside a @c visitor
 * data structure
 */
#define DATA ((CodeGenData*)visitor->data)

/**
 * @brief Fills a register with the base address of a variable.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_base (ASTNode* node, Symbol* variable)
{
    Operand reg = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:
            reg = virtual_register();
            ASTNode_emit_insn(node,
                    ILOCInsn_new_2op(LOAD_I, int_const(variable->offset), reg));
            break;
        case STACK_PARAM:
        case STACK_LOCAL:
            reg = base_register();
            break;
        default:
            break;
    }
    return reg;
}

/**
 * @brief Calculates the offset of a scalar variable reference and fills a register with that offset.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_offset (ASTNode* node, Symbol* variable)
{
    Operand op = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:    op = int_const(0); break;
        case STACK_PARAM:
        case STACK_LOCAL:   op = int_const(variable->offset);
        default:
            break;
    }
    return op;
}

#ifndef SKIP_IN_DOXYGEN

/*
 * Macros for more convenient instruction generation
 */

#define EMIT0OP(FORM)             ASTNode_emit_insn(node, ILOCInsn_new_0op(FORM))
#define EMIT1OP(FORM,OP1)         ASTNode_emit_insn(node, ILOCInsn_new_1op(FORM,OP1))
#define EMIT2OP(FORM,OP1,OP2)     ASTNode_emit_insn(node, ILOCInsn_new_2op(FORM,OP1,OP2))
#define EMIT3OP(FORM,OP1,OP2,OP3) ASTNode_emit_insn(node, ILOCInsn_new_3op(FORM,OP1,OP2,OP3))

void CodeGenVisitor_gen_program (NodeVisitor* visitor, ASTNode* node)
{
    /*
     * make sure "code" attribute exists at the program level even if there are
     * no functions (although this shouldn't happen if static analysis is run first)
     */
    ASTNode_set_attribute(node, "code", InsnList_new(), (Destructor)InsnList_free);

    /* copy code from each function */
    FOR_EACH(ASTNode*, func, node->program.functions) {
        ASTNode_copy_code(node, func);
    }
}

void CodeGenVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* generate a label reference for the epilogue that can be used while
     * generating the rest of the function (e.g., to be used when generating
     * code for a "return" statement) */
    DATA->current_epilogue_jump_label = anonymous_label();
}

void CodeGenVisitor_gen_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    Operand base_pointer = base_register();
    Operand stack_pointer = stack_register();
    int local_size = (int)ASTNode_get_int_attribute(node, "localSize");
    /* every function begins with the corresponding call label */
    EMIT1OP(LABEL, call_label(node->funcdecl.name));

    /* BOILERPLATE: TODO: implement prologue */
    EMIT1OP(PUSH, base_pointer);
    EMIT2OP(I2I, stack_pointer, base_pointer);
    EMIT3OP(ADD_I, stack_pointer,
            int_const(-local_size), stack_pointer);

    /* copy code from body */
    ASTNode_copy_code(node, node->funcdecl.body);

    EMIT1OP(LABEL, DATA->current_epilogue_jump_label);
    /* BOILERPLATE: TODO: implement epilogue */
    
    EMIT2OP(I2I, base_pointer, stack_pointer);
    EMIT1OP(POP, base_pointer);
    EMIT0OP(RETURN);
}

void CodeGenVisitor_gen_funccall (NodeVisitor* visitor, ASTNode* node)
{
}

void CodeGenVisitor_gen_block (NodeVisitor* visitor, ASTNode* node)
{
    /* copy code from each statement in the block */
    FOR_EACH(ASTNode*, stmt, node->block.statements) {
        ASTNode_copy_code(node, stmt);
    }
}

void CodeGenVisitor_gen_return (NodeVisitor* visitor, ASTNode* node)
{
    /*example code from class*/
    /*TODO: move to literal gen*/
    // EMIT2OP(LOAD_I, int_const(5), return_register());
    // Operand temp_reg = virtual_register();
    // EMIT2OP(LOAD_I, int_const(5), temp_reg);
    // EMIT2OP(I2I, temp_reg, return_register());
    
    /*TODO: copy code from child*/
    
    /*TODO: get childs temp reg and use that as instead of reg from below*/
    
    /*TODO: add a jump to epilogue*/
    Operand child_reg = ASTNode_get_temp_reg(node->funcreturn.value);
    ASTNode_copy_code(node, node->funcreturn.value);
    EMIT2OP(I2I, child_reg, return_register());
    EMIT1OP(JUMP, DATA->current_epilogue_jump_label);
    
    // EMIT1OP(LABEL, DATA->current_epilogue_jump_label);
    // EMIT2OP(I2I, base_register(), stack_register());
    // EMIT0OP(RETURN);
}

void CodeGenVisitor_previsit_assignment (NodeVisitor* visitor, ASTNode* node)
{
    DATA->gen_location_code = false;
}

void CodeGenVisitor_gen_assignment (NodeVisitor* visitor, ASTNode* node)
{
    //TOOD: IMPLEMENT ARRAY ASSIGNMENTS
    ASTNode_copy_code(node, node->assignment.value);
    Symbol* var_symbol = lookup_symbol(node, node->assignment.location->location.name);
    Operand child_reg = ASTNode_get_temp_reg(node->assignment.value);
    Operand var_base_reg = var_base(node, var_symbol);
    Operand var_offset_op = var_offset(node, var_symbol);
    EMIT3OP(STORE_AI, child_reg, var_base_reg, var_offset_op);
}

void CodeGenVisitor_gen_location (NodeVisitor* visitor, ASTNode* node)
{
    bool gen_location_code = DATA->gen_location_code;
    if(!gen_location_code) {
        DATA->gen_location_code = true;
        return;
    }
    Symbol* var_symbol = lookup_symbol(node, node->location.name);
    Operand base_reg = var_base(node, var_symbol);
    Operand offset_reg = var_offset(node, var_symbol);
    Operand reg = virtual_register();
    ASTNode_set_temp_reg(node, reg);
    EMIT3OP(LOAD_AI, base_reg, offset_reg, reg);
}

//
/* BINARY OPERATIONS */
//

void CodeGenVisitor_gen_addition (NodeVisitor* visitor, ASTNode* node);

void CodeGenVisitor_gen_binaryop (NodeVisitor* visitor, ASTNode* node)
{
    switch (node->binaryop.operator) {
        case ADDOP:
            CodeGenVisitor_gen_addition(visitor, node);
            break;
        default:
            break;
    }
}

void CodeGenVisitor_gen_addition (NodeVisitor* visitor, ASTNode* node)
{
    Operand left_reg, right_reg;
    left_reg = ASTNode_get_temp_reg(node->binaryop.left);
    right_reg = ASTNode_get_temp_reg(node->binaryop.right);
    Operand result_reg = virtual_register();
    ASTNode_copy_code(node, node->binaryop.left);
    ASTNode_copy_code(node, node->binaryop.right);
    EMIT3OP(ADD, left_reg, right_reg, result_reg);
    ASTNode_set_temp_reg(node, result_reg);
}

//
/* LITERALS */
//

void CodeGenVisitor_gen_bool (NodeVisitor* visitor, ASTNode* node);
void CodeGenVisitor_gen_int (NodeVisitor* visitor, ASTNode* node);

void CodeGenVisitor_gen_literal (NodeVisitor* visitor, ASTNode* node)
{
    switch (node->literal.type) {
        case INT:
            CodeGenVisitor_gen_int(visitor, node);
            break;
        case BOOL:
            CodeGenVisitor_gen_bool(visitor, node);
            break;
        // case STR:
        //     CodeGenVisitor_gen_str(visitor, node);
        //     break;
        default:
            break;
    }
}

void CodeGenVisitor_gen_int (NodeVisitor* visitor, ASTNode* node)
{
    Operand reg = virtual_register();
    EMIT2OP(LOAD_I, int_const(node->literal.integer), reg);
    ASTNode_set_temp_reg(node, reg);
}

void CodeGenVisitor_gen_bool (NodeVisitor* visitor, ASTNode* node)
{
    Operand reg = virtual_register();
    EMIT2OP(LOAD_I, int_const(node->literal.boolean ? 1 : 0), reg);
    ASTNode_set_temp_reg(node, reg);
}

#endif
InsnList* generate_code (ASTNode* tree)
{
    InsnList* iloc = InsnList_new();
    
    if(tree == NULL) {
        return iloc;
    }


    NodeVisitor* v = NodeVisitor_new();
    v->data = CodeGenData_new();
    v->dtor = (Destructor)CodeGenData_free;
    v->postvisit_program     = CodeGenVisitor_gen_program;
    v->previsit_funcdecl     = CodeGenVisitor_previsit_funcdecl;
    v->postvisit_funcdecl    = CodeGenVisitor_gen_funcdecl;
    v->postvisit_funccall    = CodeGenVisitor_gen_funccall;
    v->postvisit_block       = CodeGenVisitor_gen_block;
    v->postvisit_return      = CodeGenVisitor_gen_return;
    v->postvisit_literal     = CodeGenVisitor_gen_literal;
    v->previsit_assignment   = CodeGenVisitor_previsit_assignment;
    v->postvisit_assignment  = CodeGenVisitor_gen_assignment;
    v->postvisit_binaryop    = CodeGenVisitor_gen_binaryop;
    v->postvisit_location    = CodeGenVisitor_gen_location;

    /* generate code into AST attributes */
    NodeVisitor_traverse_and_free(v, tree);

    /* copy generated code into new list (the AST may be deallocated before
     * the ILOC code is needed) */
    FOR_EACH(ILOCInsn*, i, (InsnList*)ASTNode_get_attribute(tree, "code")) {
        InsnList_add(iloc, ILOCInsn_copy(i));
    }
    return iloc;
}
