
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#define HEAP_MAX 1000

#define NULLPAIR(m) mkPair(m, NULL, NULL);
#define TEMP_OBJ(m, name) \
    obj * name ## _tmp = NULLPAIR(m); \
    pushRoot(m, name ## _tmp);
#define SET_TEMP(name, v) (setCar( name ## _tmp , v))
#define GET_TEMP(name) (getCar( name ## _tmp))
#define RELEASE_TEMP(m, name) \
    popRoot(m); \

typedef enum {
    lambda_s,
    closure_s,
    quote_s,
    define_s,
    if_s,
    true_s
} defined_symbol_index;

void error (char * str) {
    printf("ERROR: ");
    printf("%s\n", str);
    return;
}

typedef struct itpr_s itpr;

// OBJS
typedef enum {INT32, SYM, PAIR, C_FUNC} obj_type;
typedef struct obj_s obj;
struct obj_s {
    obj_type type;
    obj * next;
    bool mark;
    union {
        struct {
            int val;
        } int32;
        struct {
            char * str;
        } sym;
        struct {
            obj * car; 
            obj * cdr; 
        } pair;
        struct {
            obj * (*func) (itpr *, obj *, obj *);
        } c_func;
    } data;
} ;

typedef struct stack_s {
    obj * ptr;
    struct stack_s * next;
} stack;

typedef struct itpr_s {
    obj * activeList;
    obj * freeList;
    obj * symbol_table;
    stack * root;
    obj * defined_symbol[10];
} itpr;

stack * push (stack ** s, obj * o) {
    stack * new = malloc(sizeof(stack));
    new->ptr = o;
    new->next = *s;
    *s = new;
    return *s;
}

obj * pop (stack ** s) {
    if (!*s) return NULL; 
    obj * it = (*s)->ptr;
    stack * old = (*s);
    *s =  (*s)->next;
    free(old);
    return it;
}

obj * popFromList(obj ** s) {
    if (!*s) return NULL; 
    obj * it = (*s);
    *s = (*s)->next;
    return it;
}

obj * pushToList(obj ** s, obj * o) {
    o->next = *s;
    *s = o;
    return *s;

}

stack * pushRoot (itpr * m, obj * o) {
    stack * new = malloc(sizeof(stack));
    new->ptr = o;
    new->next = m->root;
    m->root = new;
    return NULL;
}

obj * popRoot (itpr * m) {
    if (!m->root) return NULL;
    obj * it = m->root->ptr;
    stack * old = m->root;
    m->root =  m->root->next;
    free(old);
    return it;
}

void print(itpr *, obj *);

void printStack(itpr * m) {
    stack * iter = m->root;
    printf("stack: {\n");
    while (iter != NULL) {
        printf("%p ", iter->ptr);
        print(m, iter->ptr);
        iter = iter->next;
    }
    printf("}\n");

}


int countList(obj * o) {
    int c = 0;
    obj * iter = o;
    int retry = 0;
    while (iter != NULL) {
#if HARDDEBUG
        printf("%p\n", iter);
#endif
        if (iter->next == o) {
            puts("this");
            exit(1);
        }
        c++;
        iter = iter->next;
    }
    return c;
}

obj * printList(obj * l) {
    obj * iter = l;
    int i = 0;
    printf("( ");
    while (iter != NULL) {
        i++;
        printf("%p[%d] ", iter, iter->mark);
        iter = iter->next;
        if ( iter != NULL && iter->next == l) break;
    }
    printf(")\n");
    return NULL;
}

void initHeap(itpr * m, int size) {
    obj * prev = NULL;
    obj * new = NULL;
    for ( int i = 0; i < size; i++) {
        new = (obj*) malloc(sizeof(obj));
        if (!new) error ("heap really overflow");
        new->next = prev;
        new->mark = false;
        prev = new;
    }
    m->freeList = prev;
}
int mark_count;

void gc_mark_obj(itpr * m, obj * o) {
    if (o == NULL) return ;
    else if (o->mark == false) {
#ifdef DEBUG_GC
        printf("mark %p ", o);
        print(m, o);
#endif
        mark_count += 1;
        o->mark = true;
        switch(o->type) {
            case SYM:
                break;
            case PAIR:
                gc_mark_obj(m, o->data.pair.car);
                gc_mark_obj(m, o->data.pair.cdr);
                break;
        };
    }
}

#ifdef DEBUG_GC

void gc_mark(itpr * m) {
    mark_count = 0;
    printStack(m);
    stack * iter = m->root;
    while (iter != NULL) {
        printf("%p\n", iter->ptr);
        gc_mark_obj(m, iter->ptr);
        iter = iter->next;
    }
    printf("marked obj: %d\n", mark_count);
}
#else

void gc_mark(itpr * m) {
    stack * iter = m->root;
    while (iter != NULL) {
        gc_mark_obj(m, iter->ptr);
        iter = iter->next;
    }
}

#endif

#ifdef DEBUG_GC

void gc_sweep(itpr * m) {
    int sweep_count = 0;
    int walk_count = 0;
    obj * iter = popFromList(&m->activeList);
    obj * new_active_list = NULL;
    while (iter != NULL) {
        printf("%p %d\n", iter, iter->mark);
        walk_count++;
        if (iter->mark == true) {
            iter->mark = false;
            pushToList(&new_active_list, iter);
        }
        else {
            pushToList(&m->freeList, iter);
            sweep_count++;
        }
        iter = popFromList(&m->activeList);
    }
    m->activeList = new_active_list;
    printf("should sweeped: %d\n", sweep_count);
    printf("walk throuth: %d\n", walk_count);
}

#else

void gc_sweep(itpr * m) {
    obj * iter = popFromList(&m->activeList);
    obj * new_active_list = NULL;
    while (iter != NULL) {
        if (iter->mark == true) {
            iter->mark = false;
            pushToList(&new_active_list, iter);
        }
        else {
            pushToList(&m->freeList, iter);
        }
        iter = popFromList(&m->activeList);
    }
    m->activeList = new_active_list;
}

#endif

void gc_sweep_old(itpr * m) {
    int sweep_count = 0;
    obj * iter = m->activeList; 
    obj * prev = m->activeList; 
    while (iter != NULL) {
#ifdef DEBUG
        printf("%p [%d]", iter, iter->mark);
        print(m, iter);
#endif
        if (iter->mark == false) {
            obj * next = iter->next;
            if (iter == m->activeList) {
                m->activeList = m->activeList->next;
                prev = m->activeList;
            }
            else {
                prev->next = next;
                prev = iter;
            }
            iter->next = m->freeList;
            m->freeList = iter;
            //prev = iter;
            iter = next;
            sweep_count += 1;
        }
        else {
            iter->mark = false;
            prev = iter;
            iter = iter->next;
        }
    }
#ifdef DEBUG
    printf("sweeped: %d\n", sweep_count);
#endif
}

void  gc_start(itpr * m) {
#ifdef DEBUG
    puts(">start garbage collect");
#endif
    gc_mark(m);
    gc_sweep(m);
}

obj * allocObj(itpr * m) {
    if (m->freeList != NULL) {
        obj * new = popFromList(&m->freeList);
        pushToList(&m->activeList, new);
        return new;
    }
    else {
        gc_start(m);
        if (m->freeList != NULL) {
            return allocObj(m);
        }
        else {
            error("heap overflow");
            exit(1);
        }
    }
}

// C native funcs
obj * mkInt32(itpr * m, int v) {
    obj * new = allocObj(m);
    new->type = INT32;
    new->data.int32.val = v;
    return new;
}

obj * mkPair(itpr * m, obj * car, obj * cdr) {
    obj * new = allocObj(m);
    new->type = PAIR;
    new->data.pair.car = car;
    new->data.pair.cdr = cdr;
    return new;
}

#define isNil(o) (o == NULL)

#define isPair(o) ((!isNil(o) && o->type == PAIR) ? o : NULL )
#define getCar(o) ((!isNil(o) && isPair(o)) ? o->data.pair.car : NULL)
#define getCdr(o) ((!isNil(o) && isPair(o)) ? o->data.pair.cdr : NULL)
#define setCar(o, v) (o->data.pair.car = v)
#define setCdr(o, v) (o->data.pair.cdr = v)

#define getCaar(o) (getCar(getCar(o)))
#define getCadr(o) (getCar(getCdr(o)))
#define getCdar(o) (getCdr(getCar(o)))
#define getCddr(o) (getCdr(getCdr(o)))

#define getCaaar(o) (getCar(getCar(getCar(o))))
#define getCaadr(o) (getCar(getCar(getCdr(o))))
#define getCadar(o) (getCar(getCdr(getCar(o))))
#define getCaddr(o) (getCar(getCdr(getCdr(o))))
#define getCdaar(o) (getCdr(getCar(getCar(o))))
#define getCdadr(o) (getCdr(getCar(getCdr(o))))
#define getCddar(o) (getCdr(getCdr(getCar(o))))
#define getCdddr(o) (getCdr(getCdr(getCdr(o))))

obj * mkList(itpr * m, int count, ...) {
    obj * buff[1000];
    va_list list;
    va_start(list, count);
    for (int i = 0; i < count ; i++) {
        buff[i] = va_arg(list, obj *);
    } 
    obj * res = NULL;
    for (int i = count -1 ; i > -1 ; i--) {
        res = mkPair(m, buff[i], res);
    }
    va_end(list);
    return (res);
}

obj * mkFunc(itpr * m, obj * (*fn)(itpr *, obj *, obj *)) {
    obj * new = allocObj(m);
    new->type = C_FUNC;
    new->data.c_func.func = fn;
    return new;
}

#define isSym(o) ((!isNil(o) && o->type == SYM))
#define strSym(o) ((!isNil(o) && isSym(o)) ? o->data.sym.str : NULL )
#define eqSym(a, b) (a == b)

obj * mkSym(itpr * m, char * str) {
    obj * iter = m->symbol_table;
    while (!isNil(iter)) {
        if (strcmp(str, strSym(getCar(iter))) ==  0) {
            return getCar(iter);
        }
        iter = getCdr(iter);
    }

    char * new = (char *) malloc(sizeof(char) * (strlen(str)+1));
    strcpy(new, str);

    TEMP_OBJ(m, sym);
    SET_TEMP(sym, allocObj(m));
    GET_TEMP(sym)->type = SYM;
    GET_TEMP(sym)->data.sym.str = new;

    if (!isNil(m->symbol_table)) {
        TEMP_OBJ(m, head_copy);
        SET_TEMP(head_copy, getCar(m->symbol_table));
        SET_TEMP(head_copy, mkPair(m, GET_TEMP(head_copy), getCdr(m->symbol_table))); 
        setCar(m->symbol_table, GET_TEMP(sym) );
        setCdr(m->symbol_table, GET_TEMP(head_copy) );
        RELEASE_TEMP(m, head_copy);
    }
    else {
        m->symbol_table = mkPair(m, GET_TEMP(sym), NULL);
    }
    RELEASE_TEMP(m, sym);
    return GET_TEMP(sym);
}

#define isCfunc(o) (!isNil(o) && o->type == C_FUNC)

obj * assoc(obj * sym, obj * list) {
    obj * iter = list;
    while (!isNil(iter)) {
        if (eqSym(getCaar(iter), sym)) {
            return getCar(iter);
        }
        iter = getCdr(iter);
    }
    return NULL;
}

// c_func for interpreter


obj * mkFrame(itpr *m, obj * vars, obj * vals) {
    obj * var = vars;
    obj * val = vals;
    //    obj * res = NULLPAIR(m);
    TEMP_OBJ(m, res);
    TEMP_OBJ(m, tmp);
    while(!isNil(var) && !isNil(var)) {
        SET_TEMP(tmp, mkPair(m, getCar(var), getCar(val)));
        SET_TEMP(res, mkPair(m, GET_TEMP(tmp), GET_TEMP(res)));
        var = getCdr(var);
        val = getCdr(val);
    }
    if (!isNil(var) || !isNil(val))
        error("var too few or too much.");
    RELEASE_TEMP(m, tmp);
    RELEASE_TEMP(m, res);
    return GET_TEMP(res);
}

obj * lookupEnv(obj * var, obj * env) {
    obj * iter = env;
    while (!isNil(iter)) {
        obj * v = assoc(var, getCar(iter));
        if (v) return getCdr(v);
        iter = getCdr(iter);
    }
    //printf("ENV:\n");
    //print(env);
    char buffer[1000] = "var not found: ";
    strncat(buffer, strSym(var), 99);
    error(buffer);
    return NULL;
}


obj * list_of_values (itpr *, obj *, obj *);

void printPairAsList(itpr * m, obj * o) ;

void printObj(itpr * m, obj * o) {
    if (isNil(o))
        printf("nil");
    else {
        switch (o->type) {
        case INT32:
            printf("%d", o->data.int32.val);
            break;
        case SYM:
            printf("%s", strSym(o));
            break;
        case PAIR:
#ifndef DEBUG
            if (eqSym(getCar(o), m->defined_symbol[closure_s])) {
                printf("#<clousure(%p)>", o);
                return;
            }
#endif
            printf("( ");
            printObj(m, getCar(o));
            printf(" . ");
            printObj(m, getCdr(o));
            printf(" )");
            break;
        case C_FUNC:
            printf("#<C_FUNC(%p)>", o->data.c_func.func);
        }
    }
}

void printPairAsList(itpr * m, obj * o) {
    obj * iter = o;
    printf("(");
    while (!isNil(iter)) {
        if (isNil(getCdr(iter))) {
            printf(" ");
            printObj(m, getCar(iter));
            printf(" ");
        }
        else {
            printf(" ");
            printObj(m, getCar(iter));
        }
        iter = getCdr(iter);
    }
}

void print (itpr * m, obj * o) {

#ifdef DEBUG
    if (o)
        printf("[%p %d]", o, o->mark);
#endif
    printObj(m, o);
    puts("");
}


obj * copy_obj(itpr * m, obj * o) {
    if (o == NULL) return NULL;
    switch (o->type) {
        case SYM:
            return o;
        case PAIR:{
                      TEMP_OBJ(m, res);
                      SET_TEMP(res, copy_obj(m, getCar(o)));
                      SET_TEMP(res, mkPair(m, GET_TEMP(res), copy_obj(m, getCdr(o))));
                      RELEASE_TEMP(m, res);
                      return GET_TEMP(res);
                  }
        case C_FUNC: 
                  return o;
    }
}

obj * mkQuote(itpr * m, obj * obj) {
    return mkPair(m, m->defined_symbol[quote_s], obj);
}

obj * mkClosure(itpr * m, obj * exp, obj * env) {
    TEMP_OBJ(m, res);
    //SET_TEMP(res, copy_obj(m, env));
    SET_TEMP(res, env);
    SET_TEMP(res, mkPair(m, exp, GET_TEMP(res)));
    SET_TEMP(res, mkPair(m, m->defined_symbol[closure_s], GET_TEMP(res)));
    RELEASE_TEMP(m, res);
    return GET_TEMP(res);
}

obj * eval(itpr * m, obj * exp, obj * env) {
    //printf("eval on:  ");
    //print(exp);
    // symbol
    if (!exp) {
        return NULL;
    }
    if (isSym(exp)) {
        return lookupEnv(exp, env);
    }
    if (exp->type == INT32) {
        return exp;
    }
    else if (isPair(exp)) {
        if (eqSym(getCar(exp), m->defined_symbol[quote_s])) {
            return getCadr(exp);
        }
        if (eqSym(getCar(exp), m->defined_symbol[closure_s])) {
            error("evaluate a closure without inputs");
        }
        if (eqSym(getCar(exp), m->defined_symbol[lambda_s])) {
            TEMP_OBJ(m, res);
            SET_TEMP(res, mkClosure(m, exp, env));
            RELEASE_TEMP(m, res);
            return GET_TEMP(res);
        }
        if (eqSym(getCar(exp), m->defined_symbol[if_s])) {
            TEMP_OBJ(m, res);
            SET_TEMP(res, eval(m, getCadr(exp), env));
            if (GET_TEMP(res) != NULL)
                SET_TEMP(res, eval(m, getCar(getCddr(exp)), env));
            else
                SET_TEMP(res, eval(m, getCadr(getCddr(exp)), env));
            RELEASE_TEMP(m, res);
            return GET_TEMP(res);
        }
        if (eqSym(getCar(exp), m->defined_symbol[define_s])) {
            obj * var = getCadr(exp);
            obj * body = getCar(getCddr(exp));
            TEMP_OBJ(m, val);
            SET_TEMP(val, eval(m, body, env));
            //print(val);
            //return NULL;
            obj * frame = getCar(env);
            TEMP_OBJ(m, frame_head_copy);
            SET_TEMP(frame_head_copy, mkPair(m, getCar(frame), getCdr(frame)));
            TEMP_OBJ(m, new_binding);
            SET_TEMP(new_binding, mkPair(m, var, GET_TEMP(val)));

            setCar(frame, GET_TEMP(new_binding));
            setCdr(frame, GET_TEMP(frame_head_copy));

            RELEASE_TEMP(m, new_binding);
            RELEASE_TEMP(m, frame_head_copy);

            return GET_TEMP(val);
        }
        //application on c_func
        else if (isCfunc(getCar(exp))) {

            obj * fn = getCar(exp);
            TEMP_OBJ(m, res);
            SET_TEMP(res, list_of_values(m, getCdr(exp), env));
            SET_TEMP(res, fn->data.c_func.func(m, GET_TEMP(res), env));
            RELEASE_TEMP(m, res);
            return GET_TEMP(res);
        }
        //application on closure
        else if (eqSym(getCaar(exp), m->defined_symbol[closure_s])){
            obj * closure = getCar(exp);
            obj * lambda = getCadr(closure);
            obj * vars = getCadr(lambda);
            TEMP_OBJ(m, vals);
            SET_TEMP(vals, list_of_values(m, getCdr(exp), env));
            obj * closure_env = (getCddr(closure));
            //printf("closure's env  ");
            //print(closure_env);
            TEMP_OBJ(m, tmp_env);
            SET_TEMP(tmp_env, mkFrame(m, vars, GET_TEMP(vals)));
            SET_TEMP(tmp_env, mkPair(m, GET_TEMP(tmp_env), getCddr(closure)));
            obj * body = getCar(getCddr(lambda));


            obj * result = eval(m, body, GET_TEMP(tmp_env)) ;
            RELEASE_TEMP(m, tmp_env);
            RELEASE_TEMP(m, vals);
            return result;
        }
        else {
            obj * body = getCar(exp);
            TEMP_OBJ(m, call); 
            SET_TEMP(call, eval(m, body, env));
            SET_TEMP(call, mkPair(m, GET_TEMP(call), getCdr(exp)));

            if (isNil(getCar(GET_TEMP(call)))) return NULL;

            TEMP_OBJ(m, res);
            SET_TEMP(res, eval(m, GET_TEMP(call) ,env));

            RELEASE_TEMP(m, res);
            RELEASE_TEMP(m, call);

            return GET_TEMP(res);
            //puts("not a lambda or closure or func");
            //return exp;
        }
    }
    else {
        error("ast invaild");
        return NULL;
    }
}

obj * eval_w_stack(obj * exp, obj * env) {

}


obj * list_of_values_old(itpr * m, obj * vals, obj * env) {
    obj * res = NULL;
    obj * iter = vals;
    while (!isNil(iter)) {
        res = mkPair(m, eval(m, getCar(iter), env), res);
        iter = getCdr(iter);
    }
    return res;
}

obj * list_of_values(itpr * m, obj * vals, obj * env) {
    if (isNil(vals)) {
        return NULL;
    }
    else {
        TEMP_OBJ(m, first);
        SET_TEMP(first, eval(m, getCar(vals), env));
        TEMP_OBJ(m, rest);
        SET_TEMP(rest, list_of_values(m, getCdr(vals), env));
        TEMP_OBJ(m, res);
        SET_TEMP(res, mkPair(m, GET_TEMP(first), GET_TEMP(rest)));
        RELEASE_TEMP(m, res);
        RELEASE_TEMP(m, rest);
        RELEASE_TEMP(m, first);
        return GET_TEMP(res);
    }
}

obj * l_cons(itpr * m, obj * args, obj * env) {
    return mkPair(m, getCar(args), getCadr(args));
}

obj * l_car(itpr * m, obj * args, obj * env) {
    return getCaar(args);
}

obj * l_cdr(itpr * m, obj * args, obj * env) {
    return getCdar(args);
}

obj * l_eval(itpr * m, obj * args, obj * env) {
    return eval(m, getCar(args), env);
}

obj * l_int32_minus(itpr * m, obj * args, obj * env) {
    if (!(getCar(args) && getCadr(args)))
        return NULL;
    if (getCar(args)->type == INT32 && getCadr(args)->type == INT32)
        return mkInt32(m, getCar(args)->data.int32.val - getCadr(args)->data.int32.val);
    else
        return NULL;
}

obj * l_int32_mul(itpr * m, obj * args, obj * env) {
    if (!(getCar(args) && getCadr(args)))
        return NULL;
    if (getCar(args)->type == INT32 && getCadr(args)->type == INT32)
        return mkInt32(m, getCar(args)->data.int32.val * getCadr(args)->data.int32.val);
    else
        return NULL;
}

obj * l_int32_eq(itpr * m, obj * args, obj * env) {
    if (!(getCar(args) && getCadr(args)))
        return NULL;
    if (getCar(args)->type == INT32 && getCadr(args)->type == INT32)
        return (getCar(args)->data.int32.val == getCadr(args)->data.int32.val) ? m->defined_symbol[true_s] : NULL;
    else
        return NULL;
}

obj * l_eq(itpr * m, obj * args, obj * env) {
    return (getCar(args) == getCadr(args)) ? m->defined_symbol[true_s]: NULL;
}

obj * l_list(itpr * m, obj * args, obj * env) {
    return args;
}

itpr * allocItpr(void) {
    itpr * new = (itpr *) malloc(sizeof(itpr));
    new->freeList = NULL;
    new->activeList = NULL;
    new->symbol_table = NULL;
    new->root = NULL;
    return new;
}


itpr * initItpr(itpr * m) {
    initHeap(m, HEAP_MAX);
    m->defined_symbol[quote_s] = mkSym(m, "quote");
    m->defined_symbol[lambda_s] = mkSym(m, "lambda");
    m->defined_symbol[define_s] = mkSym(m, "define");
    m->defined_symbol[closure_s] = mkSym(m, "closure");
    m->defined_symbol[if_s] = mkSym(m, "if");
    m->defined_symbol[true_s] = mkSym(m, "#t");
    pushRoot(m, m->symbol_table);
    return m;
}

obj * initEnv(itpr * m) {
    TEMP_OBJ(m, frame);
    TEMP_OBJ(m, bind);
    SET_TEMP(bind, mkPair(m, mkSym(m, "cons"), mkFunc(m, l_cons)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "car"), mkFunc(m, l_car)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "cdr"), mkFunc(m, l_cdr)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "list"), mkFunc(m, l_list)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "*"), mkFunc(m, l_int32_mul)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "-"), mkFunc(m, l_int32_minus)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "="), mkFunc(m, l_int32_eq)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "eval"), mkFunc(m, l_eval)));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    SET_TEMP(bind, mkPair(m, mkSym(m, "nil"), NULL));
    SET_TEMP(frame, mkPair(m, GET_TEMP(bind), GET_TEMP(frame)));
    RELEASE_TEMP(m, bind);
    RELEASE_TEMP(m, frame);
    return GET_TEMP(frame);
}
