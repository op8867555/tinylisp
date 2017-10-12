#include <stdio.h>
#include <ctype.h>
#include "more_tiny_lisp.c"


void * skip_space (FILE * in) {
    char c;
    while ((c = getc(in)) != EOF) {
        if (isspace(c)) continue;
        else {
            ungetc(c, in);
            break;
        }
    }
}

obj *read(itpr *m, FILE * in);

obj * read_pair(itpr * m, FILE * in) {
    char c;
    skip_space(in);
    if ((c = getc(in)) == ')') {
        return NULL;
    }
    ungetc(c, in);
    skip_space(in);
    TEMP_OBJ(m, car_obj);
    SET_TEMP(car_obj, read(m, in));
    TEMP_OBJ(m, cdr_obj);
    skip_space(in);
    if ((c = getc(in)) == '.') {
        SET_TEMP(cdr_obj, read(m, in));
        skip_space(in);
       if ((c = getc(in)) != ')') {
           error("syntax error");
       } 
    }
    else {
        ungetc(c, in);
        SET_TEMP(cdr_obj, read_pair(m, in));
    }
    SET_TEMP(car_obj, mkPair(m, GET_TEMP(car_obj), GET_TEMP(cdr_obj)));
    RELEASE_TEMP(m, cdr_obj);
    RELEASE_TEMP(m, car_obj);
    return GET_TEMP(car_obj);


}

obj * read (itpr * m, FILE * in) {
    skip_space(in);
    char c = getc(in);
    if (c == EOF) {
        exit(0);
    }
    if (c == '(') {
        return read_pair(m, in);
    }
    if (isdigit(c)) {
        int val = c - '0';
        while ((c = getc(in)) && isdigit(c))
            val = val * 10 + (c - '0');
        ungetc(c, in);
        obj * s = mkInt32(m, val);
        return s;
    }
    else {
        int count = 0;
        char buffer[64];
        while (c != EOF) {
            if (c == ' ' || c == '\n' || c == '(' || c == ')' || c == '.') break;
            if (count < 63) {
                    buffer[count++] = c;
            }
            else
                error("symbol length longer than max");
            c = getc(in);
        }
        buffer[count] = '\0';
        obj * s = mkSym(m, buffer);
        ungetc(c, in);
        return s;
    }
}


obj * testCall(itpr * m, obj * a) {
    TEMP_OBJ(m, tmp);
    SET_TEMP(tmp, mkPair(m, a, NULL)); 
    SET_TEMP(tmp, mkPair(m, m->defined_symbol[quote_s], GET_TEMP(tmp)));
    obj * res = eval(m, GET_TEMP(tmp), NULL);
    RELEASE_TEMP(m, tmp);
    return res;
}


int main(void) {
    itpr * m = allocItpr();
    initItpr(m);
    TEMP_OBJ(m, read_in);
    TEMP_OBJ(m, env);
    SET_TEMP(env, initEnv(m));
    SET_TEMP(env, mkPair(m, GET_TEMP(env), NULL));
    while (1) {
        printf("eval(%5d)=>", countList(m->activeList));
        SET_TEMP(read_in, read(m, stdin));
        gc_start(m);
        SET_TEMP(read_in, eval(m, GET_TEMP(read_in), GET_TEMP(env)));
        print(m, GET_TEMP(read_in));
    }
    RELEASE_TEMP(m, env);
    RELEASE_TEMP(m, read_in);
    return 0;
}

int alloc_main(void) {
    itpr * m = allocItpr();
    initItpr(m);
    print(m, allocObj(m));
    print(m, allocObj(m));
}
    /*  
    
    obj * cons = mkSym(i, "cons");
    obj * car = mkSym(i, "car");
    obj * cdr = mkSym(i, "cdr");
    obj c_cons = { .type = C_FUNC, .data.c_func.func = l_cons};
    obj c_car = { .type = C_FUNC, .data.c_func.func = l_car};
    obj c_cdr = { .type = C_FUNC, .data.c_func.func = l_cdr};
//    obj c_quote = { .type = C_FUNC, .data.c_func.func = l_quote};
//    obj c_define = { .type = C_FUNC, .data.c_func.func = l_define};
    obj * frame = NULL;
    frame = mkPair(i, mkPair(i, cons, &c_cons), frame);
    frame = mkPair(i, mkPair(i, car, &c_car), frame);
    frame = mkPair(i, mkPair(i, cdr, &c_cdr), frame);
 //   frame = mkPair(mkPair(quote_symbol, &c_quote), frame);
 //   frame = mkPair(mkPair(define_symbol, &c_define), frame);
    obj * env = mkPair(i, frame, NULL);
    obj * ast = NULL;
    while (1) {
        printf("input=>");
//        printf("input(%5d)=>", countList(activeList));
        obj * ast = read(i, stdin);
//        push(&root, &ast);
#if DEBUG
        printf("=>");
//        printf("eval on(%5d)=>   ", countList(activeList));
        print(i, ast);
        puts("env");
        print(i, env);
#endif
        obj * res = eval(i, ast, env);
//        pop(&root);
        //puts("gc");
        //gc_start();
        puts("result");
        print(i, res);
    }
    return 0;
}
*/
