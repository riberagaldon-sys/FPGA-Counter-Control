`timescale 1ns / 1ps

// Packet format (ASCII, CRLF terminated):
// S,CCCCCCCC,R,D,L,I,DDDDDDDD,LLL,AA,O,EEEE\r\n
// C=count hex, R=running, D=count direction, L=LED direction,
// I=frequency index, second D field=divider hex, LLL=LED mask,
// AA=action code, O=Qt heartbeat online, EEEE=event counter.
module status_sender (
    input  wire        clk,
    input  wire        rst,
    input  wire        send,
    input  wire [31:0] count,
    input  wire        running,
    input  wire        count_down,
    input  wire        led_right,
    input  wire [2:0]  frequency_index,
    input  wire [31:0] divider_value,
    input  wire [11:0] led_onehot,
    input  wire [7:0]  action_code,
    input  wire        link_online,
    input  wire [15:0] event_counter,
    output reg         busy,
    output wire [7:0]  tx_data,
    output wire        tx_valid,
    input  wire        tx_ready
);
    localparam [5:0] LAST_INDEX = 6'd42;

    reg [5:0]  index;
    reg [31:0] s_count;
    reg        s_running;
    reg        s_count_down;
    reg        s_led_right;
    reg [2:0]  s_frequency_index;
    reg [31:0] s_divider_value;
    reg [11:0] s_led_onehot;
    reg [7:0]  s_action_code;
    reg        s_link_online;
    reg [15:0] s_event_counter;

    function [7:0] hex_ascii;
        input [3:0] nibble;
        begin
            if (nibble < 10)
                hex_ascii = 8'h30 + nibble;
            else
                hex_ascii = 8'h41 + (nibble - 10);
        end
    endfunction

    function [7:0] character_at;
        input [5:0] position;
        begin
            case (position)
                6'd0:  character_at = "S";
                6'd1:  character_at = ",";
                6'd2:  character_at = hex_ascii(s_count[31:28]);
                6'd3:  character_at = hex_ascii(s_count[27:24]);
                6'd4:  character_at = hex_ascii(s_count[23:20]);
                6'd5:  character_at = hex_ascii(s_count[19:16]);
                6'd6:  character_at = hex_ascii(s_count[15:12]);
                6'd7:  character_at = hex_ascii(s_count[11:8]);
                6'd8:  character_at = hex_ascii(s_count[7:4]);
                6'd9:  character_at = hex_ascii(s_count[3:0]);
                6'd10: character_at = ",";
                6'd11: character_at = s_running ? "1" : "0";
                6'd12: character_at = ",";
                6'd13: character_at = s_count_down ? "D" : "U";
                6'd14: character_at = ",";
                6'd15: character_at = s_led_right ? "R" : "L";
                6'd16: character_at = ",";
                6'd17: character_at = 8'h30 + s_frequency_index;
                6'd18: character_at = ",";
                6'd19: character_at = hex_ascii(s_divider_value[31:28]);
                6'd20: character_at = hex_ascii(s_divider_value[27:24]);
                6'd21: character_at = hex_ascii(s_divider_value[23:20]);
                6'd22: character_at = hex_ascii(s_divider_value[19:16]);
                6'd23: character_at = hex_ascii(s_divider_value[15:12]);
                6'd24: character_at = hex_ascii(s_divider_value[11:8]);
                6'd25: character_at = hex_ascii(s_divider_value[7:4]);
                6'd26: character_at = hex_ascii(s_divider_value[3:0]);
                6'd27: character_at = ",";
                6'd28: character_at = hex_ascii(s_led_onehot[11:8]);
                6'd29: character_at = hex_ascii(s_led_onehot[7:4]);
                6'd30: character_at = hex_ascii(s_led_onehot[3:0]);
                6'd31: character_at = ",";
                6'd32: character_at = hex_ascii(s_action_code[7:4]);
                6'd33: character_at = hex_ascii(s_action_code[3:0]);
                6'd34: character_at = ",";
                6'd35: character_at = s_link_online ? "1" : "0";
                6'd36: character_at = ",";
                6'd37: character_at = hex_ascii(s_event_counter[15:12]);
                6'd38: character_at = hex_ascii(s_event_counter[11:8]);
                6'd39: character_at = hex_ascii(s_event_counter[7:4]);
                6'd40: character_at = hex_ascii(s_event_counter[3:0]);
                6'd41: character_at = 8'h0D;
                default: character_at = 8'h0A;
            endcase
        end
    endfunction

    assign tx_valid = busy;
    assign tx_data  = character_at(index);

    always @(posedge clk) begin
        if (rst) begin
            busy              <= 1'b0;
            index             <= 6'd0;
            s_count           <= 32'd0;
            s_running         <= 1'b0;
            s_count_down      <= 1'b0;
            s_led_right       <= 1'b0;
            s_frequency_index <= 3'd0;
            s_divider_value   <= 32'd0;
            s_led_onehot      <= 12'd0;
            s_action_code     <= 8'd0;
            s_link_online     <= 1'b0;
            s_event_counter   <= 16'd0;
        end else if (!busy) begin
            if (send) begin
                s_count           <= count;
                s_running         <= running;
                s_count_down      <= count_down;
                s_led_right       <= led_right;
                s_frequency_index <= frequency_index;
                s_divider_value   <= divider_value;
                s_led_onehot      <= led_onehot;
                s_action_code     <= action_code;
                s_link_online     <= link_online;
                s_event_counter   <= event_counter;
                index             <= 6'd0;
                busy              <= 1'b1;
            end
        end else if (tx_ready) begin
            if (index == LAST_INDEX) begin
                index <= 6'd0;
                busy  <= 1'b0;
            end else begin
                index <= index + 1'b1;
            end
        end
    end
endmodule
