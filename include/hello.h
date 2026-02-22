void hello();
void hello(int);
void hello2();

#ifdef __cplusplus 
extern "C" {
#endif  //__cplusplus
    int add(int x, int y);
    int mul(int x, int y);
    int sub(int x, int y);
#ifdef __cplusplus
}
#endif  //__cplusplus
