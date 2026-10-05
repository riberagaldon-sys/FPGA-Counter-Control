`timescale 1ns / 1ps

module key_debounce #(
    parameter integer DEBOUNCE_CYCLES = 2_000_000
) (
    input  wire clk,
    input  wire rst,
    input  wire key_n,
    output reg  press_pulse,
    output reg  pressed
);
    localparam integer COUNTER_WIDTH = $clog2(DEBOUNCE_CYCLES + 1);

    reg sync_ff1;
    reg sync_ff2;
    reg stable_n;
    reg [COUNTER_WIDTH-1:0] stable_counter;

    always @(posedge clk) begin
        if (rst) begin
            sync_ff1      <= 1'b1;
            sync_ff2      <= 1'b1;
            stable_n      <= 1'b1;
            stable_counter <= {COUNTER_WIDTH{1'b0}};
            press_pulse   <= 1'b0;
            pressed       <= 1'b0;
        end else begin
            sync_ff1    <= key_n;
            sync_ff2    <= sync_ff1;
            press_pulse <= 1'b0;

            if (sync_ff2 == stable_n) begin
                stable_counter <= {COUNTER_WIDTH{1'b0}};
            end else if (stable_counter >= DEBOUNCE_CYCLES - 1) begin
                stable_n       <= sync_ff2;
                stable_counter <= {COUNTER_WIDTH{1'b0}};
                pressed        <= ~sync_ff2;
                if (!sync_ff2)
                    press_pulse <= 1'b1;
            end else begin
                stable_counter <= stable_counter + 1'b1;
            end
        end
    end
endmodule
