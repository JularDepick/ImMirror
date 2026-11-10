#include "test_framework.h"
#include "test_support.h"

using namespace immirror::script;
using namespace immirror::script::test;

TEST_CASE(arith_priority) {
    const Outcome outcome = Run("return 1 + 2 * 3 - 8 / 4 % 3;");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Count(), 1);
    CHECK_INT(outcome.Int(0), 5);
}

TEST_CASE(arith_int_and_float) {
    const Outcome outcome = Run(R"SRC(
        return 7 / 2, 7.0 / 2.0, 1.5 * 2.0 + 0.25, 5 % 3, 2 - 10;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 3);
    CHECK_REAL(outcome.Real(1), 3.5);
    CHECK_REAL(outcome.Real(2), 3.25);
    CHECK_INT(outcome.Int(3), 2);
    CHECK_INT(outcome.Int(4), -8);
}

TEST_CASE(unary_and_negate) {
    const Outcome outcome = Run("return -2 * 3 + 10, !true, !nil, !0, -(-5);");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 4);
    CHECK(!outcome.Bool(1));
    CHECK(outcome.Bool(2));
    CHECK(!outcome.Bool(3));
    CHECK_INT(outcome.Int(4), 5);
}

TEST_CASE(comparison_ops) {
    const Outcome outcome =
        Run("return 1 < 2, 2 <= 2, 3 > 4, 1 == 1.0, \"a\" < \"b\", 1 != 2;");
    CHECK(outcome.ok);
    CHECK(outcome.Bool(0));
    CHECK(outcome.Bool(1));
    CHECK(!outcome.Bool(2));
    CHECK(outcome.Bool(3));
    CHECK(outcome.Bool(4));
    CHECK(outcome.Bool(5));
}

TEST_CASE(short_circuit) {
    const Outcome outcome = Run(R"SRC(
        var state = { calls = 0 };
        fn bump(s) { s.calls = s.calls + 1; return true; }
        var a = false && bump(state);
        var b = true || bump(state);
        var c = nil || 5;
        var d = 0 || 5;
        return a, b, state.calls, c, d;
    )SRC");
    CHECK(outcome.ok);
    CHECK(!outcome.Bool(0));
    CHECK(outcome.Bool(1));
    CHECK_INT(outcome.Int(2), 0);
    CHECK_INT(outcome.Int(3), 5);
    CHECK_INT(outcome.Int(4), 0);
}

TEST_CASE(if_elseif_else) {
    const Outcome outcome = Run(R"SRC(
        fn classify(x) {
            if (x < 3) { return "small"; }
            else if (x < 10) { return "mid"; }
            else { return "big"; }
        }
        return classify(1), classify(5), classify(20);
    )SRC");
    CHECK(outcome.ok);
    CHECK_TEXT(outcome.Text(0), "small");
    CHECK_TEXT(outcome.Text(1), "mid");
    CHECK_TEXT(outcome.Text(2), "big");
}

TEST_CASE(while_break_continue) {
    const Outcome outcome = Run(R"SRC(
        var sum = 0;
        var i = 1;
        while (i <= 10) {
            if (i == 3) { i = i + 1; continue; }
            if (i == 7) { break; }
            sum = sum + i;
            i = i + 1;
        }
        return sum;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 18);
}

TEST_CASE(for_numeric_loop) {
    const Outcome outcome = Run(R"SRC(
        var factorial = 1;
        for (var i = 1; i <= 5; i = i + 1) { factorial = factorial * i; }
        var count = 0;
        for (var j = 10; j > 0; j -= 1) { count += 1; }
        var skipped = 0;
        for (var k = 0; k < 10; k = k + 1) {
            if (k % 2 == 0) { continue; }
            skipped = skipped + 1;
        }
        return factorial, count, skipped;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 120);
    CHECK_INT(outcome.Int(1), 10);
    CHECK_INT(outcome.Int(2), 5);
}

TEST_CASE(nested_loops) {
    const Outcome outcome = Run(R"SRC(
        var total = 0;
        for (var a = 0; a < 4; a = a + 1) {
            for (var b = 0; b < 3; b = b + 1) {
                if (b == 1) { continue; }
                total = total + 1;
            }
        }
        return total;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 8);
}

TEST_CASE(recursion_and_multi_return) {
    const Outcome outcome = Run(R"SRC(
        fn fib(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }
        fn divmod(a, b) { return a / b, a % b; }
        var q, r = divmod(17, 5);
        return fib(10), q, r;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 55);
    CHECK_INT(outcome.Int(1), 3);
    CHECK_INT(outcome.Int(2), 2);
}

TEST_CASE(multi_return_forward) {
    const Outcome outcome = Run(R"SRC(
        fn pair() { return 1, 2, 3; }
        fn forward() { return pair(); }
        return forward();
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Count(), 3);
    CHECK_INT(outcome.Int(0), 1);
    CHECK_INT(outcome.Int(1), 2);
    CHECK_INT(outcome.Int(2), 3);
}

TEST_CASE(multi_assign_swap) {
    const Outcome outcome = Run(R"SRC(
        var a = 1;
        var b = 2;
        var c = 3;
        a, b, c = c, a, b;
        var x, y = 9, 8;
        return a, b, c, x, y;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 3);
    CHECK_INT(outcome.Int(1), 1);
    CHECK_INT(outcome.Int(2), 2);
    CHECK_INT(outcome.Int(3), 9);
    CHECK_INT(outcome.Int(4), 8);
}

TEST_CASE(table_basic) {
    const Outcome outcome = Run(R"SRC(
        var t = { 10, 20, 30, name = "abc", ["x"] = 5 };
        var sum = t[1] + t[2] + t[3];
        t.y = 99;
        t[4] = 40;
        return sum, t.name, t.x, t.y, len(t);
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 60);
    CHECK_TEXT(outcome.Text(1), "abc");
    CHECK_INT(outcome.Int(2), 5);
    CHECK_INT(outcome.Int(3), 99);
    CHECK_INT(outcome.Int(4), 4);
}

TEST_CASE(table_nested_and_dynamic_key) {
    const Outcome outcome = Run(R"SRC(
        var t = { a = { b = { c = 7 } } };
        t.a.b.c = t.a.b.c + 1;
        var key = "a";
        return t[key].b.c, type(t), type(t.a), type(t.missing);
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 8);
    CHECK_TEXT(outcome.Text(1), "table");
    CHECK_TEXT(outcome.Text(2), "table");
    CHECK_TEXT(outcome.Text(3), "nil");
}

TEST_CASE(table_of_functions) {
    const Outcome outcome = Run(R"SRC(
        var ops = { add = fn (a, b) { return a + b; }, mul = fn (a, b) { return a * b; } };
        return ops.add(2, 3), ops["mul"](4, 5);
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 5);
    CHECK_INT(outcome.Int(1), 20);
}

TEST_CASE(string_ops) {
    const Outcome outcome = Run(R"SRC(
        var s = "hello" + " " + "world";
        var n = len(s);
        var t = "tab\tend";
        var u = "中文";
        var v = "n=" + 5 + ", b=" + true;
        return s, n, t, u, v;
    )SRC");
    CHECK(outcome.ok);
    CHECK_TEXT(outcome.Text(0), "hello world");
    CHECK_INT(outcome.Int(1), 11);
    CHECK_TEXT(outcome.Text(2), "tab\tend");
    CHECK_TEXT(outcome.Text(3), "中文");
    CHECK_TEXT(outcome.Text(4), "n=5, b=true");
}

TEST_CASE(string_compare) {
    const Outcome outcome = Run(R"SRC(
        return "abc" == "abc", "abc" < "abd", "ab" < "abc", "b" > "abc";
    )SRC");
    CHECK(outcome.ok);
    CHECK(outcome.Bool(0));
    CHECK(outcome.Bool(1));
    CHECK(outcome.Bool(2));
    CHECK(outcome.Bool(3));
}

TEST_CASE(const_variable_rejected) {
    const CompileResult compiled = CompileOnly(R"SRC(
        const k = 1;
        k = 2;
    )SRC");
    CHECK(!compiled.ok);
    CHECK(!compiled.diagnostics.empty());
}

TEST_CASE(syntax_error_reported) {
    const CompileResult compiled = CompileOnly("fn f( { return 1; }");
    CHECK(!compiled.ok);
    CHECK(!compiled.diagnostics.empty());
    CHECK(compiled.diagnostics[0].line >= 1);
}

TEST_CASE(comment_forms) {
    const Outcome outcome = Run(R"SRC(
        // 行注释
        var a = 1; /* 块注释
        跨行 */
        var b = 2; // 行尾注释
        return a + b;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 3);
}

TEST_CASE(global_function_mutual_recursion) {
    const Outcome outcome = Run(R"SRC(
        fn is_even(n) { if (n == 0) { return true; } return is_odd(n - 1); }
        fn is_odd(n) { if (n == 0) { return false; } return is_even(n - 1); }
        return is_even(10), is_odd(7);
    )SRC");
    CHECK(outcome.ok);
    CHECK(outcome.Bool(0));
    CHECK(outcome.Bool(1));
}

TEST_CASE(nested_function_local) {
    const Outcome outcome = Run(R"SRC(
        fn outer(a) {
            fn inner(b) { return b * 2; }
            return inner(a) + 1;
        }
        return outer(5);
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 11);
}

TEST_CASE(capture_rejected) {
    const CompileResult compiled = CompileOnly(R"SRC(
        var outer = 1;
        fn inner() { return outer; }
        return inner();
    )SRC");
    CHECK(!compiled.ok);
    CHECK(!compiled.diagnostics.empty());
}

TEST_CASE(binding_world_roundtrip) {
    TestWorld world;
    const Outcome outcome = RunWithWorld(R"SRC(
        var id = world_get_block(1, 2, 3);
        var ok = world_set_block(4, 5, 6, id + 1);
        log("hello", 42);
        return id, ok;
    )SRC", world);
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 7);
    CHECK(outcome.Bool(1));
    CHECK_INT(world.lastX, 4);
    CHECK_INT(world.lastZ, 6);
    CHECK_INT(world.lastPlaced, 8);
    CHECK_INT(world.logs.size(), 1);
    CHECK_TEXT(world.logs[0], "hello 42");
}

TEST_CASE(binding_player_position_multi) {
    TestWorld world;
    const Outcome outcome =
        RunWithWorld("var x, y, z = player_position(); return x, y, z;", world);
    CHECK(outcome.ok);
    CHECK_REAL(outcome.Real(0), 1.5);
    CHECK_REAL(outcome.Real(1), 2.5);
    CHECK_REAL(outcome.Real(2), 3.5);
}

TEST_CASE(binding_type_error) {
    const Outcome outcome = Run("return world_get_block(\"a\", 1, 2);");
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::BindingType);
}

TEST_CASE(binding_arity_error) {
    const Outcome outcome = Run("return world_get_block(1);");
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::ArityMismatch);
}

TEST_CASE(call_non_function) {
    const Outcome outcome = Run("var x = 1; return x(2);");
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::NotCallable);
}

TEST_CASE(index_non_table) {
    const Outcome outcome = Run("var x = 1; return x.field;");
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::NotIndexable);
}

TEST_CASE(divisor_zero) {
    const Outcome outcome = Run("return 1 / 0;");
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::DivisorZero);
}

TEST_CASE(integer_overflow_promotes) {
    const Outcome outcome = Run("var a = 9223372036854775807; return a + 1;");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Count(), 1);
    CHECK(outcome.results[0].type == ValueType::Double);
}

TEST_CASE(global_state_visible_in_run) {
    const Outcome outcome = Run(R"SRC(
        counter = 0;
        fn bump() { counter = counter + 1; }
        bump();
        bump();
        return counter;
    )SRC");
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 2);
}
