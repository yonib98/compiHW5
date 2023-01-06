#ifndef _TYPES
#define _TYPES

#include <string>
#include <vector>
using namespace std;

enum Type{
    _INT,
    _BYTE,
    _BOOL,
    _STRING,
    _VOID,
};
struct Expression{
    std::string place;
};
struct TypeNode: public Expression {
    Type type;
    TypeNode* next;
    TypeNode* back;
    TypeNode(Type type): type(type), next(nullptr), back(nullptr) {};
};
struct IdNode: public Expression{
    string id;
    IdNode(string id): id(id){};
};


struct NumNode {
    string value;
    NumNode(string value): value(value) {}
};

struct NodeParams{
    Type type;
    string id;
    NodeParams* next;
    NodeParams* back;
    NodeParams(TypeNode* type, IdNode* id): type(type->type), id(id->id), next(nullptr), back(nullptr) {};
};

struct FuncCallNode{
    IdNode* id;
    TypeNode* types;
    FuncCallNode(IdNode* id, TypeNode* types): id(id), types(types) {};
    FuncCallNode(IdNode* id) : id(id), types(nullptr){};
};

struct Scope{
    //record
    //1 -> local param -> offset +
    //2 -> func arg -> offset -
    //3 -> func -> offset = 0 (na) but has params and is_func == true
    struct Record{
        Type type;
        int offset;
        string id;
        NodeParams* params_head;
        bool is_func;
        Record(Type type, int offset, string id): type(type), offset(offset), id(id), params_head(nullptr), is_func(
                false){};
        Record(Type type, string id, NodeParams* params_head): type(type), offset(0), id(id), params_head(params_head), is_func(true){};
    };
    vector<Record*> records;
    void insert_local_param(Type type, int offset, string id){
        records.push_back(new Record(type, offset, id));
    }
    void insert_func(Type ret_type, string id, NodeParams* params){
        records.push_back(new Record(ret_type, id, params));
    }
    void push_params(NodeParams* params, int line_no);
};
struct SymTable{
    vector<Scope> scopes;
    vector<int> offsets;
    Scope::Record* current_func;
    bool is_main = false;
    int while_scope = 0;
    void push_scope();
    Scope& top_scope();
    void pop_scope();
    void insert_symbol(TypeNode* type_n, IdNode* id_n, int lineno);
    void check_symbol(IdNode* id_n, TypeNode* type_n, int lineno);
    void insert_and_check_symbol(TypeNode* type_n, IdNode* id_n, TypeNode* exp_n, int lineno);
    void create_func(TypeNode* return_type, IdNode* id, NodeParams* params,  int lineno);
    void check_func(FuncCallNode* call_n, int line_no);
    void check_return(int line_no);
    void check_return(Type type, int line_no);
    void check_exp_is_bool(TypeNode* type_n, int line_no);
    void check_break(int line_no);
    void check_continue(int line_no);
    void check_relop(TypeNode* type_n1, TypeNode* type_n2, int line_no);
    void check_casting(TypeNode* type_n1, TypeNode* type_n2, int line_no);
    void check_logical_op(TypeNode* type_n1, TypeNode* type_n2, int line_no);
    void check_logical_op(TypeNode* type_n1, int line_no);
    void check_byte(NumNode* num_n, int line_no);

    TypeNode* find_type_from_tri(TypeNode* type_n1, TypeNode* type_n2, TypeNode* type_n3, int line_no);
    TypeNode* find_type_from_add_min(TypeNode* type_n1, TypeNode* type_n2, int line_no);
    TypeNode* find_type_from_mul_div(TypeNode* type_n1, TypeNode* type_n2, int line_no);
    TypeNode* find_type_from_id(IdNode* id_n, int line_no);
    TypeNode* find_type_from_call_func(FuncCallNode* call_n, int line_no);
    SymTable(): is_main(false), while_scope(false) {
        scopes.push_back(Scope());
        offsets.push_back(0);
    };
};


void start_program();
void  end_program(int yychar, int yyeof, int line_no);


#endif

