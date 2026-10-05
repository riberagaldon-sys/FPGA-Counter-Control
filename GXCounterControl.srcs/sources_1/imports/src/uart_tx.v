`timescale 1ns / 1ps

module uart_tx #(
    parameter integer CLOCK_HZ = 100_000_000,
    parameter integer BAUD     = 115_200
) (
    input  wire       clk,
    input  wire       rst,
    input  wire [7:0] data,
    input  wire       valid,
    output wire       ready,
    output reg        tx
);
    localparam integer CLKS_PER_BIT = CLOCK_HZ / BAUD;
    localparam [1:0] IDLE  = 2'd0;
    localparam [1:0] START = 2'd1;
    localparam [1:0] DATA  = 2'd2;
    localparam [1:0] STOP  = 2'd3;

    reg [1:0] state;
    reg [15:0] clock_counter;
    reg [2:0] bit_index;
    reg [7:0] data_latch;

    assign ready = (state == IDLE);

    always @(posedge clk) begin
        if (rst) begin
            state         <= IDLE;
            clock_counter <= 16'd0;
            bit_index     <= 3'd0;
            data_latch    <= 8'd0;
            tx            <= 1'b1;
        end else begin
            case (state)
                IDLE: begin
                    tx            <= 1'b1;
                    clock_counter <= 16'd0;
                    bit_index     <= 3'd0;
                    if (valid) begin
                        data_latch <= data;
                        tx         <= 1'b0;
                        state      <= START;
                    end
                end

                START: begin
                    if (clock_counter >= CLKS_PER_BIT - 1) begin
                        clock_counter <= 16'd0;
                        tx            <= data_latch[0];
                        state         <= DATA;
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end

                DATA: begin
                    if (clock_counter >= CLKS_PER_BIT - 1) begin
                        clock_counter <= 16'd0;
                        if (bit_index == 3'd7) begin
                            bit_index <= 3'd0;
                            tx        <= 1'b1;
                            state     <= STOP;
                        end else begin
                            bit_index <= bit_index + 1'b1;
                            tx        <= data_latch[bit_index + 1'b1];
                        end
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end

                default: begin // STOP
                    if (clock_counter >= CLKS_PER_BIT - 1) begin
                        clock_counter <= 16'd0;
                        tx            <= 1'b1;
                        state         <= IDLE;
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end
            endcase
        end
    end
endmodule
