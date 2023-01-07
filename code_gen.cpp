#include "code_gen.h"
#include <iostream>
#include <fstream>
#include<stdlib.h>

//DEBUG REMOVE BEFORE SUBMITTING
void debug_lli(){
    CodeBuffer& bf = CodeBuffer::instance();
    std::string code = bf.getCode();
    std::ofstream file("code.txt");
    file << code;
    file.close();
    system("./llvm.py code.txt");
}
CodeGenerator::CodeGenerator() {};
CodeGenerator& CodeGenerator::instance(){
    static CodeGenerator inst;//only instance
    return inst;
}
void CodeGenerator::gen_exp(TypeNode* e, std::string value){
    CodeBuffer& bf  = CodeBuffer::instance();
    e->place = bf.freshVar();
    std::string code = e->place +" = add i32 0, " + value;
    bf.emit(code);
}
void CodeGenerator::gen_exp_add(TypeNode* e_res, TypeNode* e1, TypeNode* e2){
    CodeBuffer& bf  = CodeBuffer::instance();
    e_res->place = bf.freshVar();
    std::string type;
    if (e_res.type == _INT){
        type = "i32";
    }
    else if (e_res.type == _BYTE){
        type - "i8;"
    }
    else{
        std:: << "Something went wrong in types with types of gen_exp_add" << std::endl;
        exit(0);
    }
    std::string code = e_res->place +" = add " + type + e1->place +", " + e2->place;
    bf.emit(code);
    debug_lli();

}
