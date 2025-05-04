#include <cstdio>

void hello() {
    printf("hello world GYB!\n");
}

void hello(int a) {
    printf("%d hello world GYB!\n", a);
}

void hello2() {
    printf("hello world GYB!----------------\n");
}

extern "C" {
    int add(int a, int b) {
        return a + b;
    }
}
