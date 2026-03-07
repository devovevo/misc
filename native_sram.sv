interface native_sram_if #(
		parameter int ADDR_WIDTH = 10,
		parameter int DATA_WIDTH = 256
)(
		input clk
);
		logic en;
		logic we;
		logic [ADDR_WIDTH - 1 : 0] addr;
		logic [DATA_WIDTH - 1 : 0] wdata;
		logic [DATA_WIDTH - 1 : 0] rdata;

		modport device (
				input clk, en, we, addr, wdata
				output rdata
		);

		modport controller (
				input clk, rdata
				output en, we, addr, wdata
		);

endinterface
