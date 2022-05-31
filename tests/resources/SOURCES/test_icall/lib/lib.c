typedef void(Func)(char *,int);
typedef int(Func2)(char *, int);

struct data_init {
  int count;
  int (*cb)(char *, int);
};
struct data {
  void (*cb)(char *, int);
};

void cb(char *buf, int len) {
  for (int i = 0; i < len; i++) {
    buf[i] = 'a' + i;
  }
}

void cb2(char *buf, int flag) {
  if (flag % 2)
    *buf = 'a';
  else
    *buf = 'b';  
}

int cb3(char *buf, int len) {
    buf[4] = 'a';
    return len;
}

void init(char *buf, int len, Func func) {
  func(buf, len);
}

struct data_init data_init_s = {1, cb3};
struct data data_s;

void set_value() {
  data_s.cb = cb2; 
}

void func(char *buf, int len) {
  init(buf, len, cb);
}

void func2(char *buf, int len) {
  set_value();
  data_s.cb(buf, len);  
}

int func3(char *buf, int len) {
  Func2 *func = data_init_s.cb;
  return func(buf, len);
}

