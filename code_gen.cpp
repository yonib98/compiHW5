#include "code_gen.h"
#include "types.h"
#include <iostream>
#include <fstream>
#include<stdlib.h>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>


std::vector<std::string> start_labels_while_vec;
std::vector<std::string> base_addr_vec;
std::vector<std::vector<pair<int, BranchLabelIndex>>> end_addresses_while_vec;
CodeGenerator cg;

//Todo DEBUG REMOVE BEFORE SUBMITTING
void debug_lli(){
    return;
    CodeBuffer& cb = CodeBuffer::instance();
    std::string code = cb.getCode();
    std::ofstream file("code.txt");
    file << code;
    file.close();
    system("./llvm.py code.txt");
}

void CodeGenerator::fix_call_sc(TypeNode *e) {
    CodeBuffer& cb = CodeBuffer::instance();
    if(e->type==_BOOL){
      std::string dummy_label = cb.genLabel();
      cb.bpatch(e->true_list, dummy_label);
      cb.bpatch(e->false_list, dummy_label);
    }
    cb.bpatch(e->start_list, e->start_label);

}
void fix_start_label(TypeNode* e_res, TypeNode* e1, TypeNode* e2){
    CodeBuffer& cb = CodeBuffer::instance();
    cb.bpatch(e2->start_list, e2->start_label);
    e_res->start_label = e1->start_label;
    e_res->start_list = e1->start_list;
    e_res->end_label = e2->start_label;
}
void fix_normal_exp(TypeNode* e){
    CodeBuffer& cb = CodeBuffer::instance();
    cb.bpatch(e->start_list, e->start_label);
}

void CodeGenerator::const_strings_definition(){
    CodeBuffer& cb = CodeBuffer::instance();
    std::vector<std::string> strings = {
            "@.divZeroMsg = internal constant [23 x i8] c\"Error division by zero\\00\"",
    };
    for (string s: strings){
        cb.emitGlobal(s);
    }
}

void CodeGenerator::check_div_by_zero(TypeNode *e) {
    CodeBuffer& cb = CodeBuffer::instance();
    std::stringstream code;
    std::string reg = cb.freshVar();

    code << reg << " = icmp eq i32 0, " << e->place << std::endl;
    code << "br i1 " << reg << ",label @, label @";
    int loc = cb.emit(code.str());
    std::string zero_label = cb.genLabel();
    gen_div_by_zero_msg(zero_label);
    std::string not_zero_label= cb.genLabel();
    cb.bpatch(cb.makelist({loc, FIRST}), zero_label);
    cb.bpatch(cb.makelist({loc, SECOND}), not_zero_label);
}
void CodeGenerator::gen_div_by_zero_msg(std::string some_label) {
    CodeBuffer& cb = CodeBuffer::instance();
    std::string reg = cb.freshVar();
    std::stringstream code;
    code << reg << " = " << "getelementptr [23 x i8], [23 x i8]* @.divZeroMsg, i32 0, i32 0" << std::endl;
    code << "call void @print(i8* " << reg << ")" << std::endl;
    code << "call void @exit(i32 1)";
    cb.emit(code.str());
    int loc =  cb.emit("br label @"); //Block needs to end with a br/ret command, we jump to somewhere doesn't really matter where as long as the label exist.
    cb.bpatch(cb.makelist({loc, FIRST}), some_label);

}
void CodeGenerator::gen_exp_num(TypeNode* e, std::string value){
    CodeBuffer& cb  = CodeBuffer::instance();
    e->place = cb.freshVar();
    std::string code = e->place +" = add i32 0, " + value;
    cb.emit(code);

}
void CodeGenerator::gen_exp_binop(TypeNode* e_res, TypeNode* e1, TypeNode* e2, char op){
    CodeBuffer& cb  = CodeBuffer::instance();
    e_res->place = cb.freshVar();
    std::stringstream code;
    std::string llvm_op;
    fix_start_label(e_res, e1, e2);
    switch (op){
        case '+':
            llvm_op = "add";
            break;
        case '-':
            llvm_op = "sub";
            break;
        case '*':
            llvm_op = "mul";
            break;
        case '/':
            check_div_by_zero(e2);
            if (e_res->type==_INT){
                llvm_op = "sdiv";
            }else if (e_res->type == _BYTE){
                llvm_op = "udiv";
            }

            break;
        default:
            std::cout << "ERROR IN LLVM_OP" << std::endl; //REMOVE BEFORE SUBMITTING.
            exit(0);
    }
    int loc = cb.emit("br label @");
    e_res->end_label = cb.genLabel();
    cb.bpatch(cb.makelist({loc, FIRST}), e_res->end_label);
    code << e_res->place << " = " << llvm_op << " i32 " << e1->place << ", " << e2->place;
    cb.emit(code.str());
    if (e_res->type==_BYTE){
        //Zero 24 msb bits
        std::string old_var = e_res->place;
        e_res->place = cb.freshVar();
        unsigned int mask = 0xff; // get only 8 bits
        std::stringstream casting_code;
        casting_code << e_res->place << " = and i32 " << old_var << ", " << mask;
        cb.emit(casting_code.str());
    }



}
void CodeGenerator::gen_defult_bool(int offset){
    CodeBuffer& cb  = CodeBuffer::instance();
    std::string place = cb.freshVar();
    std::stringstream code;
    code << place << " = " << "add i32 0, 0";
    cb.emit(code.str());
    gen_stack_var(offset, place);
}

void CodeGenerator::gen_bool(TypeNode *e, bool value){
    CodeBuffer& cb  = CodeBuffer::instance();
    if (!value) {
        int loc = cb.emit("br label @");
        e->false_list = cb.makelist({loc, FIRST});
    } else {
        int loc = cb.emit("br label @");
        e->true_list = cb.makelist({loc, FIRST});
    }
    //cb.bpatch(e->start_list, e->start_label);
}
void CodeGenerator::gen_exp_and(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string label){
    CodeBuffer& cb  = CodeBuffer::instance();
    cb.bpatch(e1->true_list, label);
    e_res->false_list = cb.merge(e1->false_list, e2->false_list);
    e_res->true_list = e2->true_list;
    fix_start_label(e_res, e1, e2);
}

void CodeGenerator::gen_exp_or(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string label){
    CodeBuffer& cb  = CodeBuffer::instance();
    cb.bpatch(e1->false_list, label);
    e_res->true_list = cb.merge(e1->true_list, e2->true_list);
    e_res->false_list = e2->false_list;

    fix_start_label(e_res, e1, e2);



}
void CodeGenerator::gen_exp_relop(TypeNode* e_res, TypeNode* e1, TypeNode* e2, std::string relop){
    CodeBuffer& cb  = CodeBuffer::instance();
    std::stringstream code;
    std::unordered_map<std::string, std::string> llvm_relop_map = {
            {"<:SIGNED", "slt"},
            {"<:UNSIGNED", "ult"},
            {"<=:SIGNED", "sle"},
            {"<=:UNSIGNED", "ule"},
            {">:SIGNED", "sgt"},
            {">:UNSIGNED", "ugt"},
            {">=:SIGNED", "sge"},
            {">=:UNSIGNED", "uge"},
            {"==:SIGNED", "eq"},
            {"==:UNSIGNED", "eq"},
            {"!=:SIGNED", "ne"},
            {"!=:UNSIGNED", "ne"},


    };
    relop += e1->type==_BYTE && e2->type==_BYTE? ":UNSIGNED":":SIGNED"; // If both aren't BYTE -> hamra to int
    std::string llvm_relop = llvm_relop_map[relop];
    std::string cond_reg = cb.freshVar();
    code << cond_reg <<" = icmp " << llvm_relop << " i32 " << e1->place << ", " << e2->place << std::endl;
    code << "br i1 " << cond_reg << ", label @, label @";
    int loc = cb.emit(code.str());
    e_res->true_list = cb.makelist({loc, FIRST});
    e_res->false_list = cb.makelist({loc, SECOND});
    fix_start_label(e_res, e1, e2);
}
void CodeGenerator::gen_exp_bool_labels(TypeNode* e, int offset) {
    std::string base_addr = base_addr_vec.back();
    CodeBuffer& cb  = CodeBuffer::instance();
    std::string true_label = cb.genLabel();
    int next_label_true = cb.emit("br label @");
    std::string false_label = cb.genLabelPhi();
    int next_label_false = cb.emit("br label @");
    std::string next_label = cb.genLabelPhi();

    std::vector<pair<int,BranchLabelIndex>> next_label_list = CodeBuffer::merge(CodeBuffer::makelist({next_label_true, FIRST}),
                                           CodeBuffer::makelist({next_label_false,FIRST}));

    cb.bpatch(e->true_list, true_label);
    cb.bpatch(e->false_list, false_label);
    cb.bpatch(next_label_list, next_label);
    std::string reg = cb.freshVar();
    cb.emit(reg +" = phi i32 [1" +", %" + true_label + "] , [0" + " , %" + false_label+"]");
    //Store result
    std::string ptr = cb.freshVar();
    std::stringstream  code;
    code << ptr << " = " << "getelementptr [50 x i32], [50 x i32]* " << base_addr << ", i32 0, i32 " << offset;
    code << std::endl << "store i32 " << reg << ", i32* " << ptr;
    cb.emit(code.str());

}
void CodeGenerator::gen_stack_var(int offset, std::string place){
    CodeBuffer& cb  = CodeBuffer::instance();
    std::string base_addr = base_addr_vec.back();
    std::stringstream code;
    if (offset<0){
        //WON'T GO HERE
        return;
    }
    std::string ptr = cb.freshVar();
    code << ptr << " = " << "getelementptr [50 x i32], [50 x i32]* " << base_addr << ", i32 0, i32 " << offset;
    code << std::endl << "store i32 " << place << ", i32* " << ptr;
    cb.emit(code.str());
}
void CodeGenerator::gen_stack_var(int offset, TypeNode* exp) {
    CodeBuffer& cb  = CodeBuffer::instance();

    if (exp->type==_BOOL){
        gen_exp_bool_labels(exp, offset);
        return;
    }if (offset<0){
       int loc = cb.emit("br label @");
        exp->start_label = cb.genLabel();
        cb.bpatch(cb.makelist({loc, FIRST}), exp->start_label);
        return;
    }
    std::string place = exp->place;

    std::string base_addr = base_addr_vec.back();
    std::stringstream code;
    std::string ptr = cb.freshVar();
    //   int loc = cb.emit("br label @");
   // exp->start_label = cb.genLabel();
   // cb.bpatch(cb.makelist({loc, FIRST}), exp->start_label);
    code << ptr << " = " << "getelementptr [50 x i32], [50 x i32]* " << base_addr << ", i32 0, i32 " << offset;
    code << std::endl << "store i32 " << place << ", i32* " << ptr;
    cb.emit(code.str());

}
void CodeGenerator::get_exp_id_place(TypeNode* e, int offset){
    std::string base_addr = base_addr_vec.back();
    CodeBuffer& cb  = CodeBuffer::instance();
    std::string ptr = cb.freshVar();
    e->place = cb.freshVar();
    std::stringstream code;
    if(offset >= 0){
        int loc = cb.emit("br label @");
        e->start_label = cb.genLabel();
        e->start_list = cb.makelist({loc, FIRST});

        code << ptr << " = " << "getelementptr [50 x i32], [50 x i32]* " << base_addr << ", i32 0, i32 " << offset << std::endl;
        code << e->place << " = load i32, i32* " << ptr;
        cb.emit(code.str());
        //fix_start_and_end_labels(e);
    }else{
        e->place = "%" + std::to_string((-offset) - 1);
        int loc = cb.emit("br label @");

        e->start_label = cb.genLabel();
        e->start_list = cb.makelist({loc, FIRST});
    }
    //If Bool we need to gen_bool branches..
    if (e->type == _BOOL){
        std::string cond_reg = cb.freshVar();
        std::stringstream trunc_code;
        // Convert to i1
        trunc_code << cond_reg << " = trunc i32 " << e->place <<  " to i1";
        cb.emit(trunc_code.str());
        //Jump to true/false labels
        std:stringstream branch_code;
        branch_code << "br i1 " << cond_reg << " , label @, label @" << std::endl;
        int loc = cb.emit(branch_code.str());
        e->true_list = cb.makelist({loc, FIRST});
        e->false_list = cb.makelist({loc, SECOND});
    }


}

void CodeGenerator::gen_if(TypeNode* e, std::string true_label){
    CodeBuffer& cb  = CodeBuffer::instance();
    cb.bpatch(e->true_list, true_label);

    int loc = cb.emit("br label @");
    std::vector<pair<int, BranchLabelIndex>> next_list = cb.merge(e->false_list, {{loc, FIRST}});
    std::string next_label = CodeBuffer::instance().genLabel();
    cb.bpatch(next_list, next_label);


}
void CodeGenerator::gen_if_bool(TypeNode* e){
        CodeBuffer& cb = CodeBuffer::instance();
        cb.bpatch(e->start_list, e->start_label);
}
void CodeGenerator::gen_if_else(TypeNode* e, std::string true_label,  NarkerNode* n,std::string false_label){
    CodeBuffer& cb  = CodeBuffer::instance();
    cb.bpatch(e->true_list, true_label);
    cb.bpatch(e->false_list, false_label);

    int loc =cb.emit("br label @");
    n->next_list = cb.merge(n->next_list, {{loc, FIRST}});
    std::string next_label = CodeBuffer::instance().genLabel();
    cb.bpatch(n->next_list, next_label);
    //fix_start_and_end_labels(e);
    cb.bpatch(e->start_list, e->start_label);

}
void CodeGenerator::gen_start_while(std::string start_label, NarkerNode* n){
    CodeBuffer& cb  = CodeBuffer::instance();
    start_labels_while_vec.push_back(start_label);
    end_addresses_while_vec.push_back({});
    cb.bpatch(n->next_list, start_label);


}
void CodeGenerator::gen_while(TypeNode* e, std::string true_label, std::string start_while_label){
    start_labels_while_vec.pop_back();
    CodeBuffer& cb  = CodeBuffer::instance();
    cb.bpatch(e->true_list, true_label);
    cb.emit("br label %" + start_while_label);
    std::string false_label = cb.genLabel();
    cb.bpatch(e->false_list, false_label);
    std::vector<pair<int, BranchLabelIndex>> break_addrs  = end_addresses_while_vec.back();
    end_addresses_while_vec.pop_back();
    cb.bpatch(break_addrs, false_label);



}

void CodeGenerator::gen_continue() {
    CodeBuffer& cb  = CodeBuffer::instance();
    std::stringstream code;
    code << "br label %" << start_labels_while_vec.back();
    cb.emit(code.str());
}

void CodeGenerator::gen_break(){
    CodeBuffer& cb  = CodeBuffer::instance();
    std::stringstream code;
    code << "br label @";
    int loc = cb.emit(code.str());
    end_addresses_while_vec.back().push_back({loc, FIRST});
}
void CodeGenerator::gen_func(TypeNode* ret_type_n, std::string id, NodeParams* params_n){
    CodeBuffer& cb  = CodeBuffer::instance();
    std::stringstream code;
    std::string ret_type = ret_type_n->type != _VOID ? "i32" : "void";
    code << "define " << ret_type << " @" << id << "(";
    NodeParams* tmp = params_n;
    while (tmp!=nullptr){
        code << "i32";
        tmp=tmp->next;
        if(tmp!=nullptr){
            code << ", ";
        }
    }
    code << ") {" << std::endl;
    std::string func_base_addr = cb.freshVar();
    base_addr_vec.push_back(func_base_addr);
    code << func_base_addr << " = " << "alloca [50 x i32]";
    cb.emit(code.str());
}
TypeNode* CodeGenerator::gen_func_call(Type ret_type, std::string id, FuncCallNode* params) {
    CodeBuffer &cb = CodeBuffer::instance();
    std::stringstream code;
    std::string real_ret_type = ret_type != _VOID ? "i32" : "void";
    TypeNode* call_node = new TypeNode(ret_type);
    call_node->is_func_call = true;
    std::string reg = cb.freshVar();
    ret_type == _VOID ? code << "call " : code << reg << " = call ";
    code << real_ret_type << " @" << id << "(";
    TypeNode *tmp = params->types;
    call_node->place = reg;
    if(tmp==nullptr){
        int x = cb.emit("br label @");
        std::string call_label = cb.genLabel();
        cb.bpatch(cb.makelist({x, FIRST}), call_label);
        call_node->start_label = call_label;
        call_node->end_label = call_label;
    }
    TypeNode* last_exp;
    std::string param_type = id == "print" ? "i8*" : "i32";
    while (tmp != nullptr) {
    code << param_type << " " << tmp->place;
    last_exp = tmp;
    tmp = tmp->next;
    if (tmp != nullptr) {
        code << ", ";
    }

}
    code << ")" << std::endl;

    cb.emit(code.str());
    if(params->types != nullptr){
        call_node->start_label = params->types->start_label;
        call_node->start_list = params->types->start_list;
        call_node->end_label = last_exp->start_label;
    }

    if(call_node != nullptr) {
    // cb.bpatch(call_node->start_list, call_node->start_label);
        if (ret_type == _BOOL) {
            std::stringstream trunc_code;
            std::string cond_reg = cb.freshVar();
            trunc_code << cond_reg << " = trunc i32 " << reg << " to i1";
            cb.emit(trunc_code.str());
            int loc = cb.emit("br i1 " + cond_reg + ", label @, label @");
            call_node->true_list = cb.makelist({loc, FIRST});
            call_node->false_list = cb.makelist({loc, SECOND});
            call_node->place = reg;
        }
    }
    return call_node;
}
void CodeGenerator::gen_ret_with_exp(TypeNode* ret_exp){
    CodeBuffer& cb = CodeBuffer::instance();
    std::string ret = ret_exp->type != _VOID ? "i32" : "void";
    std::stringstream  code;
    if(ret_exp->type==_BOOL){
        //APART OF GEN_BOOL_LABELS
        std::string base_addr = base_addr_vec.back();
        CodeBuffer& cb  = CodeBuffer::instance();
        std::string true_label = cb.genLabel();
        int next_label_true = cb.emit("br label @");
        std::string false_label = cb.genLabelPhi();
        int next_label_false = cb.emit("br label @");
        std::string next_label = cb.genLabelPhi();

        std::vector<pair<int,BranchLabelIndex>> next_label_list = CodeBuffer::merge(CodeBuffer::makelist({next_label_true, FIRST}),
                                                                                    CodeBuffer::makelist({next_label_false,FIRST}));

        cb.bpatch(ret_exp->true_list, true_label);
        cb.bpatch(ret_exp->false_list, false_label);
        cb.bpatch(next_label_list, next_label);
        std::string reg = cb.freshVar();
        cb.emit(reg +" = phi i32 [1" +", %" + true_label + "] , [0" + " , %" + false_label+"]");
        ret_exp->place = reg;
    }
    code << "ret " << ret << " " << ret_exp->place;
    cb.emit(code.str());
}

void CodeGenerator::gen_ret(TypeNode* ret_type){
    //IN THE END OF EACH FUNCTION.

    std::string ret = ret_type->type != _VOID ? "i32 0" : "void";
    CodeBuffer::instance().emit("ret " + ret + "\n}");
    base_addr_vec.pop_back();
}
void CodeGenerator::gen_string(TypeNode* e, std::string value){
    static int counter  = 0;
    CodeBuffer& cb  = CodeBuffer::instance();
    std::stringstream string_decl;
    std::string root = value.substr(1, value.length()-2);
    int size = root.length() + 1;
    string_decl << "@s" << to_string(counter) <<" = " << "constant [" << size << " x i8] c\"" << root << "\\00\"";
    cb.emitGlobal(string_decl.str());

    std::stringstream code;
    e->place = cb.freshVar();
    code << e->place << " = " << "getelementptr " << "[" << size << " x i8], [" <<  size << " x i8]* @s" << counter <<", i32 0, i32 0";
    cb.emit(code.str());
    counter++;
    //fix_start_label(e);
}
void CodeGenerator::gen_tri(TypeNode *e_true, NarkerNode* after_e1, TypeNode* e_bool, TypeNode* e_false, TypeNode* e_res){
    if(e_false->type==_BOOL){
        cg.gen_bool_for_tri(e_false);
    }
    CodeBuffer& cb = CodeBuffer::instance();
    NarkerNode* jmp_to_end_label = new NarkerNode();
    std::string to_start_label = cb.genLabel();

    NarkerNode* middle_jmp = new NarkerNode();
    std::string middle_label = cb.genLabel();

    NarkerNode* jmp_to_bool_exp = new NarkerNode();
    std::string end_label = cb.genLabelPhi();
    if(e_true->type==_BOOL){

    }
    cb.bpatch(e_true->start_list, to_start_label);
    cb.bpatch(e_false->start_list, e_false->start_label);
    cb.bpatch(e_bool->true_list, e_true->start_label);
    cb.bpatch(e_bool->false_list, e_false->start_label);
    cb.bpatch(e_bool->start_list, e_bool->start_label);
    std::vector<pair<int, BranchLabelIndex>> all_jumps_to_end = cb.merge(after_e1->next_list, jmp_to_end_label->next_list);
    cb.bpatch(all_jumps_to_end, end_label);
    cb.bpatch(jmp_to_bool_exp->next_list, e_bool->start_label);
    e_res->place = cb.freshVar();
    cb.bpatch(e_true->start_list, to_start_label);
    std::string phi_true_label = e_true->is_tri ? e_true->start_label_tri: e_true->end_label!=""? e_true->end_label:e_true->start_label;
    std::string phi_false_label = e_false->is_tri ? e_false->start_label_tri: e_false->end_label!=""? e_false->end_label:e_false->start_label;
    std::string type = e_res->type==_STRING? "i8*":"i32";
    cb.emit(e_res->place +" = phi " + type + " [" +e_true->place +", %" + phi_true_label + "] , [" + e_false->place +  " , %" + phi_false_label+"]");
    e_res->start_label_tri = end_label;
    e_res->start_label = middle_label;
    e_res->start_list = middle_jmp->next_list;
    e_res->is_tri = true;
}
void CodeGenerator::gen_not_exp(TypeNode *e){
    vector<pair<int, BranchLabelIndex>> tmp = e->true_list;
    e->true_list = e->false_list;
    e->false_list = tmp;
}
void CodeGenerator::gen_func_call_param(TypeNode* e){
    CodeBuffer& cb = CodeBuffer::instance();
    if (e->type==_BOOL){
        std::string true_label = cb.genLabel();
        int loc1 = cb.emit("br label @");
        std::string false_label = cb.genLabel();
        int loc2 = cb.emit("br label @");
        std::string phi_label = cb.genLabelPhi();
        cb.bpatch(e->true_list, true_label);
        cb.bpatch(e->false_list, false_label);
        cb.bpatch(cb.makelist({loc1, FIRST}), phi_label);
        cb.bpatch(cb.makelist({loc2, FIRST}), phi_label);

        std::string reg = cb.freshVar();
        e->place = reg;
        cb.emit(reg + " = phi i32 [1" + ", %" + true_label + "] , [0" + " , %" + false_label + "]");

    }
   // cb.bpatch(e->start_list,e->start_label);
}
void CodeGenerator::connect_explist(TypeNode *e_list) {
    TypeNode* s = e_list->next;//atleast one parameter;
    //One paramter still needs to be Backpatched./
    while(s!=nullptr){
        fix_normal_exp(s);
        s=s->next;
    }
}
void CodeGenerator::gen_cast(TypeNode* old_e, TypeNode *new_e){
    CodeBuffer& cb = CodeBuffer::instance();
    new_e->start_list = old_e->start_list;
    new_e->true_list = old_e->true_list;
    new_e->false_list = old_e->false_list;
    int loc = cb.emit("br label @");
    new_e->end_label = cb.genLabel();
    new_e->start_label = old_e->start_label;
    cb.bpatch(cb.makelist({loc, FIRST}), new_e->end_label);
    if (old_e->type==_INT && new_e->type==_BYTE){
        std::string old_var = old_e->place;
        new_e->place = cb.freshVar();
        unsigned int mask = 0xff; // get only 8 bits
        std::stringstream casting_code;
        casting_code << new_e->place << " = and i32 " << old_var << ", " << mask;
        cb.emit(casting_code.str());
    }else{
        new_e->place = old_e->place;
    }
}
void CodeGenerator::gen_bool_for_tri(TypeNode* e){
    CodeBuffer& cb = CodeBuffer::instance();
    std::string true_label = cb.genLabel();
    int loc1 = cb.emit("br label @");
    std::string false_label = cb.genLabel();
    int loc2 = cb.emit("br label @");
    std::string phi_label = cb.genLabelPhi();
    cb.bpatch(e->true_list, true_label);
    cb.bpatch(e->false_list, false_label);
    cb.bpatch(cb.makelist({loc1, FIRST}), phi_label);
    cb.bpatch(cb.makelist({loc2, FIRST}), phi_label);
    std::string reg = cb.freshVar();
    e->place = reg;
    cb.emit(reg + " = phi i32 [1" + ", %" + true_label + "] , [0" + " , %" + false_label + "]");
    e->end_label=phi_label;
}
