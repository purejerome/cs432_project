/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 */
#include "p3-analysis.h"

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
    /**
     * @brief List of errors detected
     */
    ErrorList* errors;

    /* BOILERPLATE: TODO: add any new desired state information (and clean it up in AnalysisData_free) */
    DecafType current_function_type;

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 * 
 * @returns Pointer to allocated structure
 */
AnalysisData* AnalysisData_new (void)
{
    AnalysisData* data = (AnalysisData*)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    data->current_function_type = UNKNOWN;
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free (AnalysisData* data)
{
    /* free everything in data that is allocated on the heap except the error
     * list; it needs to be returned after the analysis is complete */
    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref AnalysisVisitor
 * data structure
 */
#define DATA ((AnalysisData*)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData*)visitor->data)->errors)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 * 
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol* lookup_symbol_with_reporting(NodeVisitor* visitor, ASTNode* node, const char* name)
{
    Symbol* symbol = lookup_symbol(node, name);
    if (symbol == NULL) {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void*)(T), \
                                 type_attr_print, dummy_free)
                                 

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType)(long)ASTNode_get_attribute(N, "type")

void AnalysisVisitor_infer_literal (NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(node->literal.type);
}

void AnalysisVisitor_screen_vardecl (NodeVisitor* visitor, ASTNode* node)
{
    DecafType var_type = node->vardecl.type;
    if (var_type == VOID) {
        ErrorList_printf(ERROR_LIST, "Type error on line %d: variable declared with void type", node->source_line);
    }
    if (node->vardecl.is_array && node->vardecl.array_length <= 0) {
        ErrorList_printf(ERROR_LIST, "Type error on line %d: array variable declared with non-positive length", node->source_line);
    }
    return;
}

void AnalysisVisitor_screen_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    DecafType return_type = node->funcdecl.return_type;
    DATA->current_function_type = return_type;
    return;
}

void AnalysisVisitor_leave_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    DATA->current_function_type = UNKNOWN;
    return;
}

void AnalysisVisitor_check_return (NodeVisitor* visitor, ASTNode* node)
{
    DecafType return_type = (node->funcreturn.value == NULL) ? VOID : GET_INFERRED_TYPE(node->funcreturn.value);
    if (return_type != DATA->current_function_type) {
        ErrorList_printf(ERROR_LIST, "Type error on line %d: return type does not match function return type", node->source_line);
    }
    return;
}


void AnalysisVisitor_infer_location (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting(visitor, node, node->location.name);
    if (symbol != NULL) {
        SET_INFERRED_TYPE(symbol->type);
    } else {
        SET_INFERRED_TYPE(UNKNOWN);
    }
    return;
}

void AnalysisVisitor_check_location (NodeVisitor* visitor, ASTNode* node)
{
    if(node->location.index != NULL) {
        Symbol* symbol = lookup_symbol_with_reporting(visitor, node, node->location.name);
        if(symbol != NULL && symbol->symbol_type != ARRAY_SYMBOL) {
            ErrorList_printf(ERROR_LIST, "Type error on line %d: non-array variable used with index", node->source_line);
        }
        
        // if(symbol != NULL && symbol->symbol_type == SCALAR_SYMBOL){
        //     printf("SCALAR SYMBOL\n");
        // } else if(symbol != NULL && symbol->symbol_type == ARRAY_SYMBOL){
        //     printf("ARRAY SYMBOL\n");
        // } else if(symbol != NULL && symbol->symbol_type == FUNCTION_SYMBOL){
        //     printf("FUNCTION SYMBOL\n");
        // }
        
        DecafType index_type = GET_INFERRED_TYPE(node->location.index);
        if(index_type != INT) {
            ErrorList_printf(ERROR_LIST, "Type error on line %d: array index is not an integer", node->source_line);
        }
    } else {
        Symbol* symbol = lookup_symbol_with_reporting(visitor, node, node->location.name);
        if(symbol != NULL && symbol->symbol_type != SCALAR_SYMBOL) {
            ErrorList_printf(ERROR_LIST, "Type error on line %d: array variable used without index", node->source_line);
        }
        // if(symbol != NULL && symbol->symbol_type == SCALAR_SYMBOL){
        //     printf("SCALAR SYMBOL\n");
        // } else if(symbol != NULL && symbol->symbol_type == ARRAY_SYMBOL){
        //     printf("ARRAY SYMBOL\n");
        // } else if(symbol != NULL && symbol->symbol_type == FUNCTION_SYMBOL){
        //     printf("FUNCTION SYMBOL\n");
        // }
        // Symbol_print(symbol, stdout);
        // printf("\n");
    }
    return;
}
void AnalysisVisitor_infer_unaryop (NodeVisitor* visitor, ASTNode* node)
{
    switch(node->unaryop.operator) {
        case NEGOP:
            SET_INFERRED_TYPE(INT);
            break;
        case NOTOP:
            SET_INFERRED_TYPE(BOOL);
            break;
        default:
            ErrorList_printf(ERROR_LIST, "Internal error: unhandled unary operator on line %d", node->source_line);
            SET_INFERRED_TYPE(UNKNOWN);
    }
    return;
}

void AnalysisVisitor_check_unaryop (NodeVisitor* visitor, ASTNode* node)
{
    DecafType child_type = GET_INFERRED_TYPE(node->unaryop.child);
    
    switch(node->unaryop.operator) {
        case NEGOP:
            if(child_type != INT) {
                ErrorList_printf(ERROR_LIST, "Type error on line %d: unary - applied to non-integer operand", node->source_line);
            }
            break;
        case NOTOP:
            if(child_type != BOOL) {
                ErrorList_printf(ERROR_LIST, "Type error on line %d: unary ! applied to non-boolean operand", node->source_line);
            }
            break;
        default:
            ErrorList_printf(ERROR_LIST, "Internal error: unhandled unary operator on line %d", node->source_line);
    }
    return;
}

void AnalysisVisitor_infer_binaryop (NodeVisitor* visitor, ASTNode* node)
{
    switch(node->binaryop.operator) {
        case ADDOP: case SUBOP: case MULOP: case DIVOP: case MODOP:
            SET_INFERRED_TYPE(INT);
            break;
        case OROP: case ANDOP: case LTOP: case LEOP: case GEOP: case GTOP:
        case EQOP: case NEQOP:
            SET_INFERRED_TYPE(BOOL);
            break;
        default:
            ErrorList_printf(ERROR_LIST, "Internal error: unhandled binary operator on line %d", node->source_line);
            SET_INFERRED_TYPE(UNKNOWN);
    }
    return;
}

void AnalysisVisitor_check_binaryop (NodeVisitor* visitor, ASTNode* node)
{
    DecafType left_type = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right_type = GET_INFERRED_TYPE(node->binaryop.right);
    
    switch(node->binaryop.operator) {
        case ADDOP: case SUBOP: case MULOP: case DIVOP: case MODOP:
        case LTOP: case LEOP: case GEOP: case GTOP:
            if(left_type != INT || right_type != INT) {
                ErrorList_printf(ERROR_LIST, "Type error on line %d: binary operator applied to non-integer operands", node->source_line);
            } 
            break;
        case OROP: case ANDOP:
            if(left_type != BOOL || right_type != BOOL) {
                ErrorList_printf(ERROR_LIST, "Type error on line %d: binary operator applied to non-boolean operands", node->source_line);
            }
            break;
        case EQOP: case NEQOP:
            if(left_type != right_type) {
                ErrorList_printf(ERROR_LIST, "Type error on line %d: binary operator applied to incompatible operands", node->source_line);
            }
            break;
        default:
            ErrorList_printf(ERROR_LIST, "Internal error: unhandled binary operator on line %d", node->source_line);
    }
    return;
}

typedef struct {
 int block_depth;
} ListVisitorsData;
 
#define DATA2 ((ListVisitorsData*)(visitor->data))
 
void ListVariablesVisitor_previsit_program (NodeVisitor* visitor, ASTNode* node) {
 DATA2->block_depth = 0;
}
 
void ListVariablesVisitor_previsit_block (NodeVisitor* visitor, ASTNode* node) {
 DATA2->block_depth += 1;
}
 
void ListVariablesVisitor_postvisit_block (NodeVisitor* visitor, ASTNode* node) {
 DATA2->block_depth -= 1;
}
 
void ListVariablesVisitor_visit_vardecl (NodeVisitor* visitor, ASTNode* node) {
 printf("%d %s\n", DATA2->block_depth, node->vardecl.name);
}
 
NodeVisitor* ListVariablesVisitor_new () {
    NodeVisitor* v = NodeVisitor_new();
    v->data = malloc(sizeof(ListVisitorsData));
    v->dtor = free;
    v->previsit_program = ListVariablesVisitor_previsit_program;
    v->previsit_block = ListVariablesVisitor_previsit_block;
    v->postvisit_block = ListVariablesVisitor_postvisit_block;
    v->previsit_vardecl = ListVariablesVisitor_visit_vardecl;
    return v;
}

ErrorList* analyze (ASTNode* tree)
{
    if(tree == NULL) {
        ErrorList* errors = ErrorList_new();
        return errors;
    }
    /* allocate analysis structures */
    NodeVisitor* v = NodeVisitor_new();
    v->data = (void*)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_literal = AnalysisVisitor_infer_literal;
    v->previsit_vardecl = AnalysisVisitor_screen_vardecl;
    v->previsit_funcdecl = AnalysisVisitor_screen_funcdecl;
    v->postvisit_funcdecl = AnalysisVisitor_leave_funcdecl;
    v->postvisit_return = AnalysisVisitor_check_return;
    v->previsit_location = AnalysisVisitor_infer_location;
    v->postvisit_location = AnalysisVisitor_check_location;
    v->previsit_unaryop = AnalysisVisitor_infer_unaryop;
    v->postvisit_unaryop = AnalysisVisitor_check_unaryop;
    v->previsit_binaryop = AnalysisVisitor_infer_binaryop;
    v->postvisit_binaryop = AnalysisVisitor_check_binaryop;
    /* perform analysis, save error list, clean up, and return errors */
    NodeVisitor_traverse(v, tree);
    ErrorList* errors = ((AnalysisData*)v->data)->errors;
    NodeVisitor_free(v);
    return errors;
}

