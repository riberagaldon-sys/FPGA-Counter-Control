`timescale 1ns / 1ps

module gx_counter_uart_lcd_top (
    input  wire        CLK_100M,

    // Five user keys plus the board's FPGA_nRST pushbutton, all active low.
    input  wire        KEY0_N,
    input  wire        KEY1_N,
    input  wire        KEY2_N,
    input  wire        KEY3_N,
    input  wire        KEY4_N,
    input  wire        KEY_CLEAR_N,

    // Core-board CH340E USB-UART, 115200 8N1.
    input  wire        UART_RXD,
    output wire        UART_TXD,

    // Existing XO2 Mode0 display buses.
    output wire [31:0] PO,
    output wire [15:0] PIO,

    // Baseboard LCD1602 C-group interface.
    output wire [7:0]  LCD_D,
    output wire        LCD_RS,
    output wire        LCD_RW,
    output wire        LCD_E
);
    localparam integer CLOCK_HZ = 100_000_000;
    localparam integer BAUD = 115_200;
    localparam integer LINK_TIMEOUT_CYCLES = 300_000_000; // 3 seconds
    localparam integer STATUS_PERIOD_CYCLES = 10_000_000; // 100 ms

    // The physical FPGA_nRST switch is connected to ordinary I/O T6.  It is
    // intentionally treated as the sixth user key, not as an async reset.
    reg [21:0] power_on_counter = 22'd0;
    wire rst = ~power_on_counter[21];
    always @(posedge CLK_100M) begin
        if (!power_on_counter[21])
            power_on_counter <= power_on_counter + 1'b1;
    end

    wire key0_pulse;
    wire key1_pulse;
    wire key2_pulse;
    wire key3_pulse;
    wire key4_pulse;
    wire key_clear_pulse;
    wire key0_pressed;
    wire key1_pressed;
    wire key2_pressed;
    wire key3_pressed;
    wire key4_pressed;
    wire key_clear_pressed;

    key_debounce key0_db (.clk(CLK_100M), .rst(rst), .key_n(KEY0_N),
                          .press_pulse(key0_pulse), .pressed(key0_pressed));
    key_debounce key1_db (.clk(CLK_100M), .rst(rst), .key_n(KEY1_N),
                          .press_pulse(key1_pulse), .pressed(key1_pressed));
    key_debounce key2_db (.clk(CLK_100M), .rst(rst), .key_n(KEY2_N),
                          .press_pulse(key2_pulse), .pressed(key2_pressed));
    key_debounce key3_db (.clk(CLK_100M), .rst(rst), .key_n(KEY3_N),
                          .press_pulse(key3_pulse), .pressed(key3_pressed));
    key_debounce key4_db (.clk(CLK_100M), .rst(rst), .key_n(KEY4_N),
                          .press_pulse(key4_pulse), .pressed(key4_pressed));
    key_debounce clear_db (.clk(CLK_100M), .rst(rst), .key_n(KEY_CLEAR_N),
                           .press_pulse(key_clear_pulse), .pressed(key_clear_pressed));

    wire [7:0] rx_data;
    wire rx_valid;
    uart_rx #(.CLOCK_HZ(CLOCK_HZ), .BAUD(BAUD)) uart_receiver (
        .clk(CLK_100M), .rst(rst), .rx(UART_RXD),
        .data(rx_data), .valid(rx_valid)
    );

    // PC commands are deliberately small and deterministic:
    // T/U/D/L = F1..F4 controls, C = F9 clear, F = F10 frequency,
    // P = heartbeat and Q = query.  Every command is newline terminated.
    reg command_line_start;
    reg cmd_ping;
    reg cmd_pause_toggle;
    reg cmd_increment;
    reg cmd_decrement;
    reg cmd_led_direction;
    reg cmd_clear;
    reg cmd_frequency;
    reg cmd_query;

    always @(posedge CLK_100M) begin
        if (rst) begin
            command_line_start <= 1'b1;
            cmd_ping           <= 1'b0;
            cmd_pause_toggle   <= 1'b0;
            cmd_increment      <= 1'b0;
            cmd_decrement      <= 1'b0;
            cmd_led_direction  <= 1'b0;
            cmd_clear          <= 1'b0;
            cmd_frequency      <= 1'b0;
            cmd_query          <= 1'b0;
        end else begin
            cmd_ping      <= 1'b0;
            cmd_pause_toggle  <= 1'b0;
            cmd_increment     <= 1'b0;
            cmd_decrement     <= 1'b0;
            cmd_led_direction <= 1'b0;
            cmd_clear     <= 1'b0;
            cmd_frequency <= 1'b0;
            cmd_query     <= 1'b0;

            if (rx_valid) begin
                if (rx_data == 8'h0A) begin
                    command_line_start <= 1'b1;
                end else if (rx_data != 8'h0D) begin
                    if (command_line_start) begin
                        case (rx_data)
                            "P", "p": cmd_ping      <= 1'b1;
                            "T", "t": cmd_pause_toggle  <= 1'b1;
                            "U", "u": cmd_increment     <= 1'b1;
                            "D", "d": cmd_decrement     <= 1'b1;
                            "L", "l": cmd_led_direction <= 1'b1;
                            "C", "c": cmd_clear     <= 1'b1;
                            "F", "f": cmd_frequency <= 1'b1;
                            "Q", "q": cmd_query     <= 1'b1;
                            default: begin end
                        endcase
                    end
                    command_line_start <= 1'b0;
                end
            end
        end
    end

    wire valid_command = cmd_ping | cmd_pause_toggle | cmd_increment |
                         cmd_decrement | cmd_led_direction | cmd_clear |
                         cmd_frequency | cmd_query;
    reg [28:0] link_timeout_counter;
    reg link_online;

    always @(posedge CLK_100M) begin
        if (rst) begin
            link_timeout_counter <= 29'd0;
            link_online          <= 1'b0;
        end else if (valid_command) begin
            link_timeout_counter <= 29'd0;
            link_online          <= 1'b1;
        end else if (link_online) begin
            if (link_timeout_counter >= LINK_TIMEOUT_CYCLES - 1) begin
                link_timeout_counter <= 29'd0;
                link_online          <= 1'b0;
            end else begin
                link_timeout_counter <= link_timeout_counter + 1'b1;
            end
        end
    end

    wire pause_any        = key0_pulse | cmd_pause_toggle;
    wire increment_any    = key1_pulse | cmd_increment;
    wire decrement_any    = key2_pulse | cmd_decrement;
    wire led_direction_any = key3_pulse | cmd_led_direction;
    wire clear_any        = key_clear_pulse | cmd_clear;
    wire frequency_any    = key4_pulse | cmd_frequency;
    wire [31:0] count;
    wire running;
    wire count_down;
    wire led_right;
    wire [2:0] frequency_index;
    wire [31:0] divider_value;
    wire [11:0] led_onehot;
    wire [3:0] led_index;
    wire [3:0] last_action;
    wire [15:0] event_counter;
    wire core_event_pulse;

    counter_core counter_logic (
        .clk(CLK_100M),
        .rst(rst),
        .pause_toggle_event(pause_any),
        .clear_event(clear_any),
        .increment_event(increment_any),
        .decrement_event(decrement_any),
        .led_direction_event(led_direction_any),
        .frequency_event(frequency_any),
        .count(count),
        .running(running),
        .count_down(count_down),
        .led_right(led_right),
        .frequency_index(frequency_index),
        .divider_value(divider_value),
        .led_onehot(led_onehot),
        .led_index(led_index),
        .last_action(last_action),
        .event_counter(event_counter),
        .event_pulse(core_event_pulse)
    );

    assign PO  = count;
    assign PIO = {4'b0000, led_onehot};

    reg [63:0] action_field;
    reg [63:0] frequency_field;
    wire [127:0] lcd_line1 = link_online ? "UART:ONLINE     "
                                               : "UART:OFFLINE    ";
    wire [127:0] lcd_line2 = {action_field, frequency_field};

    always @(*) begin
        case (last_action)
            4'h1: action_field = "RUN     ";
            4'h2: action_field = "PAUSE   ";
            4'h3: action_field = "CLEAR   ";
            4'h4: action_field = "INC     ";
            4'h5: action_field = "DEC     ";
            4'h6: action_field = "LED->   ";
            4'h7: action_field = "LED<-   ";
            4'h8: action_field = "FREQ    ";
            default: action_field = "POWERON ";
        endcase

        case (frequency_index)
            3'd0: frequency_field = "F:0.25Hz";
            3'd1: frequency_field = "F:0.50Hz";
            3'd2: frequency_field = "F:1.00Hz";
            3'd3: frequency_field = "F:2.00Hz";
            default: frequency_field = "F:4.00Hz";
        endcase
    end

    lcd1602_driver #(.CLOCK_HZ(CLOCK_HZ)) lcd_driver (
        .clk(CLK_100M), .rst(rst),
        .line1(lcd_line1), .line2(lcd_line2),
        .lcd_d(LCD_D), .lcd_rs(LCD_RS), .lcd_rw(LCD_RW), .lcd_e(LCD_E)
    );

    reg [23:0] status_period_counter;
    reg periodic_status_pulse;
    always @(posedge CLK_100M) begin
        if (rst) begin
            status_period_counter <= 24'd0;
            periodic_status_pulse <= 1'b0;
        end else begin
            periodic_status_pulse <= 1'b0;
            if (status_period_counter >= STATUS_PERIOD_CYCLES - 1) begin
                status_period_counter <= 24'd0;
                periodic_status_pulse <= 1'b1;
            end else begin
                status_period_counter <= status_period_counter + 1'b1;
            end
        end
    end

    reg link_online_previous;
    wire link_changed = (link_online != link_online_previous);
    always @(posedge CLK_100M) begin
        if (rst)
            link_online_previous <= 1'b0;
        else
            link_online_previous <= link_online;
    end

    reg status_pending;
    reg status_send;
    wire status_busy;
    always @(posedge CLK_100M) begin
        if (rst) begin
            status_pending <= 1'b0;
            status_send    <= 1'b0;
        end else begin
            status_send <= 1'b0;
            if (periodic_status_pulse | core_event_pulse | cmd_query | link_changed)
                status_pending <= 1'b1;

            if (status_pending && !status_busy) begin
                status_pending <= 1'b0;
                status_send    <= 1'b1;
            end
        end
    end

    wire [7:0] tx_data;
    wire tx_valid;
    wire tx_ready;

    status_sender sender (
        .clk(CLK_100M), .rst(rst), .send(status_send),
        .count(count), .running(running), .count_down(count_down),
        .led_right(led_right), .frequency_index(frequency_index),
        .divider_value(divider_value), .led_onehot(led_onehot),
        .action_code({4'b0000, last_action}), .link_online(link_online),
        .event_counter(event_counter), .busy(status_busy),
        .tx_data(tx_data), .tx_valid(tx_valid), .tx_ready(tx_ready)
    );

    uart_tx #(.CLOCK_HZ(CLOCK_HZ), .BAUD(BAUD)) uart_transmitter (
        .clk(CLK_100M), .rst(rst), .data(tx_data), .valid(tx_valid),
        .ready(tx_ready), .tx(UART_TXD)
    );
endmodule
