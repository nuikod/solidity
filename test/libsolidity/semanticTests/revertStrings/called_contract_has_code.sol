contract C {
	function f() external {}
	function g() external {
		C c = C(0x0000000000000000000000000000000000000000000000000000000000000000);
		c.f();
	}
	function h() external {
		address(0x0000000000000000000000000000000000000000000000000000000000000000).delegatecall("");
	}
}
// ====
// revertStrings: debug
// ----
// g() -> FAILURE, hex"08c379a0", 0x20, 37, "Target contract does not contain", " code"
// h() -> FAILURE, hex"08c379a0", 0x20, 37, "Target contract does not contain", " code"
