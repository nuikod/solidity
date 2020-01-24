contract C {
	function d(bytes memory _data) public pure returns (uint8) {
		return abi.decode(_data, (uint8));
	}
}
// ====
// revertStrings: debug
// ----
// d(bytes): 0x20, 0x20, 0x0000000000000000000000000000000000000000000000000000000000000000 -> 0
// d(bytes): 0x100, 0x20, 0x0000000000000000000000000000000000000000000000000000000000000000 -> FAILURE, hex"08c379a0", 0x20, 49, "ABI calldata decoding error: inv", "alid head pointer"
// d(bytes): 0x20, 0x100, 0x0000000000000000000000000000000000000000000000000000000000000000 -> FAILURE, hex"08c379a0", 0x20, 49, "ABI calldata decoding error: inv", "alid data pointer"
