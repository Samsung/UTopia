/*****************************************************************************
*                 .::::.
*             ..:::...::::..
*         ..::::..      ..::::.
*      ..:::..              ..:::..
*   .::::.                      .::::.
*  .::.                            .::.
*  .::                         ..:. ::.  UTopia
*  .:: .::.                ..::::: .::.  Unit Tests to Fuzzing
*  .:: .:::             .::::::..  .::.  https://github.com/Samsung/UTopia
*  .:: .:::            ::::...     .::.
*  .:: .:::      ...   .....::     .::.  Base UT: utc_pointer_type_p
*  .:: .:::      .::.  ..::::.     .::.
*  .:: .::: .:.  .:::  :::..       .::.  This file was generated automatically
*  .::. ... .::: .:::  ....        .::.  by UTopia v[version]
*   .::::..  .:: .:::  .:::    ..:::..
*      ..:::...   :::  ::.. .::::..
*          ..:::.. ..  ...:::..
*             ..::::..::::.
*                 ..::..
*****************************************************************************/
#include "../lib/lib.h"
#include "autofuzz.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int autofuzz37;
extern e1 autofuzz39;
#ifdef __cplusplus
}
#endif
#include "../lib/lib.h"

typedef void (*void_fun_ptr)(void);
typedef void (*tc_fun_ptr)(void);
typedef struct testcase_s {
  const char *name;
  tc_fun_ptr function;
  void_fun_ptr startup;
  void_fun_ptr cleanup;
} testcase;

void utc_startup_1() {}
void utc_cleanup_1() {}

int g_input[20] = {
    1,
    2,
    3,
};
int g_input2[20];
static int s_input[20] = {
    1,
    2,
    3,
};
static int s_input2[20];
#define MAX 50

void utc_defined_type_p() {
  const enum _e1 var1 = A;
  e1 var2;
  inputEnum(var1, var2);

  int Buf[MAX];
  struct _s1 struct1 = {5,
                        "CString",
                        1.1,
                        1.1f,
                        {
                            0,
                        },
                        Buf,
                        MAX};
  s1 struct2;
  struct2.a = 1;
  inputStruct(struct1, struct2);
}

#define MACRO_1(Var) sizeof(Var) / sizeof(Var[0])

void utc_macro_func_assign_n() {
  char *Msg = "Hello";
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
  int input[20];
  inputArr(input);

  int input2[20] = {
      1,
      2,
      3,
  };
  inputArr(input2);

  // with global and static variables
  inputArr(g_input);
  inputArr(g_input2);
  inputArr(s_input);
  inputArr(s_input2);

  int input3[MAX] = {
      1,
      2,
      3,
  };
  inputArr(input3);

  // fixed length string
  char input4[MAX] = {
      1,
      2,
      3,
  };
  inputArr(input4);

  char input5[5] = {
      1,
      2,
      3,
  };
  inputArr(input5);

  // fixed length array with arr-len relation
  int input9[20] = {
      1,
      2,
      3,
  };
  inputArrArrLen(input9, 20);

  char input10[20] = {
      0,
  };
  inputArrArrLen(input10, 20);

  // multiple definition at one line
  int input11[20], input12[20];
  inputArr(input11);
  inputArr(input12);
}

void utc_fixed_length_array_n() {
  // fixed length string array
  char *StringArray[20] = {
      "a",
      "b",
      "c",
  };
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
  inputVoidPtr((void *)"dummy3");

  // char* as array with arraylen
  char *Str2 = "dummy4";
  inputVoidArrArrLen(Str2, 20);
}

void utc_pointer_type_p() {
  int Var1 = autofuzz37;
  int *Var1P = &Var1;
  s1 *Var2;
  e1 Var3 = autofuzz39;
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
    {"utc_macro_func_assign_n", utc_macro_func_assign_n, utc_startup_1,
     utc_cleanup_1},
    {"utc_variable_length_array_n", utc_variable_length_array_n, utc_startup_1,
     utc_cleanup_1},
    {"utc_fixed_length_array_p", utc_fixed_length_array_p, utc_startup_1,
     utc_cleanup_1},
    {"utc_fixed_length_array_n", utc_fixed_length_array_n, utc_startup_1,
     utc_cleanup_1},
    {"utc_primitive_type_p", utc_primitive_type_p, utc_startup_1,
     utc_cleanup_1},
    {"utc_str_type_p", utc_str_type_p, utc_startup_1, utc_cleanup_1},
    {"utc_pointer_type_p", utc_pointer_type_p, utc_startup_1, utc_cleanup_1},
    {"utc_property_p", utc_property_p, 0, 0},
    {"utc_unsupported_type_n", utc_unsupported_type_n, utc_startup_1,
     utc_cleanup_1},
    {"utc_no_input_n", utc_no_input_n, utc_startup_1, utc_cleanup_1}};



#ifdef __cplusplus
extern "C" {
#endif
void enterAutofuzz() {
  utc_startup_1();
  utc_pointer_type_p();
  utc_cleanup_1();
}
#ifdef __cplusplus
}
#endif
