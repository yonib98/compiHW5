#include "bp.hpp"
#include "types.h"
class CodeGenerator {
public:
    CodeGenerator() {};
    void const_strings_definition();

    void gen_exp_num(TypeNode *e, std::string value);
    void gen_exp_binop(TypeNode* e_res, TypeNode *e1, TypeNode *e2, char op);

    void gen_bool(TypeNode *e, bool value);
    void gen_defult_bool(int offset);
    void gen_string(TypeNode *e, std::string value);
    void gen_exp_and(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string label);
    void gen_exp_or(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string label);
    void gen_exp_relop(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string relop);
    void gen_exp_bool_labels(TypeNode* e, int offset);
    void gen_if(TypeNode* e, std::string true_label);
    void gen_if_bool(TypeNode* e);
    void gen_if_else(TypeNode* e, std::string true_label,  NarkerNode* n,std::string false_label);
    void gen_tri(TypeNode *e_true, NarkerNode* after_e1, TypeNode* e_bool, TypeNode* e_false, TypeNode* e_res);
    void gen_while(TypeNode* e, std::string true_label, std::string start_while_label);
    void gen_start_while(std::string start_label, NarkerNode* n);
    void gen_continue();
    void gen_break();
    void get_exp_id_place(TypeNode* e, int offset);
    void gen_stack_var(int offset, TypeNode* exp);
    void gen_stack_var(int offset, std::string place);
    void gen_ret(TypeNode* ret_type);
    void gen_ret_with_exp(TypeNode* ret_exp);
    void check_div_by_zero(TypeNode* e);
    void gen_div_by_zero_msg(std::string some_label);
    void gen_not_exp(TypeNode *e);
    void fix_call_sc(TypeNode* e);
    void gen_func_call_param(TypeNode* e);
    void connect_explist(TypeNode* e_list);
    void gen_cast(TypeNode* old_e, TypeNode *new_e);
    //funcs
    void gen_func(TypeNode* ret_type_n, std::string id, NodeParams* params_n);
    TypeNode* gen_func_call(Type ret_type, std::string id, FuncCallNode* params);
    void gen_bool_for_tri(TypeNode* e);
};
void fix_normal_exp(TypeNode* e);