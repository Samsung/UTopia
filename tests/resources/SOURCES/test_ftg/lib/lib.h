#ifndef LIB_H
#define LIB_H

#include "pre.h"

//struct with supported types
typedef struct _s1 {
  int a;
  char *b;
  double c;
  float d;
  int Arr[10];
  int *Buf;
  int BufLen;
} s1;

//struct with unsupported types
struct _struct0 {
  void *a;
};

typedef void (*cb)(void *user_data);

union _union {
  int a;
};

// Primitive
void inputInt(int a1);
void inputUInt(unsigned Arg);
void inputChar(char Arg);
//void inputBool(bool Arg);
void inputFloat(float a1);
void inputDouble(double Arg);

// String
void inputCStr(char* Str);
//void inputStr(std::string Str);

// Pointer
void inputIntPtr(int *Ptr);
void inputStructPtr(s1 *Ptr);
void inputEnumPtr(e1 *Ptr);
void outputPtr(int *P);
void inputVoidPtr(void *a1);
void inputCallBackPtr(cb *a1);

// Array
void inputCStrStrLen(char* CStr, int StrLen);
void inputArr(void *Array);
void inputStringArr(char **StringArray);
void inputStructArr(struct _s1 *StructArray);
void inputArrArrLen(int *Array, int ArrayLen);
void inputVoidArrArrLen(void *a1, int a2);
void outputArrArrLen(int **a1, int a2);

// Defined type
void inputEnum(e1 a1, enum _e1 a2);
void inputStruct(struct _s1 a1, s1 a2);
void inputUnion(union _union a1);
void inputUnsupportedStruct(struct _struct0 a1);

// Properties
int GV1 = 0;
void filepath(const char *P);
void loopexit(int P);

void noop();

#endif // LIB_H
