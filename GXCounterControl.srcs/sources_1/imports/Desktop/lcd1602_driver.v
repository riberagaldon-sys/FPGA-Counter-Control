`timescale 1ns / 1ps

// HD44780-compatible LCD1602 driver, 4-bit mode, write-only (RW=0).
// Only LCD_D[7:4] carry data.  LCD_D[3:0] are held low so the display does
// not depend on the baseboard's unreliable D0/D1 paths.
// line1/line2 contain 16 ASCII characters, leftmost character in [127:120].
module lcd1602_driver #(
    parameter integer CLOCK_HZ = 100_000_000,
    parameter integer REFRESH_INTERVAL_CYCLES = CLOCK_HZ
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
    localparam integer TICK_DIV_RAW = CLOCK_HZ / 100_000;
    localparam integer TICK_DIV = (TICK_DIV_RAW < 1) ? 1 : TICK_DIV_RAW; // 10 us
    localparam [2:0] STATE_POWER_WAIT = 3'd0;
    localparam [2:0] STATE_ISSUE      = 3'd1;
    localparam [2:0] STATE_WAIT_DONE  = 3'd2;
    localparam [2:0] STATE_DELAY      = 3'd3;
    localparam [2:0] STATE_IDLE       = 3'd4;

    reg [15:0] tick_counter;
    wire tick = (tick_counter >= TICK_DIV - 1);

    reg [2:0] state;
    reg [15:0] delay_ticks;
    reg [3:0] init_index;
    reg [5:0] operation_index;
    reg initialized;
    reg [31:0] refresh_counter;
    reg [127:0] line1_latched;
    reg [127:0] line2_latched;

    reg writer_start;
    reg writer_rs;
    reg [7:0] writer_data;
    reg writer_nibble_only;
    wire writer_busy;
    wire writer_done;

    assign lcd_rw = 1'b0;

    function [7:0] init_byte;
        input [3:0] index;
        begin
            case (index)
                // The first four values are single high-nibble transfers
                // required by the HD44780 4-bit power-on sequence.
                4'd0: init_byte = 8'h30;
                4'd1: init_byte = 8'h30;
                4'd2: init_byte = 8'h30;
                4'd3: init_byte = 8'h20; // select 4-bit interface
                4'd4: init_byte = 8'h28; // 4-bit, 2-line, 5x8 font
                4'd5: init_byte = 8'h08; // display off
                4'd6: init_byte = 8'h01; // clear display
                4'd7: init_byte = 8'h06; // increment, no display shift
                default: init_byte = 8'h0C; // display on, cursor off
            endcase
        end
    endfunction

    function init_nibble_only;
        input [3:0] index;
        begin
            init_nibble_only = (index <= 4'd3);
        end
    endfunction

    function [15:0] init_delay;
        input [3:0] index;
        begin
            case (index)
                4'd0: init_delay = 16'd500; // 5 ms
                4'd1: init_delay = 16'd20;  // 200 us
                4'd2: init_delay = 16'd20;
                4'd3: init_delay = 16'd20;
                4'd6: init_delay = 16'd200; // clear command > 1.5 ms
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

    lcd1602_4bit_writer byte_writer (
        .clk(clk),
        .rst(rst),
        .tick(tick),
        .start(writer_start),
        .rs_in(writer_rs),
        .data_in(writer_data),
        .nibble_only(writer_nibble_only),
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
            refresh_counter <= 32'd0;
            line1_latched   <= "                ";
            line2_latched   <= "                ";
            writer_start    <= 1'b0;
            writer_rs       <= 1'b0;
            writer_data     <= 8'd0;
            writer_nibble_only <= 1'b0;
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
                            writer_rs          <= 1'b0;
                            writer_data        <= init_byte(init_index);
                            writer_nibble_only <= init_nibble_only(init_index);
                        end else if (operation_index == 0) begin
                            writer_rs          <= 1'b0;
                            writer_data        <= 8'h80;
                            writer_nibble_only <= 1'b0;
                        end else if (operation_index <= 16) begin
                            writer_rs          <= 1'b1;
                            writer_data        <= line_character(line1_latched,
                                                                 operation_index - 1'b1);
                            writer_nibble_only <= 1'b0;
                        end else if (operation_index == 17) begin
                            writer_rs          <= 1'b0;
                            writer_data        <= 8'hC0;
                            writer_nibble_only <= 1'b0;
                        end else begin
                            writer_rs          <= 1'b1;
                            writer_data        <= line_character(line2_latched,
                                                                 operation_index - 6'd18);
                            writer_nibble_only <= 1'b0;
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

                STATE_IDLE: begin
                    // Hold the LCD bus still between complete frames.  A new
                    // frame starts immediately when either 16-character line
                    // changes, or once per second as a recovery refresh.
                    if ((line1 != line1_latched) ||
                        (line2 != line2_latched) ||
                        (refresh_counter >= REFRESH_INTERVAL_CYCLES - 1)) begin
                        line1_latched   <= line1;
                        line2_latched   <= line2;
                        refresh_counter <= 32'd0;
                        operation_index <= 6'd0;
                        state           <= STATE_ISSUE;
                    end else begin
                        refresh_counter <= refresh_counter + 1'b1;
                    end
                end

                default: begin // STATE_DELAY
                    if (tick) begin
                        if (delay_ticks != 0) begin
                            delay_ticks <= delay_ticks - 1'b1;
                        end else if (!initialized) begin
                            if (init_index == 4'd8) begin
                                initialized     <= 1'b1;
                                operation_index <= 6'd0;
                                refresh_counter <= 32'd0;
                                line1_latched   <= line1;
                                line2_latched   <= line2;
                            end else begin
                                init_index <= init_index + 1'b1;
                            end
                            state <= STATE_ISSUE;
                        end else begin
                            if (operation_index == 6'd33) begin
                                operation_index <= 6'd0;
                                refresh_counter <= 32'd0;
                                state           <= STATE_IDLE;
                            end else begin
                                operation_index <= operation_index + 1'b1;
                                state <= STATE_ISSUE;
                            end
                        end
                    end
                end
            endcase
        end
    end
endmodule

module lcd1602_4bit_writer (
    input  wire       clk,
    input  wire       rst,
    input  wire       tick,
    input  wire       start,
    input  wire       rs_in,
    input  wire [7:0] data_in,
    input  wire       nibble_only,
    output reg        busy,
    output reg        done,
    output reg  [7:0] lcd_d,
    output reg        lcd_rs,
    output reg        lcd_e
);
    reg [2:0] phase;
    reg [3:0] low_nibble;
    reg nibble_only_latched;

    always @(posedge clk) begin
        if (rst) begin
            busy   <= 1'b0;
            done   <= 1'b0;
            phase  <= 3'd0;
            lcd_d  <= 8'd0;
            lcd_rs <= 1'b0;
            lcd_e  <= 1'b0;
            low_nibble <= 4'd0;
            nibble_only_latched <= 1'b0;
        end else begin
            done <= 1'b0;
            if (!busy) begin
                lcd_e <= 1'b0;
                if (start) begin
                    // In 4-bit mode the upper nibble is transferred first.
                    // Lower physical data pins remain at zero at all times.
                    lcd_d               <= {data_in[7:4], 4'b0000};
                    low_nibble          <= data_in[3:0];
                    nibble_only_latched <= nibble_only;
                    lcd_rs              <= rs_in;
                    phase               <= 3'd0;
                    busy                <= 1'b1;
                end
            end else if (tick) begin
                case (phase)
                    3'd0: begin
                        lcd_e <= 1'b1;
                        phase <= 3'd1;
                    end
                    3'd1: begin
                        lcd_e <= 1'b0;
                        phase <= 3'd2;
                    end
                    3'd2: begin
                        if (nibble_only_latched) begin
                            busy  <= 1'b0;
                            done  <= 1'b1;
                            phase <= 3'd0;
                        end else begin
                            lcd_d <= {low_nibble, 4'b0000};
                            phase <= 3'd3;
                        end
                    end
                    3'd3: begin
                        lcd_e <= 1'b1;
                        phase <= 3'd4;
                    end
                    3'd4: begin
                        lcd_e <= 1'b0;
                        phase <= 3'd5;
                    end
                    default: begin
                        lcd_e <= 1'b0;
                        busy  <= 1'b0;
                        done  <= 1'b1;
                        phase <= 3'd0;
                    end
                endcase
            end
        end
    end
endmodule
