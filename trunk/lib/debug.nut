local function test(message, lhs, rhs = true)
{
	if (lhs != rhs)
		alert(message, format("result = %s\nexpected = %s", lhs, rhs), "regression failed", ERROR)
}

test("startswith 1", "ABC".startswith("ABC"))
test("startswith 2", "ABCD".startswith("xABC", 1))
test("startswith 3", "ABCD".startswith("xABCx" 1, 4))
test("startswith 4", "AB".startswith("ABC"), false)
test("startswith 5", "A_CD".startswith("xABC", 1), false)
test("startswith 6", "A_CD".startswith("xABCx" 1, 4), false)

test("endswith 1", "ABC".endswith("ABC"))
test("endswith 2", "XABC".endswith("xABC", 1))
test("endswith 3", "XABC".endswith("xABCx" 1, 4))
test("endswith 4", "BC".endswith("ABC"), false)
test("endswith 5", "XA_C".endswith("xABC", 1), false)
test("endswith 6", "XA_C".endswith("xABCx" 1, 4), false)

test("replace 1", "AAA".replace("A", "B"), "BBB")
test("replace 2", "AAA".replace("A", "XYZ"), "XYZXYZXYZ")
test("replace 3", "ABCDABCDABCD".replace("ABC", "X"), "XDXDXD")
test("replace 4", "AAA".replace("A", "AA"), "AAAAAA")
test("replace 4", "ABCABC".replace("B", null), "ACAC")

test("rfind 1", "ABCABC".rfind("AB"), 3)
test("rfind 2", "ABCABC".rfind("C"), 5)
test("rfind 3", "ABCABC".rfind("AB", 4), 0)
test("rfind 4", "ABCABC".rfind("C", 3), 2)

test("repeat 1", "ABC".repeat(0), "")
test("repeat 2", "ABC".repeat(1), "ABC")
test("repeat 3", "ABC".repeat(2), "ABCABC")
