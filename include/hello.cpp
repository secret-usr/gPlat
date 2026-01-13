#include <cstdio>
#include "hello.h"

void hello() {
    printf("Hello from include! (Auto-generated implementation)\n");
}

void hello(int i) {
    printf("Hello %d from include! (Auto-generated implementation)\n", i);
}

void hello2() {
    printf("Hello2 from include! (Auto-generated implementation)\n");
}

extern "C" {
    int add(int x, int y) { return x + y; }
    int mul(int x, int y) { return x * y; }
    int sub(int x, int y) { return x - y; }
}
