#include "bp.hpp"
#include "types.h"
class CodeGenerator {
    CodeGenerator();
    CodeGenerator(CodeGenerator const&);
    void operator=(CodeGenerator const&);
public:
    static CodeGenerator &instance();

    void gen_exp(TypeNode *e, std::string value);

    void gen_exp_add(TypeNode* e_res, TypeNode *e1, TypeNode *e2);

};