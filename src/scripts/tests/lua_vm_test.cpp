/* 界面脚本求值器的离线断言: 值, 作用域, 控制流, 函数与宿主绑定. */
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/script/lua_vm.h"

using namespace immirror::ui::script;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static void near(const char* label, double got, double want) {
    if (std::fabs(got - want) > 1e-6) {
        std::printf("FAIL %-24s got %.4f want %.4f\n", label, got, want);
        failures += 1;
    } else {
        std::printf("ok   %-24s %.4f\n", label, got);
    }
}

static bool Run(Interpreter& vm, const char* source) {
    if (!vm.Run(source, std::strlen(source))) {
        std::printf("FAIL 运行失败: %s\n", vm.Error().c_str());
        failures += 1;
        return false;
    }
    return true;
}

int main() {
    {
        Interpreter vm;
        Run(vm, "R = 2 + 3 * 4;");
        near("算术优先级", vm.GetGlobal("R").number, 14.0);
    }
    {
        Interpreter vm;
        Run(vm, "R = 2 ^ 3 ^ 2;");
        near("幂运算右结合", vm.GetGlobal("R").number, 512.0);
    }
    {
        Interpreter vm;
        Run(vm, "R = \"a\" .. 1 .. \"b\";");
        check("拼接", vm.GetGlobal("R").text == "a1b");
    }
    {
        Interpreter vm;
        Run(vm, "local t = {1, 2, 3, name = \"x\"}; R = t[2] + #t; N = t.name;");
        near("表索引与长度", vm.GetGlobal("R").number, 5.0);
        check("表字段", vm.GetGlobal("N").text == "x");
    }
    {
        Interpreter vm;
        Run(vm, "function Add(a, b) return a + b end R = Add(3, 4);");
        near("函数调用", vm.GetGlobal("R").number, 7.0);
    }
    {
        Interpreter vm;
        Run(vm, "local function Fib(n) if n < 2 then return n end return Fib(n-1) + Fib(n-2) end "
                "R = Fib(10);");
        near("递归", vm.GetGlobal("R").number, 55.0);
    }
    {
        Interpreter vm;
        Run(vm, "local s = 0; for i = 1, 10 do s = s + i end R = s;");
        near("数值 for", vm.GetGlobal("R").number, 55.0);
    }
    {
        Interpreter vm;
        Run(vm, "local s = 0; local i = 1; while i <= 5 do s = s + i; i = i + 1 end R = s;");
        near("while", vm.GetGlobal("R").number, 15.0);
    }
    {
        Interpreter vm;
        Run(vm, "local n = 0; repeat n = n + 1 until n >= 3 R = n;");
        near("repeat", vm.GetGlobal("R").number, 3.0);
    }
    {
        Interpreter vm;
        Run(vm, "local function G(n) if n >= 90 then return 3 elseif n >= 60 then return 2 "
                "else return 1 end end A = G(95); B = G(70); C = G(10);");
        near("分支一", vm.GetGlobal("A").number, 3.0);
        near("分支二", vm.GetGlobal("B").number, 2.0);
        near("分支三", vm.GetGlobal("C").number, 1.0);
    }
    {
        /* 宿主绑定: 脚本调用原生函数并取回结果. */
        Interpreter vm;
        vm.RegisterNative("host", [](const std::vector<Value>& args) {
            return std::vector<Value>{Value::Number(args.empty() ? 0.0 : args[0].number * 2.0)};
        });
        Run(vm, "R = host(21);");
        near("宿主函数", vm.GetGlobal("R").number, 42.0);
    }
    {
        /* 方法调用: 表里的函数以冒号形式调用. */
        Interpreter vm;
        Run(vm, "local o = { v = 7, get = function(self) return self.v end } R = o:get();");
        near("方法调用", vm.GetGlobal("R").number, 7.0);
    }
    {
        /* 位运算与十六进制常量. */
        Interpreter vm;
        Run(vm, "R = 0xff;");
        near("十六进制", vm.GetGlobal("R").number, 255.0);
    }
    {
        /* 死循环保护. */
        Interpreter vm;
        check("死循环被拦下", !vm.Run("while true do end", 18));
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
