`timescale 1ns / 1ps

module uart_rx #(
    parameter integer CLOCK_HZ = 100_000_000,
    parameter integer BAUD     = 115_200
) (
    input  wire       clk,
    input  wire       rst,
    input  wire       rx,
    output reg [7:0]  data,
    output reg        valid
);
    localparam integer CLKS_PER_BIT = CLOCK_HZ / BAUD;
    localparam integer HALF_BIT     = CLKS_PER_BIT / 2;
    localparam [1:0] IDLE  = 2'd0;
    localparam [1:0] START = 2'd1;
    localparam [1:0] DATA  = 2'd2;
    localparam [1:0] STOP  = 2'd3;

    reg rx_meta;
    reg rx_sync;
    reg [1:0] state;
    reg [15:0] clock_counter;
    reg [2:0] bit_index;
    reg [7:0] shift_reg;

    always @(posedge clk) begin
        rx_meta <= rx;
        rx_sync <= rx_meta;

        if (rst) begin
            state         <= IDLE;
            clock_counter <= 16'd0;
            bit_index     <= 3'd0;
            shift_reg     <= 8'd0;
            data          <= 8'd0;
            valid         <= 1'b0;
            rx_meta       <= 1'b1;
            rx_sync       <= 1'b1;
        end else begin
            valid <= 1'b0;
            case (state)
                IDLE: begin
                    clock_counter <= 16'd0;
                    bit_index     <= 3'd0;
                    if (!rx_sync)
                        state <= START;
                end

                START: begin
                    if (clock_counter >= HALF_BIT - 1) begin
                        clock_counter <= 16'd0;
                        if (!rx_sync)
                            state <= DATA;
                        else
                            state <= IDLE;
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end

                DATA: begin
                    if (clock_counter >= CLKS_PER_BIT - 1) begin
                        clock_counter       <= 16'd0;
                        shift_reg[bit_index] <= rx_sync;
                        if (bit_index == 3'd7) begin
                            bit_index <= 3'd0;
                            state     <= STOP;
                        end else begin
                            bit_index <= bit_index + 1'b1;
                        end
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end

                default: begin // STOP
                    if (clock_counter >= CLKS_PER_BIT - 1) begin
                        clock_counter <= 16'd0;
                        state         <= IDLE;
                        if (rx_sync) begin
                            data  <= shift_reg;
                            valid <= 1'b1;
                        end
                    end else begin
                        clock_counter <= clock_counter + 1'b1;
                    end
                end
            endcase
        end
    end
endmodule
