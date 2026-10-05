`timescale 1ns / 1ps

// Core behavior derived from the reference experiment, rewritten for six
// direct core-board pushbuttons and a selectable update rate.
module counter_core #(
    parameter integer DIV_LEVEL_0 = 400_000_000, // 0.25 Hz @ 100 MHz
    parameter integer DIV_LEVEL_1 = 200_000_000, // 0.50 Hz @ 100 MHz
    parameter integer DIV_LEVEL_2 = 100_000_000, // 1.00 Hz @ 100 MHz
    parameter integer DIV_LEVEL_3 =  50_000_000, // 2.00 Hz @ 100 MHz
    parameter integer DIV_LEVEL_4 =  25_000_000  // 4.00 Hz @ 100 MHz
) (
    input  wire        clk,
    input  wire        rst,
    input  wire        pause_toggle_event,
    input  wire        clear_event,
    input  wire        increment_event,
    input  wire        decrement_event,
    input  wire        led_direction_event,
    input  wire        frequency_event,

    output reg  [31:0] count,
    output reg         running,
    output reg         count_down,
    output reg         led_right,
    output reg  [2:0]  frequency_index,
    output reg  [31:0] divider_value,
    output wire [11:0] led_onehot,
    output reg  [3:0]  led_index,
    output reg  [3:0]  last_action,
    output reg  [15:0] event_counter,
    output reg         event_pulse
);
    localparam [3:0] ACTION_POWER_ON  = 4'h0;
    localparam [3:0] ACTION_RUN       = 4'h1;
    localparam [3:0] ACTION_PAUSE     = 4'h2;
    localparam [3:0] ACTION_CLEAR     = 4'h3;
    localparam [3:0] ACTION_INCREMENT = 4'h4;
    localparam [3:0] ACTION_DECREMENT = 4'h5;
    localparam [3:0] ACTION_LED_RIGHT = 4'h6;
    localparam [3:0] ACTION_LED_LEFT  = 4'h7;
    localparam [3:0] ACTION_FREQUENCY = 4'h8;

    reg [31:0] divider_counter;

    always @(*) begin
        case (frequency_index)
            3'd0: divider_value = DIV_LEVEL_0;
            3'd1: divider_value = DIV_LEVEL_1;
            3'd2: divider_value = DIV_LEVEL_2;
            3'd3: divider_value = DIV_LEVEL_3;
            default: divider_value = DIV_LEVEL_4;
        endcase
    end

    assign led_onehot = (12'b0000_0000_0001 << led_index);

    always @(posedge clk) begin
        if (rst) begin
            divider_counter <= 32'd0;
            count           <= 32'd0;
            running         <= 1'b1;
            count_down      <= 1'b0;
            led_right       <= 1'b1;
            frequency_index <= 3'd2;
            led_index       <= 4'd0;
            last_action     <= ACTION_POWER_ON;
            event_counter   <= 16'd0;
            event_pulse     <= 1'b0;
        end else begin
            event_pulse <= 1'b0;

            // Control-event priority makes simultaneous presses deterministic.
            if (clear_event) begin
                count           <= 32'd0;
                divider_counter <= 32'd0;
                last_action     <= ACTION_CLEAR;
                event_counter   <= event_counter + 1'b1;
                event_pulse     <= 1'b1;
            end else if (pause_toggle_event) begin
                running       <= ~running;
                last_action   <= running ? ACTION_PAUSE : ACTION_RUN;
                event_counter <= event_counter + 1'b1;
                event_pulse   <= 1'b1;
            end else if (increment_event) begin
                count_down    <= 1'b0;
                last_action   <= ACTION_INCREMENT;
                event_counter <= event_counter + 1'b1;
                event_pulse   <= 1'b1;
            end else if (decrement_event) begin
                count_down    <= 1'b1;
                last_action   <= ACTION_DECREMENT;
                event_counter <= event_counter + 1'b1;
                event_pulse   <= 1'b1;
            end else if (led_direction_event) begin
                led_right     <= ~led_right;
                last_action   <= led_right ? ACTION_LED_LEFT : ACTION_LED_RIGHT;
                event_counter <= event_counter + 1'b1;
                event_pulse   <= 1'b1;
            end else if (frequency_event) begin
                frequency_index <= (frequency_index == 3'd4)
                                   ? 3'd0 : frequency_index + 1'b1;
                divider_counter <= 32'd0;
                last_action     <= ACTION_FREQUENCY;
                event_counter   <= event_counter + 1'b1;
                event_pulse     <= 1'b1;
            end else if (divider_counter >= divider_value - 1'b1) begin
                divider_counter <= 32'd0;
                if (running) begin
                    if (count_down)
                        count <= count - 1'b1;
                    else
                        count <= count + 1'b1;

                    if (led_right) begin
                        if (led_index == 4'd11)
                            led_index <= 4'd0;
                        else
                            led_index <= led_index + 1'b1;
                    end else begin
                        if (led_index == 4'd0)
                            led_index <= 4'd11;
                        else
                            led_index <= led_index - 1'b1;
                    end
                end
            end else begin
                divider_counter <= divider_counter + 1'b1;
            end
        end
    end
endmodule
