#include "../lib/lib.h"
#include "autofuzz.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int * autofuzz0;
extern unsigned autofuzz0size;
extern int * autofuzz1;
extern unsigned autofuzz1size;
extern int * autofuzz2;
extern unsigned autofuzz2size;
extern int * autofuzz3;
extern unsigned autofuzz3size;
extern int * autofuzz13;
extern unsigned autofuzz13size;
extern int * autofuzz14;
extern unsigned autofuzz14size;
extern int * autofuzz15;
extern unsigned autofuzz15size;
extern char * autofuzz16;
extern unsigned autofuzz16size;
extern char * autofuzz17;
extern unsigned autofuzz17size;
extern int * autofuzz18;
extern unsigned autofuzz18size;
extern int autofuzz19;
extern char * autofuzz20;
extern unsigned autofuzz20size;
extern int autofuzz21;
extern int * autofuzz22;
extern unsigned autofuzz22size;
extern int * autofuzz23;
extern unsigned autofuzz23size;
void assign_fuzz_input_to_global_autofuzz0();
void assign_fuzz_input_to_global_autofuzz1();
void assign_fuzz_input_to_global_autofuzz2();
void assign_fuzz_input_to_global_autofuzz3();
#ifdef __cplusplus
}
#endif
#include "../lib/lib.h"

typedef void (*void_fun_ptr)(void);
typedef void (*tc_fun_ptr)(void);
typedef struct testcase_s {
    const char* name;
    tc_fun_ptr function;
    void_fun_ptr startup;
    void_fun_ptr cleanup;
} testcase;

void utc_startup_1() {}
void utc_cleanup_1() {}

int g_input[20] = {1,2,3,};
int g_input2[20];
static int s_input[20] = {1,2,3,};
static int s_input2[20];
#define MAX 50

void utc_defined_type_p(){
  const enum _e1 var1 = A;
  e1 var2;
  inputEnum(var1, var2);

  int Buf[MAX];
  struct _s1 struct1 = {5, "CString", 1.1, 1.1f, {0,}, Buf, MAX};
  s1 struct2;
  struct2.a = 1;
  inputStruct(struct1, struct2);
}

#define MACRO_1(Var) sizeof(Var) / sizeof(Var[0])

void utc_macro_func_assign_n() {
  char* Msg = "Hello";
  int MsgLen = MACRO_1(Msg);
  inputCStrStrLen(Msg, MsgLen);
}

void utc_variable_length_array_n() {
  // variable length array is unsupported - not fuzzable.
  int len;
  char input6[len];
  inputCStrStrLen(input6, len);
}

void utc_fixed_length_array_p() {
  // fixed length array
  int input[20]; { for (unsigned i=0; i<autofuzz13size; ++i) { input[i] = autofuzz13[i]; } }
  inputArr(input);

  int input2[20] = {1,2,3,}; { for (unsigned i=0; i<autofuzz14size; ++i) { input2[i] = autofuzz14[i]; } }
  inputArr(input2);

  // with global and static variables
  inputArr(g_input);
  inputArr(g_input2);
  inputArr(s_input);
  inputArr(s_input2);

  int input3[MAX] = {1,2,3,}; { for (unsigned i=0; i<autofuzz15size; ++i) { input3[i] = autofuzz15[i]; } }
  inputArr(input3);

  // fixed length string
  char input4[MAX] = {1,2,3,}; { for (unsigned i=0; i<autofuzz16size; ++i) { input4[i] = autofuzz16[i]; } }
  inputArr(input4);

  char input5[5] = {1,2,3,}; { for (unsigned i=0; i<autofuzz17size; ++i) { input5[i] = autofuzz17[i]; } }
  inputArr(input5);

  // fixed length array with arr-len relation
  int input9[20] = {1,2,3,}; { for (unsigned i=0; i<autofuzz18size; ++i) { input9[i] = autofuzz18[i]; } }
  inputArrArrLen(input9, autofuzz19);

  char input10[20] = { 0, }; { for (unsigned i=0; i<autofuzz20size; ++i) { input10[i] = autofuzz20[i]; } }
  inputArrArrLen(input10, autofuzz21);

  // multiple definition at one line
  int input11[20], input12[20]; { for (unsigned i=0; i<autofuzz22size; ++i) { input11[i] = autofuzz22[i]; } } { for (unsigned i=0; i<autofuzz23size; ++i) { input12[i] = autofuzz23[i]; } }
  inputArr(input11);
  inputArr(input12);
}

void utc_fixed_length_array_n() {
  // fixed length string array
  char *StringArray[20] = {"a", "b", "c",};
  inputStringArr(StringArray);

  // fixed length struct array
  struct _s1 StructArray[20];
  inputStructArr(StructArray);
}

void utc_primitive_type_p() {
  inputInt(1);
  inputUInt(1u);
  inputChar('a');
  _Bool b = 1;
  inputInt(b);
//  inputBool(true);
  inputFloat(1.1f);
  inputDouble(1.1);
}

void utc_str_type_p() {
  char *Str1 = "dummy";
  inputCStr(Str1);
  inputCStr("dummy2");
  inputVoidPtr("dummy3");

  //char* as array with arraylen
  char *Str2 = "dummy4";
  inputVoidArrArrLen(Str2, 20);
}

void utc_pointer_type_p() {
  int Var1 = 5;
  int *Var1P = &Var1;
  s1 *Var2;
  e1 Var3 = A;
  inputIntPtr(Var1P);
  inputStructPtr(Var2);
  inputEnumPtr(&Var3);
}

void utc_property_p() {
  char *v1 = "input.txt";
  filepath(v1);
  loopexit(10);
}

void utc_unsupported_type_n() {
  void *void_only_in;
  inputVoidPtr(void_only_in);
  union _union union_only_in;
  inputUnion(union_only_in);
  struct _struct0 void_struct_in;
  inputUnsupportedStruct(void_struct_in);
  void *void_arrlen_in;
  inputVoidArrArrLen(void_arrlen_in, 1);
  cb *cb_only_in;
  inputCallBackPtr(cb_only_in);
}

void utc_no_input_n() {
  noop();
  int out_only_in;
  outputPtr(&out_only_in);
  int *out_arrlen_in;
  outputArrArrLen(&out_arrlen_in, 1);
}

testcase tc_array[] = {
  {"utc_defined_type_p", utc_defined_type_p, utc_startup_1, utc_cleanup_1},
  {"utc_macro_func_assign_n", utc_macro_func_assign_n, utc_startup_1, utc_cleanup_1},
  {"utc_variable_length_array_n", utc_variable_length_array_n, utc_startup_1, utc_cleanup_1},
  {"utc_fixed_length_array_p", utc_fixed_length_array_p,utc_startup_1, utc_cleanup_1},
  {"utc_fixed_length_array_n", utc_fixed_length_array_n,utc_startup_1, utc_cleanup_1},
  {"utc_primitive_type_p", utc_primitive_type_p, utc_startup_1, utc_cleanup_1},
  {"utc_str_type_p", utc_str_type_p, utc_startup_1, utc_cleanup_1},
  {"utc_pointer_type_p", utc_pointer_type_p, utc_startup_1, utc_cleanup_1},
  {"utc_property_p", utc_property_p, 0, 0},
  {"utc_unsupported_type_n", utc_unsupported_type_n, utc_startup_1, utc_cleanup_1},
  {"utc_no_input_n", utc_no_input_n, utc_startup_1, utc_cleanup_1}
};



#ifdef __cplusplus
extern "C" {
#endif
void assign_fuzz_input_to_global_autofuzz0() {
    { for (unsigned i=0; i<autofuzz0size; ++i) { g_input[i] = autofuzz0[i]; } }
}
void assign_fuzz_input_to_global_autofuzz1() {
    { for (unsigned i=0; i<autofuzz1size; ++i) { g_input2[i] = autofuzz1[i]; } }
}
void assign_fuzz_input_to_global_autofuzz2() {
    { for (unsigned i=0; i<autofuzz2size; ++i) { s_input[i] = autofuzz2[i]; } }
}
void assign_fuzz_input_to_global_autofuzz3() {
    { for (unsigned i=0; i<autofuzz3size; ++i) { s_input2[i] = autofuzz3[i]; } }
}
void enterAutofuzz() {
  assign_fuzz_input_to_global_autofuzz0();
  assign_fuzz_input_to_global_autofuzz1();
  assign_fuzz_input_to_global_autofuzz2();
  assign_fuzz_input_to_global_autofuzz3();
  utc_startup_1();
  utc_fixed_length_array_p();
  utc_cleanup_1();
}
#ifdef __cplusplus
}
#endif
