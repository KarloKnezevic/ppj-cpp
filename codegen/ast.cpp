#include <cstdio>

#include <set>
using namespace std;

#include "ast.h"
#include "types.h"
#include "parse.h"
#include "symtable.h"
#include "emit.h"

void ASTree::check(bool test, string msg = "") {
  if (!test) {
    if (msg != "") {
      cerr << msg << endl;
    } else {
      cerr << this->token << " ::=";

      for (vector<ASTree*>::iterator it = adj.begin(); it != adj.end(); ++it) {
        ASTree *node = *it;
        cerr << " " << node->token;
        if (node->token[0] != '<') {
          cerr << "(" << node->line << "," << node->source << ")";
        }
      }
      cerr << endl;
    }

    exit(1);
  }
}

struct terminal : ASTree {
  void dfs(SymTable *sym) {}
};

namespace primarni_izraz {
  struct IDN : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      string name = t->source;

      ASTree *s = sym->get(name);
      check(s);

      type = s->type;
      fun = s->fun;
      l_value = !(type & (M_FUNCTION|M_ARRAY|M_CONST));
      is_ptr = true;

      if (!(type & M_FUNCTION)) {
        if (s->is_global_decl) {
          emit("MOVE G_%s, R6", name.c_str());
        } else {
          emit("MOVE R5, R6");
          emit("SUB R6, %%D %d, R6", s->stack_pos - sym->base_pointer);
        }
      }
    }
  };

  struct BROJ : ASTree {
    void dfs(SymTable *sym) {
      string s = adj[0]->source;
      int num;
      check(parseInt(s, num));

      type = M_INT;
      l_value = false;
      is_ptr = false;

      emit("MOVE %%D %d, R6", num >> 16);
      emit("SHL R6, %%D 16, R6");
      emit("OR R6, %%D %d, R6", num & 0xFFFF);
    }
  };

  struct ZNAK : ASTree {
    void dfs(SymTable *sym) {
      string s = adj[0]->source;
      int num;
      check(parseChar(s, num));

      type = M_CHAR;
      l_value = false;
      is_ptr = false;

      emit("MOVE %%D %d, R6", num);
    }
  };

  struct NIZ_ZNAKOVA : ASTree {
    void dfs(SymTable *sym) {
      string s = adj[0]->source;
      vector<int> values;
      check(parseString(s, values));

      arg_types = new vector<int>(values.size(), M_CHAR);
      type = M_CHAR | M_CONST | M_ARRAY;
      l_value = false;
      is_ptr = true;

      string l_str = makeLabel();
      string l_out = makeLabel();
      emit("JR %s", l_out.c_str());
      emitLabel(l_str.c_str());
      for (vector<int>::iterator it = values.begin(); it != values.end(); ++it ) {
        emit("DW %%D %d", *it);
      }
      emitLabel(l_out.c_str());
      emit("MOVE %s, R6", l_str.c_str());
    }
  };

  struct L_ZAGRADA__izraz__D_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[1];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };
}

namespace postfiks_izraz {
  struct primarni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct postfiks_izraz__L_UGL_ZAGRADA__izraz__D_UGL_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_array = adj[0];
      ASTree *t_index = adj[2];

      t_array->dfs(sym);
      check(t_array->type & M_ARRAY);
      emit("LOAD R6, (R6)");
      emit("PUSH R6");

      t_index->dfs(sym);
      check(is_convertible_implicit(t_index->type, M_INT));
      if (t_index->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("POP R0");
      emit("SHL R6, 2, R6");
      emit("ADD R6, R0, R6");

      type = t_array->type ^ M_ARRAY;
      l_value = !(t_array->type & M_CONST);
      is_ptr = true;
    }
  };

  struct postfiks_izraz__L_ZAGRADA__D_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      check(t->type == M_FUNCTION);
      check(t->fun->args.empty());

      type = t->fun->type;
      l_value = false;
      is_ptr = false;

      while (!t->adj.empty()) {
        t = t->adj[0];
      }
      emit("CALL FUN_%s", t->source.c_str());
    }
  };

  struct postfiks_izraz__L_ZAGRADA__lista_argumenata__D_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_izraz = adj[0];
      ASTree *t_lista = adj[2];

      t_izraz->dfs(sym);
      t_lista->dfs(sym);

      check(t_izraz->type == M_FUNCTION);

      vector<int> &v1 = *t_lista->arg_types;
      vector<int> &v2 = t_izraz->fun->args;
      check(v1.size() == v2.size());
      for (size_t i = 0; i < v1.size(); ++i) {
        check(is_convertible_implicit(v1[i], v2[i]));
      }

      type = t_izraz->fun->type;
      l_value = false;
      is_ptr = false;

      while (!t_izraz->adj.empty()) {
        t_izraz = t_izraz->adj[0];
      }
      emit("CALL FUN_%s", t_izraz->source.c_str());
      emit("ADD SP, %%D %d, SP", v1.size() * 4);
    }
  };

  struct Base : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);
      check(t->l_value);
      check(is_convertible_implicit(t->type, M_INT));

      type = M_INT;
      l_value = false;
      is_ptr = false;

      emit("LOAD R0, (R6)");
      compute();
      emit("STORE R1, (R6)");
      emit("MOVE R0, R6");
    }

    virtual void compute() = 0;
  };

  struct postfiks_izraz__OP_INC : Base {
    void compute() {
      emit("ADD R0, 1, R1");
    }
  };

  struct postfiks_izraz__OP_DEC : Base {
    void compute() {
      emit("SUB R0, 1, R1");
    }
  };
}

namespace lista_argumenata {
  struct izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      arg_types = new vector<int>;
      arg_types->push_back(t->type);

      if (t->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("PUSH R6");
    }
  };

  struct lista_argumenata__ZAREZ__izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_lista = adj[0];
      ASTree *t_izraz = adj[2];

      t_lista->dfs(sym);
      arg_types = t_lista->arg_types;

      t_izraz->dfs(sym);
      arg_types->push_back(t_izraz->type);

      if (t_izraz->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("PUSH R6");
    }
  };
}

namespace unarni_izraz {
  struct postfiks_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct Base : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[1];

      t->dfs(sym);
      check(t->l_value);
      check(is_convertible_implicit(t->type, M_INT));

      type = M_INT;
      l_value = false;
      is_ptr = false;

      emit("LOAD R0, (R6)");
      compute();
      emit("STORE R0, (R6)");
      emit("MOVE R0, R6");
    }

    virtual void compute() = 0;
  };

  struct OP_INC__unarni_izraz : Base {
    void compute() {
      emit("ADD R0, 1, R0");
    }
  };

  struct OP_DEC__unarni_izraz : Base {
    void compute() {
      emit("SUB R0, 1, R0");
    }
  };

  struct unarni_operator__cast_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_op = adj[0];
      ASTree *t_izraz = adj[1];

      t_izraz->dfs(sym);
      check(is_convertible_implicit(t_izraz->type, M_INT));
      if (t_izraz->is_ptr) {
        emit("LOAD R6, (R6)");
      }

      type = M_INT;
      l_value = false;
      is_ptr = false;

      t_op->dfs(sym);
    }
  };
}

namespace unarni_operator {
  struct PLUS : ASTree {
    void dfs(SymTable *sym) {}
  };

  struct MINUS : ASTree {
    void dfs(SymTable *sym) {
      emit("MOVE 0, R0");
      emit("SUB R0, R6, R6");
    }
  };

  struct OP_TILDA : ASTree {
    void dfs(SymTable *sym) {
      emit("MOVE -1, R0");
      emit("XOR R0, R6, R6");
    }
  };

  struct OP_NEG : ASTree {
    void dfs(SymTable *sym) {
      emitIntToBool();
      emit("XOR R6, 1, R6");
    }
  };
}

namespace cast_izraz {
  struct unarni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct L_ZAGRADA__ime_tipa__D_ZAGRADA__cast_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_tip = adj[1];
      ASTree *t_cast = adj[3];

      t_tip->dfs(sym);
      t_cast->dfs(sym);
      if (t_cast->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      check(is_convertible_explicit(t_cast->type, t_tip->type));

      type = t_tip->type;
      fun = t_tip->fun;
      l_value = false;
      is_ptr = false;

      if (type & M_CHAR) {
        emit("AND R6, 0FF, R6");
      }
    }
  };
}

namespace ime_tipa {
  struct specifikator_tipa : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
    }
  };

  struct KR_CONST__specifikator_tipa : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[1];
      t->dfs(sym);
      check(!(t->type & M_VOID));

      type = t->type | M_CONST;
      fun = t->fun;
    }
  };
}

namespace specifikator_tipa {
  struct KR_VOID : ASTree {
    void dfs(SymTable *sym) {
      type = M_VOID;
    }
  };

  struct KR_CHAR : ASTree {
    void dfs(SymTable *sym) {
      type = M_CHAR;
    }
  };

  struct KR_INT : ASTree {
    void dfs(SymTable *sym) {
      type = M_INT;
    }
  };
}

namespace multiplikativni_izraz {
  struct cast_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct Base : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t1 = adj[0];
      ASTree *t2 = adj[2];

      t1->dfs(sym);
      check(is_convertible_implicit(t1->type, M_INT));
      if (t1->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("PUSH R6");

      t2->dfs(sym);
      check(is_convertible_implicit(t2->type, M_INT));
      if (t2->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("PUSH R6");

      type = M_INT;
      l_value = false;
      is_ptr = false;

      compute();
      emit("POP R0");
      emit("POP R0");
    }

    virtual void compute() = 0;
  };

  struct multiplikativni_izraz__OP_PUTA__cast_izraz : Base {
    void compute() {
      emit("CALL MUL");
    }
  };

  struct multiplikativni_izraz__OP_DIJELI__cast_izraz : Base {
    void compute() {
      emit("CALL DIV");
    }
  };

  struct multiplikativni_izraz__OP_MOD__cast_izraz : Base {
    void compute() {
      emit("CALL MOD");
    }
  };
}

struct BaseArithmetic : ASTree {
  void dfs(SymTable *sym) {
    ASTree *t1 = adj[0];
    ASTree *t2 = adj[2];

    t1->dfs(sym);
    check(is_convertible_implicit(t1->type, M_INT));
    if (t1->is_ptr) {
      emit("LOAD R6, (R6)");
    }
    emit("PUSH R6");

    t2->dfs(sym);
    check(is_convertible_implicit(t2->type, M_INT));
    if (t2->is_ptr) {
      emit("LOAD R6, (R6)");
    }

    type = M_INT;
    l_value = false;
    is_ptr = false;

    emit("POP R0");
    compute();
  }

  virtual void compute() = 0;
};

namespace aditivni_izraz {
  struct multiplikativni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct aditivni_izraz__PLUS__multiplikativni_izraz : BaseArithmetic {
    void compute() {
      emit("ADD R0, R6, R6");
    }
  };

  struct aditivni_izraz__MINUS__multiplikativni_izraz : BaseArithmetic {
    void compute() {
      emit("SUB R0, R6, R6");
    }
  };
}

struct BaseComparison : ASTree {
  void dfs(SymTable *sym) {
    ASTree *t1 = adj[0];
    ASTree *t2 = adj[2];

    t1->dfs(sym);
    check(is_convertible_implicit(t1->type, M_INT));
    if (t1->is_ptr) {
      emit("LOAD R6, (R6)");
    }
    emit("PUSH R6");

    t2->dfs(sym);
    check(is_convertible_implicit(t2->type, M_INT));
    if (t2->is_ptr) {
      emit("LOAD R6, (R6)");
    }

    string l_out = makeLabel();
    emit("POP R0");
    emit("CMP R0, R6");
    emit("MOVE 1, R6");
    emit("JR_%s %s", cond(), l_out.c_str());
    emit("MOVE 0, R6");
    emitLabel(l_out.c_str());

    type = M_INT;
    l_value = false;
    is_ptr = false;
  }

  virtual const char *cond() = 0;
};

namespace odnosni_izraz {
  struct aditivni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct odnosni_izraz__OP_LT__aditivni_izraz : BaseComparison {
    const char *cond() {
      return "SLT";
    }
  };

  struct odnosni_izraz__OP_GT__aditivni_izraz : BaseComparison {
    const char *cond() {
      return "SGT";
    }
  };

  struct odnosni_izraz__OP_LTE__aditivni_izraz : BaseComparison {
    const char *cond() {
      return "SLE";
    }
  };

  struct odnosni_izraz__OP_GTE__aditivni_izraz : BaseComparison {
    const char *cond() {
      return "SGE";
    }
  };
}

namespace jednakosni_izraz {
  struct odnosni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct jednakosni_izraz__OP_EQ__odnosni_izraz : BaseComparison {
    const char *cond() {
      return "EQ";
    }
  };

  struct jednakosni_izraz__OP_NEQ__odnosni_izraz : BaseComparison {
    const char *cond() {
      return "NE";
    }
  };
}

namespace bin_i_izraz {
  struct jednakosni_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct bin_i_izraz__OP_BIN_I__jednakosni_izraz : BaseArithmetic {
    void compute() {
      emit("AND R0, R6, R6");
    }
  };
}

namespace bin_xili_izraz {
  struct bin_i_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct bin_xili_izraz__OP_BIN_XILI__bin_i_izraz : BaseArithmetic {
    void compute() {
      emit("XOR R0, R6, R6");
    }
  };
}

namespace bin_ili_izraz {
  struct bin_xili_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct bin_ili_izraz__OP_BIN_ILI__bin_xili_izraz : BaseArithmetic {
    void compute() {
      emit("OR R0, R6, R6");
    }
  };
}

struct BaseLogic : ASTree {
  void dfs(SymTable *sym) {
    ASTree *t1 = adj[0];
    ASTree *t2 = adj[2];

    t1->dfs(sym);
    check(is_convertible_implicit(t1->type, M_INT));
    if (t1->is_ptr) {
      emit("LOAD R6, (R6)");
    }
    emitIntToBool();

    string l_out = makeLabel();
    emit("CMP R6, 0");
    emit("JP_%s %s", jump_test().c_str(), l_out.c_str());

    t2->dfs(sym);
    if (t2->is_ptr) {
      emit("LOAD R6, (R6)");
    }
    check(is_convertible_implicit(t2->type, M_INT));
    emitIntToBool();

    emitLabel(l_out.c_str());

    type = M_INT;
    l_value = false;
    is_ptr = false;
  }

  virtual string jump_test() = 0;
};

namespace log_i_izraz {
  struct bin_ili_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct log_i_izraz__OP_I__bin_ili_izraz : BaseLogic {
    string jump_test() {
      return "EQ";
    }
  };
}

namespace log_ili_izraz {
  struct log_i_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct log_ili_izraz__OP_ILI__log_i_izraz : BaseLogic {
    string jump_test() {
      return "NE";
    }
  };
}

namespace izraz_pridruzivanja {
  struct log_ili_izraz : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct postfiks_izraz__OP_PRIDRUZI__izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_left = adj[0];
      ASTree *t_right = adj[2];

      t_left->dfs(sym);
      check(t_left->l_value);
      emit("PUSH R6");

      t_right->dfs(sym);
      check(is_convertible_implicit(t_right->type, t_left->type));
      if (t_right->is_ptr) {
        emit("LOAD R6, (R6)");
      }

      type = t_left->type;
      fun = t_left->fun;
      l_value = false;
      is_ptr = false;

      emit("POP R0");
      emit("STORE R6, (R0)");
    }
  };
}

namespace izraz {
  struct izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct izraz__ZAREZ__izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t1 = adj[0];
      ASTree *t2 = adj[2];

      t1->dfs(sym);
      t2->dfs(sym);

      type = t2->type;
      fun = t2->fun;
      l_value = false;
      is_ptr = t2->is_ptr;
    }
  };
}

namespace slozena_naredba {
  struct L_VIT_ZAGRADA__lista_naredbi__D_VIT_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      adj[1]->dfs(sym);
    }
  };

  struct L_VIT_ZAGRADA__lista_deklaracija__lista_naredbi__D_VIT_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      adj[1]->dfs(sym);
      adj[2]->dfs(sym);
    }
  };
}

namespace lista_naredbi {
  struct naredba : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
    }
  };

  struct lista_naredbi__naredba : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
      adj[1]->dfs(sym);
    }
  };
}

namespace naredba {
  struct slozena_naredba : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym->extend());

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };

  struct Base : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      l_value = t->l_value;
      is_ptr = t->is_ptr;
    }
  };
  struct izraz_naredba : Base {};
  struct naredba_grananja : Base {};
  struct naredba_petlje : Base {};
  struct naredba_skoka : Base {};
}

namespace izraz_naredba {
  struct TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      type = M_INT;
    }
  };

  struct izraz__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      type = t->type;
      fun = t->fun;
      is_ptr = t->is_ptr;
    }
  };
}

namespace naredba_grananja {
  struct KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_if = adj[2];
      ASTree *t_then = adj[4];

      t_if->dfs(sym);
      check(is_convertible_implicit(t_if->type, M_INT));
      if (t_if->is_ptr) {
        emit("LOAD R6, (R6)");
      }

      string l_out = makeLabel();
      emit("CMP R6, 0");
      emit("JR_EQ %s", l_out.c_str());

      t_then->dfs(sym);
      emitLabel(l_out.c_str());
    }
  };

  struct KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba__KR_ELSE__naredba : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_if = adj[2];
      ASTree *t_then = adj[4];
      ASTree *t_else = adj[6];

      t_if->dfs(sym);
      check(is_convertible_implicit(t_if->type, M_INT));
      if (t_if->is_ptr) {
        emit("LOAD R6, (R6)");
      }

      string l_out = makeLabel();
      emit("CMP R6, 0");
      emit("JR_EQ %s", l_out.c_str());

      t_then->dfs(sym);
      emitLabel(l_out.c_str());
      t_else->dfs(sym);
    }
  };
}

namespace naredba_petlje {
  struct KR_WHILE__L_ZAGRADA__izraz__D_ZAGRADA__naredba : ASTree {
    void dfs(SymTable *sym) {
      is_loop = true;
      ASTree *t_while = adj[2];
      ASTree *t_do = adj[4];

      string l_in = makeLabel();
      string l_out = makeLabel();
      sym->addLoop(l_in, l_out);

      emitLabel(l_in.c_str());
      t_while->dfs(sym);
      if (t_while->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("CMP R6, 0");
      emit("JR_EQ %s", l_out.c_str());

      check(is_convertible_implicit(t_while->type, M_INT));
      t_do->dfs(sym);
      emit("JR %s", l_in.c_str());

      fflush(stdout);
      emitLabel(l_out.c_str());
      sym->removeLoop();
    }
  };

  struct KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__D_ZAGRADA__naredba : ASTree {
    void dfs(SymTable *sym) {
      is_loop = true;
      ASTree *t_start = adj[2];
      ASTree *t_test = adj[3];
      ASTree *t_do = adj[5];

      t_start->dfs(sym);

      string l_test = makeLabel();
      string l_out = makeLabel();
      sym->addLoop(l_test, l_out);

      emitLabel(l_test.c_str());
      emit("MOVE 1, R6");
      t_test->dfs(sym);
      if (t_test->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      check(is_convertible_implicit(t_test->type, M_INT));
      emit("CMP R6, 0");
      emit("JR_EQ %s", l_out.c_str());

      t_do->dfs(sym);
      emit("JR %s", l_test.c_str());

      emitLabel(l_out.c_str());
      sym->removeLoop();
    }
  };

  struct KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__izraz__D_ZAGRADA__naredba : ASTree {
    void dfs(SymTable *sym) {
      is_loop = true;
      ASTree *t_start = adj[2];
      ASTree *t_test = adj[3];
      ASTree *t_step = adj[4];
      ASTree *t_do = adj[6];

      t_start->dfs(sym);

      string l_test = makeLabel();
      string l_step = makeLabel();
      string l_do = makeLabel();
      string l_out = makeLabel();
      sym->addLoop(l_step, l_out);

      emitLabel(l_test.c_str());
      emit("MOVE 1, R6");
      t_test->dfs(sym);
      if (t_test->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      check(is_convertible_implicit(t_test->type, M_INT));
      emit("CMP R6, 0");
      emit("JR_NE %s", l_do.c_str());
      emit("JR %s", l_out.c_str());

      emitLabel(l_step.c_str());
      t_step->dfs(sym);
      emit("JR %s", l_test.c_str());

      emitLabel(l_do.c_str());
      t_do->dfs(sym);
      emit("JR %s", l_step.c_str());

      emitLabel(l_out.c_str());
      sym->removeLoop();
    }
  };
}

namespace naredba_skoka {
  struct KR_CONTINUE__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      check(sym->insideLoop());
      emit("JR %s", sym->getContinue().c_str());
    }
  };

  struct KR_BREAK__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      check(sym->insideLoop());
      emit("JR %s", sym->getBreak().c_str());
    }
  };

  struct KR_RETURN__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      check(sym->fun);
      check(sym->fun->type == M_VOID);
      emitFunctionEnd();
    }
  };

  struct KR_RETURN__izraz__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[1];
      t->dfs(sym);
      if (t->is_ptr) {
        emit("LOAD R6, (R6)");
      }

      check(sym->fun);
      check(is_convertible_implicit(t->type, sym->fun->type));
      emitFunctionEnd();
    }
  };
}

namespace prijevodna_jedinica {
  struct vanjska_deklaracija : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
    }
  };

  struct prijevodna_jedinica__vanjska_deklaracija : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
      adj[1]->dfs(sym);
    }
  };
}

namespace vanjska_deklaracija {
  struct Base : ASTree {
    void dfs(SymTable *sym) {
      emit();
      adj[0]->dfs(sym);
    }
  };
  struct definicija_funkcije : Base {};
  struct deklaracija : Base {};
}

namespace definicija_funkcije {
  struct Base : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_tip = adj[0];
      ASTree *t_idn = adj[1];
      ASTree *t_naredba = adj[5];

      t_tip->dfs(sym);

      type = M_FUNCTION;
      fun = new Function;
      fun->type = t_tip->type;
      check(!(fun->type & M_CONST));

      string name = t_idn->source;
      ASTree *f = sym->get(name);

      emitLabel("FUN_%s", name.c_str());
      emitFunctionStart();

      sym = sym->extend();
      sym->fun = fun;
      init_args(sym);
      sym->push();
      sym->base_pointer = sym->push();

      check(fun_def.get(name) == NULL);
      if (f) {
        check(f->type == M_FUNCTION);
        check(*f->fun == *this->fun);
      }

      fun_declared.insert(make_pair(name, this->fun));
      fun_def.put(name, this);
      sym->dad->put(name, this);

      t_naredba->dfs(sym);

      emitFunctionEnd();
    }

    virtual void init_args(SymTable *sym) = 0;
  };

  struct ime_tipa__IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA__slozena_naredba : Base {
    void init_args(SymTable *sym) {}
  };

  struct ime_tipa__IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA__slozena_naredba : Base {
    void init_args(SymTable *sym) {
      ASTree *t = adj[3];
      t->dfs(sym);
      fun->args = *t->arg_types;
    }
  };
}

namespace lista_parametara {
  struct Base : ASTree {
    void addParam(SymTable *sym, ASTree *t) {
      arg_types->push_back(t->type);

      check(sym->getLocal(t->arg_name) == NULL);
      sym->put(t->arg_name, t);
      t->stack_pos = sym->push();
    }
  };

  struct deklaracija_parametra : Base {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->dfs(sym);

      arg_types = new vector<int>;
      addParam(sym, t);
    }
  };

  struct lista_parametara__ZAREZ__deklaracija_parametra : Base {
    void dfs(SymTable *sym) {
      ASTree *t_lista = adj[0];
      ASTree *t_param = adj[2];

      t_lista->dfs(sym);
      arg_types = t_lista->arg_types;

      t_param->dfs(sym);
      addParam(sym, t_param);
    }
  };
}

namespace deklaracija_parametra {
  struct ime_tipa__IDN : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_tip = adj[0];
      ASTree *t_idn = adj[1];

      t_tip->dfs(sym);
      type = t_tip->type;
      check(!(type & M_VOID));

      arg_name = t_idn->source;
    }
  };

  struct ime_tipa__IDN__L_UGL_ZAGRADA__D_UGL_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_tip = adj[0];
      ASTree *t_idn = adj[1];

      t_tip->dfs(sym);
      type = t_tip->type | M_ARRAY;
      check(!(type & M_VOID));

      arg_name = t_idn->source;
    }
  };
}

namespace lista_deklaracija {
  struct deklaracija : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
    }
  };

  struct lista_deklaracija__deklaracija : ASTree {
    void dfs(SymTable *sym) {
      adj[0]->dfs(sym);
      adj[1]->dfs(sym);
    }
  };
}

namespace deklaracija {
  struct ime_tipa__lista_init_deklaratora__TOCKAZAREZ : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_tip = adj[0];
      ASTree *t_lista = adj[1];

      if (sym->dad == NULL) {
        emitLabel(l_global.c_str());
      }

      t_tip->dfs(sym);
      t_lista->type_inherited = t_tip->type;
      t_lista->dfs(sym);

      if (sym->dad == NULL) {
        l_global = makeLabel();
        emit("JR %s", l_global.c_str());
      }
    }
  };
}

namespace lista_init_deklaratora {
  struct init_deklarator : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->type_inherited = type_inherited;
      t->dfs(sym);
    }
  };

  struct lista_init_deklaratora__ZAREZ__init_deklarator : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_lista = adj[0];
      ASTree *t_dekl = adj[2];

      t_lista->type_inherited = type_inherited;
      t_lista->dfs(sym);

      t_dekl->type_inherited = type_inherited;
      t_dekl->dfs(sym);
    }
  };
}

namespace init_deklarator {
  struct izravni_deklarator : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];
      t->type_inherited = type_inherited;
      t->dfs(sym);

      check(!(t->type & M_CONST));
    }
  };

  struct izravni_deklarator__OP_PRIDRUZI__inicijalizator : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_dekl = adj[0];
      ASTree *t_value = adj[2];

      t_dekl->type_inherited = type_inherited;
      t_dekl->dfs(sym);

      t_value->dfs(sym);

      if (t_dekl->type & M_ARRAY) {
        check(t_value->arg_types);

        vector<int> &v = *t_value->arg_types;
        check(v.size() <= (size_t) t_dekl->array_size);

        int elem_type = t_dekl->type & ~M_ARRAY & ~M_CONST;
        for (vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
          check(is_convertible_implicit(*it, elem_type));
        }
      } else {
        check(is_convertible_implicit(t_value->type, t_dekl->type));
      }
    }
  };
}

namespace izravni_deklarator {
  struct IDN : ASTree {
    void dfs(SymTable *sym) {
      string name = adj[0]->source;

      type = type_inherited;
      check(!(type & M_VOID));
      check(sym->getLocal(name) == NULL);

      sym->put(name, this);
      stack_pos = sym->push();
      is_global_decl = sym->dad == NULL;

      if (sym->dad) {
        emit("SUB SP, 4, SP");
      } else {
        string l_out = makeLabel();
        emit("JR %s", l_out.c_str());
        emitLabel("G_%s", name.c_str());
        emit("DW 0");
        emitLabel(l_out.c_str());
        emit("MOVE G_%s, R6", name.c_str());
        emit("PUSH R6");
      }
    }
  };

  struct IDN__L_UGL_ZAGRADA__BROJ__D_UGL_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_idn = adj[0];
      ASTree *t_broj = adj[2];

      check(!(type_inherited & M_VOID));
      type = type_inherited | M_ARRAY;

      string name = t_idn->source;
      check(sym->getLocal(name) == NULL);
      sym->put(name, this);
      is_global_decl = sym->dad == NULL;

      int num;
      check(parseInt(t_broj->source, num));
      check(num > 0 && num <= 1024);

      array_size = num;
      sym->push(array_size);
      stack_pos = sym->push();

      if (sym->dad) {
        emit("SUB SP, %%D %d, SP", array_size * 4);
        emit("MOVE SP, R0");
        emit("PUSH R0");
      } else {
        is_global_decl = true;

        string l_out = makeLabel();
        emit("JR %s", l_out.c_str());
        emitLabel("G_%s", name.c_str());
        emit("DW G_%s", name.c_str());
        emit("`DS %%D %d", array_size * 4);
        emitLabel(l_out.c_str());
        emit("LOAD R0, (G_%s)", name.c_str());
        emit("ADD R0, 4, R0");
        emit("STORE R0, (G_%s)", name.c_str());
      }
    }
  };

  struct IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      string name = adj[0]->source;

      if (ASTree *t = sym->getLocal(name)) {
        check(t->type == M_FUNCTION);
        check(t->fun->type == type_inherited && t->fun->args.empty());
      }

      type = M_FUNCTION;
      fun = new Function;
      fun->type = type_inherited;

      fun_declared.insert(make_pair(name, fun));
      sym->put(name, this);
      is_global_decl = sym->dad == NULL;
    }
  };

  struct IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_idn = adj[0];
      ASTree *t_lista = adj[2];

      t_lista->dfs(sym->extend());

      string name = t_idn->source;
      if (ASTree *t = sym->getLocal(name)) {
        check(t->type == M_FUNCTION);
        check(t->fun->type == type_inherited);
        check(t->fun->args == *t_lista->arg_types);
      }

      type = M_FUNCTION;
      fun = new Function;
      fun->type = type_inherited;
      fun->args = *t_lista->arg_types;

      fun_declared.insert(make_pair(name, fun));
      sym->put(name, this);
      is_global_decl = sym->dad == NULL;
    }
  };
}

namespace inicijalizator {
  struct izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];

      t->dfs(sym);
      if (t->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      type = t->type;
      fun = t->fun;

      if (type == (M_CONST|M_CHAR|M_ARRAY)) {
        while (t) {
          if (t->arg_types) {
            arg_types = t->arg_types;
            break;
          }

          if (t->adj.size() != 1) break;
          t = t->adj[0];
        }
      }

      if (sym->dad) {
        emit("STORE R6, (SP)");
      } else {
        emit("POP R0");
        emit("STORE R6, (R0)");
      }
    }
  };

  struct L_VIT_ZAGRADA__lista_izraza_pridruzivanja__D_VIT_ZAGRADA : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[1];
      t->dfs(sym);
      arg_types = t->arg_types;
    }
  };
}

namespace lista_izraza_pridruzivanja {
  struct izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t = adj[0];

      emit("PUSH R0");
      t->dfs(sym);
      if (t->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("POP R0");
      emit("STORE R6, (R0)");

      arg_types = new vector<int>;
      arg_types->push_back(t->type);
    }
  };

  struct lista_izraza_pridruzivanja__ZAREZ__izraz_pridruzivanja : ASTree {
    void dfs(SymTable *sym) {
      ASTree *t_lista = adj[0];
      ASTree *t_izraz = adj[2];

      t_lista->dfs(sym);
      emit("ADD R0, 4, R0");
      emit("PUSH R0");
      t_izraz->dfs(sym);
      if (t_izraz->is_ptr) {
        emit("LOAD R6, (R6)");
      }
      emit("POP R0");
      emit("STORE R6, (R0)");

      arg_types = t_lista->arg_types;
      arg_types->push_back(t_izraz->type);
    }
  };
}

ASTree *makeProduction(string token, string children) {
  if (isupper(token[0])) {
    return new terminal;
  }

  if (token == "primarni_izraz") {
    if (children == "IDN")
      return new primarni_izraz::IDN;
    if (children == "BROJ")
      return new primarni_izraz::BROJ;
    if (children == "ZNAK")
      return new primarni_izraz::ZNAK;
    if (children == "NIZ_ZNAKOVA")
      return new primarni_izraz::NIZ_ZNAKOVA;
    if (children == "L_ZAGRADA__izraz__D_ZAGRADA")
      return new primarni_izraz::L_ZAGRADA__izraz__D_ZAGRADA;
  }

  if (token == "postfiks_izraz") {
    if (children == "primarni_izraz")
      return new postfiks_izraz::primarni_izraz;
    if (children == "postfiks_izraz__L_UGL_ZAGRADA__izraz__D_UGL_ZAGRADA")
      return new postfiks_izraz::postfiks_izraz__L_UGL_ZAGRADA__izraz__D_UGL_ZAGRADA;
    if (children == "postfiks_izraz__L_ZAGRADA__D_ZAGRADA")
      return new postfiks_izraz::postfiks_izraz__L_ZAGRADA__D_ZAGRADA;
    if (children == "postfiks_izraz__L_ZAGRADA__lista_argumenata__D_ZAGRADA")
      return new postfiks_izraz::postfiks_izraz__L_ZAGRADA__lista_argumenata__D_ZAGRADA;
    if (children == "postfiks_izraz__OP_INC")
      return new postfiks_izraz::postfiks_izraz__OP_INC;
    if (children == "postfiks_izraz__OP_DEC")
      return new postfiks_izraz::postfiks_izraz__OP_DEC;
  }

  if (token == "lista_argumenata") {
    if (children == "izraz_pridruzivanja")
      return new lista_argumenata::izraz_pridruzivanja;
    if (children == "lista_argumenata__ZAREZ__izraz_pridruzivanja")
      return new lista_argumenata::lista_argumenata__ZAREZ__izraz_pridruzivanja;
  }

  if (token == "unarni_izraz") {
    if (children == "postfiks_izraz")
      return new unarni_izraz::postfiks_izraz;
    if (children == "OP_INC__unarni_izraz")
      return new unarni_izraz::OP_INC__unarni_izraz;
    if (children == "OP_DEC__unarni_izraz")
      return new unarni_izraz::OP_DEC__unarni_izraz;
    if (children == "unarni_operator__cast_izraz")
      return new unarni_izraz::unarni_operator__cast_izraz;
  }

  if (token == "unarni_operator") {
    if (children == "PLUS")
      return new unarni_operator::PLUS;
    if (children == "MINUS")
      return new unarni_operator::MINUS;
    if (children == "OP_TILDA")
      return new unarni_operator::OP_TILDA;
    if (children == "OP_NEG")
      return new unarni_operator::OP_NEG;
  }

  if (token == "cast_izraz") {
    if (children == "unarni_izraz")
      return new cast_izraz::unarni_izraz;
    if (children == "L_ZAGRADA__ime_tipa__D_ZAGRADA__cast_izraz")
      return new cast_izraz::L_ZAGRADA__ime_tipa__D_ZAGRADA__cast_izraz;
  }

  if (token == "ime_tipa") {
    if (children == "specifikator_tipa")
      return new ime_tipa::specifikator_tipa;
    if (children == "KR_CONST__specifikator_tipa")
      return new ime_tipa::KR_CONST__specifikator_tipa;
  }

  if (token == "specifikator_tipa") {
    if (children == "KR_VOID")
      return new specifikator_tipa::KR_VOID;
    if (children == "KR_CHAR")
      return new specifikator_tipa::KR_CHAR;
    if (children == "KR_INT")
      return new specifikator_tipa::KR_INT;
  }

  if (token == "multiplikativni_izraz") {
    if (children == "cast_izraz")
      return new multiplikativni_izraz::cast_izraz;
    if (children == "multiplikativni_izraz__OP_PUTA__cast_izraz")
      return new multiplikativni_izraz::multiplikativni_izraz__OP_PUTA__cast_izraz;
    if (children == "multiplikativni_izraz__OP_DIJELI__cast_izraz")
      return new multiplikativni_izraz::multiplikativni_izraz__OP_DIJELI__cast_izraz;
    if (children == "multiplikativni_izraz__OP_MOD__cast_izraz")
      return new multiplikativni_izraz::multiplikativni_izraz__OP_MOD__cast_izraz;
  }

  if (token == "aditivni_izraz") {
    if (children == "multiplikativni_izraz")
      return new aditivni_izraz::multiplikativni_izraz;
    if (children == "aditivni_izraz__PLUS__multiplikativni_izraz")
      return new aditivni_izraz::aditivni_izraz__PLUS__multiplikativni_izraz;
    if (children == "aditivni_izraz__MINUS__multiplikativni_izraz")
      return new aditivni_izraz::aditivni_izraz__MINUS__multiplikativni_izraz;
  }

  if (token == "odnosni_izraz") {
    if (children == "aditivni_izraz")
      return new odnosni_izraz::aditivni_izraz;
    if (children == "odnosni_izraz__OP_LT__aditivni_izraz")
      return new odnosni_izraz::odnosni_izraz__OP_LT__aditivni_izraz;
    if (children == "odnosni_izraz__OP_GT__aditivni_izraz")
      return new odnosni_izraz::odnosni_izraz__OP_GT__aditivni_izraz;
    if (children == "odnosni_izraz__OP_LTE__aditivni_izraz")
      return new odnosni_izraz::odnosni_izraz__OP_LTE__aditivni_izraz;
    if (children == "odnosni_izraz__OP_GTE__aditivni_izraz")
      return new odnosni_izraz::odnosni_izraz__OP_GTE__aditivni_izraz;
  }

  if (token == "jednakosni_izraz") {
    if (children == "odnosni_izraz")
      return new jednakosni_izraz::odnosni_izraz;
    if (children == "jednakosni_izraz__OP_EQ__odnosni_izraz")
      return new jednakosni_izraz::jednakosni_izraz__OP_EQ__odnosni_izraz;
    if (children == "jednakosni_izraz__OP_NEQ__odnosni_izraz")
      return new jednakosni_izraz::jednakosni_izraz__OP_NEQ__odnosni_izraz;
  }

  if (token == "bin_i_izraz") {
    if (children == "jednakosni_izraz")
      return new bin_i_izraz::jednakosni_izraz;
    if (children == "bin_i_izraz__OP_BIN_I__jednakosni_izraz")
      return new bin_i_izraz::bin_i_izraz__OP_BIN_I__jednakosni_izraz;
  }

  if (token == "bin_xili_izraz") {
    if (children == "bin_i_izraz")
      return new bin_xili_izraz::bin_i_izraz;
    if (children == "bin_xili_izraz__OP_BIN_XILI__bin_i_izraz")
      return new bin_xili_izraz::bin_xili_izraz__OP_BIN_XILI__bin_i_izraz;
  }

  if (token == "bin_ili_izraz") {
    if (children == "bin_xili_izraz")
      return new bin_ili_izraz::bin_xili_izraz;
    if (children == "bin_ili_izraz__OP_BIN_ILI__bin_xili_izraz")
      return new bin_ili_izraz::bin_ili_izraz__OP_BIN_ILI__bin_xili_izraz;
  }

  if (token == "log_i_izraz") {
    if (children == "bin_ili_izraz")
      return new log_i_izraz::bin_ili_izraz;
    if (children == "log_i_izraz__OP_I__bin_ili_izraz")
      return new log_i_izraz::log_i_izraz__OP_I__bin_ili_izraz;
  }

  if (token == "log_ili_izraz") {
    if (children == "log_i_izraz")
      return new log_ili_izraz::log_i_izraz;
    if (children == "log_ili_izraz__OP_ILI__log_i_izraz")
      return new log_ili_izraz::log_ili_izraz__OP_ILI__log_i_izraz;
  }

  if (token == "izraz_pridruzivanja") {
    if (children == "log_ili_izraz")
      return new izraz_pridruzivanja::log_ili_izraz;
    if (children == "postfiks_izraz__OP_PRIDRUZI__izraz_pridruzivanja")
      return new izraz_pridruzivanja::postfiks_izraz__OP_PRIDRUZI__izraz_pridruzivanja;
  }

  if (token == "izraz") {
    if (children == "izraz_pridruzivanja")
      return new izraz::izraz_pridruzivanja;
    if (children == "izraz__ZAREZ__izraz_pridruzivanja")
      return new izraz::izraz__ZAREZ__izraz_pridruzivanja;
  }

  if (token == "slozena_naredba") {
    if (children == "L_VIT_ZAGRADA__lista_naredbi__D_VIT_ZAGRADA")
      return new slozena_naredba::L_VIT_ZAGRADA__lista_naredbi__D_VIT_ZAGRADA;
    if (children == "L_VIT_ZAGRADA__lista_deklaracija__lista_naredbi__D_VIT_ZAGRADA")
      return new slozena_naredba::L_VIT_ZAGRADA__lista_deklaracija__lista_naredbi__D_VIT_ZAGRADA;
  }

  if (token == "lista_naredbi") {
    if (children == "naredba")
      return new lista_naredbi::naredba;
    if (children == "lista_naredbi__naredba")
      return new lista_naredbi::lista_naredbi__naredba;
  }

  if (token == "naredba") {
    if (children == "slozena_naredba")
      return new naredba::slozena_naredba;
    if (children == "izraz_naredba")
      return new naredba::izraz_naredba;
    if (children == "naredba_grananja")
      return new naredba::naredba_grananja;
    if (children == "naredba_petlje")
      return new naredba::naredba_petlje;
    if (children == "naredba_skoka")
      return new naredba::naredba_skoka;
  }

  if (token == "izraz_naredba") {
    if (children == "TOCKAZAREZ")
      return new izraz_naredba::TOCKAZAREZ;
    if (children == "izraz__TOCKAZAREZ")
      return new izraz_naredba::izraz__TOCKAZAREZ;
  }

  if (token == "naredba_grananja") {
    if (children == "KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba")
      return new naredba_grananja::KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba;
    if (children == "KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba__KR_ELSE__naredba")
      return new naredba_grananja::KR_IF__L_ZAGRADA__izraz__D_ZAGRADA__naredba__KR_ELSE__naredba;
  }

  if (token == "naredba_petlje") {
    if (children == "KR_WHILE__L_ZAGRADA__izraz__D_ZAGRADA__naredba")
      return new naredba_petlje::KR_WHILE__L_ZAGRADA__izraz__D_ZAGRADA__naredba;
    if (children == "KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__D_ZAGRADA__naredba")
      return new naredba_petlje::KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__D_ZAGRADA__naredba;
    if (children == "KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__izraz__D_ZAGRADA__naredba")
      return new naredba_petlje::KR_FOR__L_ZAGRADA__izraz_naredba__izraz_naredba__izraz__D_ZAGRADA__naredba;
  }

  if (token == "naredba_skoka") {
    if (children == "KR_CONTINUE__TOCKAZAREZ")
      return new naredba_skoka::KR_CONTINUE__TOCKAZAREZ;
    if (children == "KR_BREAK__TOCKAZAREZ")
      return new naredba_skoka::KR_BREAK__TOCKAZAREZ;
    if (children == "KR_RETURN__TOCKAZAREZ")
      return new naredba_skoka::KR_RETURN__TOCKAZAREZ;
    if (children == "KR_RETURN__izraz__TOCKAZAREZ")
      return new naredba_skoka::KR_RETURN__izraz__TOCKAZAREZ;
  }

  if (token == "prijevodna_jedinica") {
    if (children == "vanjska_deklaracija")
      return new prijevodna_jedinica::vanjska_deklaracija;
    if (children == "prijevodna_jedinica__vanjska_deklaracija")
      return new prijevodna_jedinica::prijevodna_jedinica__vanjska_deklaracija;
  }

  if (token == "vanjska_deklaracija") {
    if (children == "definicija_funkcije")
      return new vanjska_deklaracija::definicija_funkcije;
    if (children == "deklaracija")
      return new vanjska_deklaracija::deklaracija;
  }

  if (token == "definicija_funkcije") {
    if (children == "ime_tipa__IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA__slozena_naredba")
      return new definicija_funkcije::ime_tipa__IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA__slozena_naredba;
    if (children == "ime_tipa__IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA__slozena_naredba")
      return new definicija_funkcije::ime_tipa__IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA__slozena_naredba;
  }

  if (token == "lista_parametara") {
    if (children == "deklaracija_parametra")
      return new lista_parametara::deklaracija_parametra;
    if (children == "lista_parametara__ZAREZ__deklaracija_parametra")
      return new lista_parametara::lista_parametara__ZAREZ__deklaracija_parametra;
  }

  if (token == "deklaracija_parametra") {
    if (children == "ime_tipa__IDN")
      return new deklaracija_parametra::ime_tipa__IDN;
    if (children == "ime_tipa__IDN__L_UGL_ZAGRADA__D_UGL_ZAGRADA")
      return new deklaracija_parametra::ime_tipa__IDN__L_UGL_ZAGRADA__D_UGL_ZAGRADA;
  }

  if (token == "lista_deklaracija") {
    if (children == "deklaracija")
      return new lista_deklaracija::deklaracija;
    if (children == "lista_deklaracija__deklaracija")
      return new lista_deklaracija::lista_deklaracija__deklaracija;
  }

  if (token == "deklaracija") {
    if (children == "ime_tipa__lista_init_deklaratora__TOCKAZAREZ")
      return new deklaracija::ime_tipa__lista_init_deklaratora__TOCKAZAREZ;
  }

  if (token == "lista_init_deklaratora") {
    if (children == "init_deklarator")
      return new lista_init_deklaratora::init_deklarator;
    if (children == "lista_init_deklaratora__ZAREZ__init_deklarator")
      return new lista_init_deklaratora::lista_init_deklaratora__ZAREZ__init_deklarator;
  }

  if (token == "init_deklarator") {
    if (children == "izravni_deklarator")
      return new init_deklarator::izravni_deklarator;
    if (children == "izravni_deklarator__OP_PRIDRUZI__inicijalizator")
      return new init_deklarator::izravni_deklarator__OP_PRIDRUZI__inicijalizator;
  }

  if (token == "izravni_deklarator") {
    if (children == "IDN")
      return new izravni_deklarator::IDN;
    if (children == "IDN__L_UGL_ZAGRADA__BROJ__D_UGL_ZAGRADA")
      return new izravni_deklarator::IDN__L_UGL_ZAGRADA__BROJ__D_UGL_ZAGRADA;
    if (children == "IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA")
      return new izravni_deklarator::IDN__L_ZAGRADA__KR_VOID__D_ZAGRADA;
    if (children == "IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA")
      return new izravni_deklarator::IDN__L_ZAGRADA__lista_parametara__D_ZAGRADA;
  }

  if (token == "inicijalizator") {
    if (children == "izraz_pridruzivanja")
      return new inicijalizator::izraz_pridruzivanja;
    if (children == "L_VIT_ZAGRADA__lista_izraza_pridruzivanja__D_VIT_ZAGRADA")
      return new inicijalizator::L_VIT_ZAGRADA__lista_izraza_pridruzivanja__D_VIT_ZAGRADA;
  }

  if (token == "lista_izraza_pridruzivanja") {
    if (children == "izraz_pridruzivanja")
      return new lista_izraza_pridruzivanja::izraz_pridruzivanja;
    if (children == "lista_izraza_pridruzivanja__ZAREZ__izraz_pridruzivanja")
      return new lista_izraza_pridruzivanja::lista_izraza_pridruzivanja__ZAREZ__izraz_pridruzivanja;
  }

  cerr << "production not recognized: " << token << " ::= " << children << endl;
  exit(1);

  return NULL;
}
