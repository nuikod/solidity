library L {
	function l() public {}
}

contract C {
	function f() public {
		address(L).call(abi.encodeWithSelector(L.l.selector));
	}
}
// ====
// revertStrings: debug
// ----
// f() -> FAILURE, hex"08c379a0", 0x20, 56, "Non view function of library cal", "led without DELEGATECALL"
