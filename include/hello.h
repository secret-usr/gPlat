void hello();
void hello(int);
void hello2();

#ifdef __cplusplus 
extern "C" {
#endif  //__cplusplus
    int add(int x, int y);//加法
    int mul(int x, int y);//乘法
    int sub(int x, int y);//减法
#ifdef __cplusplus
}
#endif  //__cplusplus
