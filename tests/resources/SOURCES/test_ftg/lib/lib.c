#include "lib.h"
#include <fcntl.h>
#include <unistd.h>

void inputInt(int a1) {}
void inputUInt(unsigned Arg) {}
void inputChar(char Arg) {}
//void inputBool(bool Arg) {}
void inputFloat(float a1) {}
void inputDouble(double Arg) {}

void inputCStr(char* Str) {}
//void inputStr(std::string Str) {}

void inputIntPtr(int *Ptr) {}
void inputStructPtr(s1 *Ptr) {}
void inputEnumPtr(e1 *Ptr) {}
void outputPtr(int *P) {
  pre2(P);
}
void inputVoidPtr(void *a1) {}
void inputCallBackPtr(cb *a1) {}

void inputCStrStrLen(char* CStr, int StrLen) {
  for (int S = 0; S < StrLen; ++S) CStr[S] = 0;
}
void inputArr(void *Array) {}
void inputStringArr(char **StringArray) {}
void inputStructArr(struct _s1 *StructArray) {}
void inputArrArrLen(int *Array, int ArrayLen) {
  for (int i=0; i<ArrayLen; ++i) {
    Array[i] = 0;
  }
}
void inputVoidArrArrLen(void *a1, int a2) {
  inputArrArrLen((int *)a1, a2);
}
void outputArrArrLen(int **a1, int a2) {
  for (int i = 0; i < a2; ++i) {
    *a1[i] = 0;
  }
}

void inputEnum(e1 a1, enum _e1 a2) {
  PRE_1(a1, a2, 0);
}
void inputStruct(struct _s1 a1, s1 a2) {
  a2.a = a1.a;
  a1.b = a2.b;
  for(int i = 0; i < a1.BufLen; ++i) {
    a1.Buf[i];
  }
}
void inputUnion(union _union a1) {}
void inputUnsupportedStruct(struct _struct0 a1) {}

void filepath(const char *P) {
  close(open(P, O_RDONLY));
}

void loopexit(int P) {
  for (int i = 0; i < P; ++i) GV1 += i;
}

void noop() {}

