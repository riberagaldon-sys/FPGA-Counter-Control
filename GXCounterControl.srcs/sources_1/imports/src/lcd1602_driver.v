`timescale 1ns / 1ps

// HD44780-compatible LCD1602 driver, 8-bit mode, write-only (RW=0).
// line1/line2 contain 16 ASCII characters, leftmost character in [127:120].
module lcd1602_driver #(
    parameter integer CLOCK_HZ = 100_000_000
) (
    input  wire         clk,
    input  wire         rst,
    input  wire [127:0] line1,
    input  wire [127:0] line2,
    output wire [7:0]   lcd_d,
    output wire         lcd_rs,
    output wire         lcd_rw,
    output wire         lcd_e
);
    localparam integer TICK_DIV = CLOCK_HZ / 100_000; // 10 us
    localparam [2:0] STATE_POWER_WAIT = 3'd0;
    localparam [2:0] STATE_ISSUE      = 3'd1;
    localparam [2:0] STATE_WAIT_DONE  = 3'd2;
    localparam [2:0] STATE_DELAY      = 3'd3;

    reg [15:0] tick_counter;
    wire tick = (tick_counter >= TICK_DIV - 1);

    reg [2:0] state;
    reg [15:0] delay_ticks;
    reg [3:0] init_index;
    reg [5:0] operation_index;
    reg initialized;

    reg writer_start;
    reg writer_rs;
    reg [7:0] writer_data;
    wire writer_busy;
    wire writer_done;

    assign lcd_rw = 1'b0;

    function [7:0] init_byte;
        input [3:0] index;
        begin
            case (index)
                4'd0: init_byte = 8'h38; // function set
                4'd1: init_byte = 8'h38;
                4'd2: init_byte = 8'h38;
                4'd3: init_byte = 8'h38; // 8-bit, 2-line
                4'd4: init_byte = 8'h08; // display off
                4'd5: init_byte = 8'h01; // clear display
                4'd6: init_byte = 8'h06; // increment, no shift
                default: init_byte = 8'h0C; // display on, cursor off
            endcase
        end
    endfunction

    function [15:0] init_delay;
        input [3:0] index;
        begin
            case (index)
                4'd0: init_delay = 16'd500; // 5 ms
                4'd1: init_delay = 16'd20;  // 200 us
                4'd2: init_delay = 16'd20;
                4'd5: init_delay = 16'd200; // clear command > 1.5 ms
                default: init_delay = 16'd5;
            endcase
        end
    endfunction

    function [7:0] line_character;
        input [127:0] line;
        input [4:0] index;
        begin
            case (index)
                5'd0:  line_character = line[127:120];
                5'd1:  line_character = line[119:112];
                5'd2:  line_character = line[111:104];
                5'd3:  line_character = line[103:96];
                5'd4:  line_character = line[95:88];
                5'd5:  line_character = line[87:80];
                5'd6:  line_character = line[79:72];
                5'd7:  line_character = line[71:64];
                5'd8:  line_character = line[63:56];
                5'd9:  line_character = line[55:48];
                5'd10: line_character = line[47:40];
                5'd11: line_character = line[39:32];
                5'd12: line_character = line[31:24];
                5'd13: line_character = line[23:16];
                5'd14: line_character = line[15:8];
                default: line_character = line[7:0];
            endcase
        end
    endfunction

    lcd1602_byte_writer byte_writer (
        .clk(clk),
        .rst(rst),
        .tick(tick),
        .start(writer_start),
        .rs_in(writer_rs),
        .data_in(writer_data),
        .busy(writer_busy),
        .done(writer_done),
        .lcd_d(lcd_d),
        .lcd_rs(lcd_rs),
        .lcd_e(lcd_e)
    );

    always @(posedge clk) begin
        if (rst) begin
            tick_counter <= 16'd0;
        end else if (tick) begin
            tick_counter <= 16'd0;
        end else begin
            tick_counter <= tick_counter + 1'b1;
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            state           <= STATE_POWER_WAIT;
            delay_ticks     <= 16'd2000; // 20 ms after FPGA reset release
            init_index      <= 4'd0;
            operation_index <= 6'd0;
            initialized     <= 1'b0;
            writer_start    <= 1'b0;
            writer_rs       <= 1'b0;
            writer_data     <= 8'd0;
        end else begin
            writer_start <= 1'b0;

            case (state)
                STATE_POWER_WAIT: begin
                    if (tick) begin
                        if (delay_ticks == 0)
                            state <= STATE_ISSUE;
                        else
                            delay_ticks <= delay_ticks - 1'b1;
                    end
                end

                STATE_ISSUE: begin
                    if (!writer_busy) begin
                        if (!initialized) begin
                            writer_rs   <= 1'b0;
                            writer_data <= init_byte(init_index);
                        end else if (operation_index == 0) begin
                            writer_rs   <= 1'b0;
                            writer_data <= 8'h80;
                        end else if (operation_index <= 16) begin
                            writer_rs   <= 1'b1;
                            writer_data <= line_character(line1, operation_index - 1'b1);
                        end else if (operation_index == 17) begin
                            writer_rs   <= 1'b0;
                            writer_data <= 8'hC0;
                        end else begin
                            writer_rs   <= 1'b1;
                            writer_data <= line_character(line2, operation_index - 6'd18);
                        end
                        writer_start <= 1'b1;
                        state        <= STATE_WAIT_DONE;
                    end
                end

                STATE_WAIT_DONE: begin
                    if (writer_done) begin
                        delay_ticks <= initialized ? 16'd4 : init_delay(init_index);
                        state       <= STATE_DELAY;
                    end
                end

                default: begin // STATE_DELAY
                    if (tick) begin
                        if (delay_ticks != 0) begin
                            delay_ticks <= delay_ticks - 1'b1;
                        end else if (!initialized) begin
                            if (init_index == 4'd7) begin
                                initialized     <= 1'b1;
                                operation_index <= 6'd0;
                            end else begin
                                init_index <= init_index + 1'b1;
                            end
                            state <= STATE_ISSUE;
                        end else begin
                            if (operation_index == 6'd33)
                                operation_index <= 6'd0;
                            else
                                operation_index <= operation_index + 1'b1;
                            state <= STATE_ISSUE;
                        end
                    end
                end
            endcase
        end
    end
endmodule

module lcd1602_byte_writer (
    input  wire       clk,
    input  wire       rst,
    input  wire       tick,
    input  wire       start,
    input  wire       rs_in,
    input  wire [7:0] data_in,
    output reg        busy,
    output reg        done,
    output reg  [7:0] lcd_d,
    output reg        lcd_rs,
    output reg        lcd_e
);
    reg [1:0] phase;

    always @(posedge clk) begin
        if (rst) begin
            busy   <= 1'b0;
            done   <= 1'b0;
            phase  <= 2'd0;
            lcd_d  <= 8'd0;
            lcd_rs <= 1'b0;
            lcd_e  <= 1'b0;
        end else begin
            done <= 1'b0;
            if (!busy) begin
                lcd_e <= 1'b0;
                if (start) begin
                    lcd_d  <= data_in;
                    lcd_rs <= rs_in;
                    phase  <= 2'd0;
                    busy   <= 1'b1;
                end
            end else if (tick) begin
                case (phase)
                    2'd0: begin
                        lcd_e <= 1'b1;
                        phase <= 2'd1;
                    end
                    2'd1: begin
                        lcd_e <= 1'b0;
                        phase <= 2'd2;
                    end
                    default: begin
                        lcd_e <= 1'b0;
                        busy  <= 1'b0;
                        done  <= 1'b1;
                        phase <= 2'd0;
                    end
                endcase
            end
        end
    end
endmodule
