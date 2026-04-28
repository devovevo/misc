`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 04/27/2026 08:05:44 PM
// Design Name: 
// Module Name: blink_module
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


// No special attributes needed for now, let's keep it simple
module blink_module(
    input clk_in,      // Changed name to avoid auto-bus inference
    input blink_in,    // This will connect to your EMIO
    output led_out
);
    reg [26:0] counter = 0;
    
    always @(posedge clk_in) begin
        if (blink_in) begin
            counter <= counter + 1;
        end
    end
    
    assign led_out = counter[26];

endmodule