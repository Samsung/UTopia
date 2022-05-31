typedef void (*void_fun_ptr)(void);
typedef void (*tc_fun_ptr)(void);

typedef struct testcase_s {
  const char* name;
  tc_fun_ptr function;
  void_fun_ptr startup;
  void_fun_ptr cleanup;
} testcase;

void API_1(int);
void API_2(const char*);
void API_3(int*);

void utc_int() {
  int Var1 = 0;
  API_1(Var1);
}

void utc_filepath() {
  API_2("File.txt");
}

void utc_fixedlen_array() {
  int Var1[5];
  API_3(Var1);
}

void utc_varlen_array() {
  int Var1 = 10;
  int Var2[Var1];
  API_3(Var2);
}

void utc_nullptr() {
  int *Var1 = 0;
  API_3(Var1);
}

#define SIZE(Var) sizeof(Var) / sizeof(Var[0])

void utc_reliedconst() {
  int Var1[10] = { 0, };
  API_1(SIZE(Var1));
}

void API_4(int*);
int API_5();

void utc_externallyassigned() {
  API_1(API_5());
  
  int Var = 10;
  API_4(&Var);
  API_1(Var);
}

void API_6(int*, int);

void utc_array_property_pair() {

  int Var1[10] = { 0, };
  API_6(Var1, 10);

}

int API_7(int*);

void utc_array_property_only_array() {

  int Var1[10] = { 0, };
  API_6(Var1, API_7(Var1));

}

int *API_8();

void utc_array_property_only_arraylen() {

  API_6(API_8(), 10);

}

void utc_array_property_two_array_one_len() {

  int Var1[10] = { 0, };
  int Var2[10] = { 0, };
  int Var3 = 10;

  API_6(Var1, Var3);
  API_6(Var2, Var3);
}

testcase tc_array[] = {
  {"utc_int", utc_int, 0, 0},
  {"utc_filepath", utc_filepath, 0, 0},
  {"utc_fixedlen_array", utc_fixedlen_array, 0, 0},
  {"utc_varlen_array", utc_varlen_array, 0, 0},
  {"utc_nullptr", utc_nullptr, 0, 0},
  {"utc_reliedconst", utc_reliedconst, 0, 0},
  {"utc_externallyassigned", utc_externallyassigned, 0, 0},
  {"utc_array_property_pair", utc_array_property_pair, 0, 0},
  {"utc_array_only_array", utc_array_property_only_array, 0, 0},
  {"utc_array_property_only_arraylen",utc_array_property_only_arraylen, 0, 0},
  {"utc_array_property_two_array_one_len",
      utc_array_property_two_array_one_len, 0, 0}
};

int main(int argc, char* argv[]) {
  return 0;
}
