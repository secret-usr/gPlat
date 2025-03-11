// See https://aka.ms/new-console-template for more information
using System.Runtime.InteropServices;

[DllImport("/home/gyb/projects/x64/Debug/libgPlature.so", EntryPoint = "add", CallingConvention = CallingConvention.Cdecl)]
static extern int add(int a, int b);

Console.WriteLine("古云波 你好！Hello, World!");

// 调用C++函数
int result = add(9, 100);
Console.WriteLine("Result of Add: " + result);

Console.WriteLine("sizeof(MyStruct)={0}", Marshal.SizeOf<MyStruct>());

string input;
#pragma warning disable CS8600 // 将 null 字面量或可能为 null 的值转换为非 null 类型。
input = Console.ReadLine();
#pragma warning restore CS8600 // 将 null 字面量或可能为 null 的值转换为非 null 类型。

Console.WriteLine("You typed: " + input);

[StructLayout(LayoutKind.Sequential, Pack = 8, CharSet = CharSet.Ansi)]     //必备
public class MyStruct
{
    public bool flag;
    public Int32 a;
    public float b;
    public DateTime dt;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 50)]                    //必备
    public required String str1;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 50)]                   //必备
    public required char[] str2;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]                    //必备
    public required int[] c;
}
