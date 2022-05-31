#include "../lib/lib.c"

typedef void (*void_fun_ptr)(void);
typedef void (*tc_fun_ptr)(void);
typedef struct testcase_s {
  const char* name;
  tc_fun_ptr function;
  void_fun_ptr startup;
  void_fun_ptr cleanup;
} testcase;

float GVar;
void API_1(void*);

void supports(int P) {

  int Var1 = 0;
  static double Var2;
  int *Var3;
  int Var4[10] = { 0, };

  API_1(&Var1);
  API_1(&Var2);
  API_1(Var3);
  API_1(Var4);
  API_1(&GVar);
}


void utc_test() {
  supports(10);
}

testcase tc_array[] = {
  {"utc_test", utc_test, 0, 0},
};

int main(int argc, char* argv[]) {

  return 0;

}
