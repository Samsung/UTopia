#include "../lib/lib.c"

typedef void (*void_fun_ptr)(void);
typedef void (*tc_fun_ptr)(void);

typedef struct testcase_s {
  const char* name;
  tc_fun_ptr function;
  void_fun_ptr startup;
  void_fun_ptr cleanup;
} testcase;

void API_1(void*);

void strings(int P) {

  char *Var1 = 0;
  char **Var2 = 0;
  char Var3[10] = { 0, };
  char Var4[10][10] = { 0, };
  char Var5[P];
  char Var6[P][P];

  API_1(Var1);
  API_1(Var2);
  API_1(Var3);
  API_1(Var4);
  API_1(Var5);
  API_1(Var6);

}

void integers(int P) {

  char Var1 = 0;
  int Var2 = 0;
  int *Var3 = 0;
  int Var4[10] = { 0, };
  int Var5[P];

  unsigned char Var11 = 0;
  unsigned Var12= 0;
  unsigned *Var13 = 0;
  unsigned Var14[10] = { 0, };
  unsigned Var15[P];

  API_1(&Var1);
  API_1(&Var2);
  API_1(Var3);
  API_1(Var4);
  API_1(Var5);
  API_1(&Var11);
  API_1(&Var12);
  API_1(Var13);
  API_1(Var14);
  API_1(Var15);

}

void floats(int P) {

  float Var1 = 0.0;
  float *Var2 = 0;
  float Var3[10] = { 0.0, };
  float Var4[P];

  API_1(&Var1);
  API_1(Var2);
  API_1(Var3);
  API_1(Var4);

}

void enums(int P) {

  enum E1 Var1 = 0;
  enum E1 *Var2 = 0;
  enum E1 Var3[10] = {0, };
  enum E1 Var4[P];

  API_1(&Var1);
  API_1(Var2);
  API_1(Var3);
  API_1(Var4);

}

void unsupports(int P) {

  void *Var1 = 0;
  struct S1 Var2 = { 0, };
  union U1 Var3 = { 0, };
  char ***Var4 = 0;
  char Var5[10][10][10] = { 0, };
  char Var6[P][P][P];
  int **Var7 = 0;
  int Var8[10][10];
  int Var9[P][P];
  unsigned **Var10 = 0;
  unsigned Var11[10][10];
  unsigned Var12[P][P];
  float **Var13 = 0;
  float Var14[10][10];
  float Var15[P][P];
  enum E1 **Var16 = 0;
  enum E1 Var17[10][10];
  enum E1 Var18[P][P];

  API_1(Var1);
  API_1(&Var2);
  API_1(&Var3);
  API_1(Var4);
  API_1(Var5);
  API_1(Var6);
  API_1(Var7);
  API_1(Var8);
  API_1(Var9);
  API_1(Var10);
  API_1(Var11);
  API_1(Var12);
  API_1(Var13);
  API_1(Var14);
  API_1(Var15);
  API_1(Var16);
  API_1(Var17);
  API_1(Var18);

}

void utc_test() {

  integers(10);
  floats(10);
  enums(10);
  strings(10);
  unsupports(10);

}


testcase tc_array[] = {
  {"utc_test", utc_test, 0, 0},
};

int main(int argc, char* argv[]) {

  return 0;

}
