## GX-BIDT FPGA platform + XC7A200T-FBG484-2
## Top: gx_counter_uart_lcd_top

## 100 MHz active crystal on the core board.
set_property PACKAGE_PIN W19 [get_ports CLK_100M]
set_property IOSTANDARD LVCMOS33 [get_ports CLK_100M]
create_clock -name CLK_100M -period 10.000 [get_ports CLK_100M]

## Core-board buttons, active low. Bank 34 is powered at 1.5 V.
## Physical mapping:
##   FPGA_nRST/T6 = clear, KEY0 = pause/run, KEY1 = increment,
##   KEY2 = decrement, KEY3 = LED direction, KEY4 = frequency.
set_property PACKAGE_PIN Y6  [get_ports KEY0_N]
set_property PACKAGE_PIN AA6 [get_ports KEY1_N]
set_property PACKAGE_PIN V7  [get_ports KEY2_N]
set_property PACKAGE_PIN W7  [get_ports KEY3_N]
set_property PACKAGE_PIN AB7 [get_ports KEY4_N]
set_property PACKAGE_PIN T6  [get_ports KEY_CLEAR_N]
set_property IOSTANDARD LVCMOS15 [get_ports {KEY0_N KEY1_N KEY2_N KEY3_N KEY4_N KEY_CLEAR_N}]

## Core-board CH340E USB-UART: 115200 baud, 8 data bits, no parity, 1 stop bit.
## CH340E TX -> FPGA AA14 (RX); FPGA V14 (TX) -> CH340E RX.
set_property PACKAGE_PIN AA14 [get_ports UART_RXD]
set_property PACKAGE_PIN V14  [get_ports UART_TXD]
set_property IOSTANDARD LVCMOS33 [get_ports {UART_RXD UART_TXD}]

## LCD1602 on the baseboard C-group bus switches.
## Hardware selector requirement: B4 path, S1C=1, S0C=1, nOEC=0.
set_property PACKAGE_PIN E14 [get_ports {LCD_D[0]}]
set_property PACKAGE_PIN K19 [get_ports {LCD_D[1]}]
set_property PACKAGE_PIN K21 [get_ports {LCD_D[2]}]
set_property PACKAGE_PIN L21 [get_ports {LCD_D[3]}]
set_property PACKAGE_PIN F16 [get_ports {LCD_D[4]}]
set_property PACKAGE_PIN M22 [get_ports {LCD_D[5]}]
set_property PACKAGE_PIN M18 [get_ports {LCD_D[6]}]
set_property PACKAGE_PIN L18 [get_ports {LCD_D[7]}]
set_property PACKAGE_PIN N18 [get_ports LCD_RS]
set_property PACKAGE_PIN N19 [get_ports LCD_RW]
set_property PACKAGE_PIN N20 [get_ports LCD_E]
set_property IOSTANDARD LVCMOS33 [get_ports {LCD_D[*] LCD_RS LCD_RW LCD_E}]

## Existing XO2 Mode0 32-bit hexadecimal display bus.
set_property PACKAGE_PIN J19  [get_ports {PO[0]}]
set_property PACKAGE_PIN F20  [get_ports {PO[1]}]
set_property PACKAGE_PIN G18  [get_ports {PO[2]}]
set_property PACKAGE_PIN J20  [get_ports {PO[3]}]
set_property PACKAGE_PIN G17  [get_ports {PO[4]}]
set_property PACKAGE_PIN J21  [get_ports {PO[5]}]
set_property PACKAGE_PIN F19  [get_ports {PO[6]}]
set_property PACKAGE_PIN H19  [get_ports {PO[7]}]
set_property PACKAGE_PIN F21  [get_ports {PO[8]}]
set_property PACKAGE_PIN A19  [get_ports {PO[9]}]
set_property PACKAGE_PIN A18  [get_ports {PO[10]}]
set_property PACKAGE_PIN C19  [get_ports {PO[11]}]
set_property PACKAGE_PIN B20  [get_ports {PO[12]}]
set_property PACKAGE_PIN A21  [get_ports {PO[13]}]
set_property PACKAGE_PIN B21  [get_ports {PO[14]}]
set_property PACKAGE_PIN A20  [get_ports {PO[15]}]
set_property PACKAGE_PIN T3   [get_ports {PO[16]}]
set_property PACKAGE_PIN U6   [get_ports {PO[17]}]
set_property PACKAGE_PIN AB16 [get_ports {PO[18]}]
set_property PACKAGE_PIN V5   [get_ports {PO[19]}]
set_property PACKAGE_PIN R6   [get_ports {PO[20]}]
set_property PACKAGE_PIN Y13  [get_ports {PO[21]}]
set_property PACKAGE_PIN L19  [get_ports {PO[22]}]
set_property PACKAGE_PIN L20  [get_ports {PO[23]}]
set_property PACKAGE_PIN C22  [get_ports {PO[24]}]
set_property PACKAGE_PIN W11  [get_ports {PO[25]}]
set_property PACKAGE_PIN W12  [get_ports {PO[26]}]
set_property PACKAGE_PIN C14  [get_ports {PO[27]}]
set_property PACKAGE_PIN Y9   [get_ports {PO[28]}]
set_property PACKAGE_PIN L6   [get_ports {PO[29]}]
set_property PACKAGE_PIN G4   [get_ports {PO[30]}]
set_property PACKAGE_PIN Y11  [get_ports {PO[31]}]

## Bank 34 and Bank 35 are both powered from 1.5 V on this core board.
## PO[16], [17], [19], [20], [28] belong to Bank 34; PO[29], [30]
## belong to Bank 35.  Keeping every port in each bank at the same standard
## avoids BIVC-1 and matches the actual VCCO rails.
set_property IOSTANDARD LVCMOS15 [get_ports {PO[16] PO[17] PO[19] PO[20] PO[28] PO[29] PO[30]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PO[0] PO[1] PO[2] PO[3] PO[4] PO[5] PO[6] PO[7] PO[8] PO[9] PO[10] PO[11] PO[12] PO[13] PO[14] PO[15] PO[18] PO[21] PO[22] PO[23] PO[24] PO[25] PO[26] PO[27] PO[31]}]

## Existing XO2 Mode0 LED bus. PIO[11:0] drives LED1..LED12 one-hot.
set_property PACKAGE_PIN J16 [get_ports {PIO[0]}]
set_property PACKAGE_PIN E22 [get_ports {PIO[1]}]
set_property PACKAGE_PIN F18 [get_ports {PIO[2]}]
set_property PACKAGE_PIN E19 [get_ports {PIO[3]}]
set_property PACKAGE_PIN D21 [get_ports {PIO[4]}]
set_property PACKAGE_PIN D17 [get_ports {PIO[5]}]
set_property PACKAGE_PIN D19 [get_ports {PIO[6]}]
set_property PACKAGE_PIN C17 [get_ports {PIO[7]}]
set_property PACKAGE_PIN W5  [get_ports {PIO[8]}]
set_property PACKAGE_PIN E13 [get_ports {PIO[9]}]
set_property PACKAGE_PIN D14 [get_ports {PIO[10]}]
set_property PACKAGE_PIN B13 [get_ports {PIO[11]}]
set_property PACKAGE_PIN C13 [get_ports {PIO[12]}]
set_property PACKAGE_PIN E18 [get_ports {PIO[13]}]
set_property PACKAGE_PIN L14 [get_ports {PIO[14]}]
set_property PACKAGE_PIN L15 [get_ports {PIO[15]}]

## PIO[8]/W5 is in 1.5 V Bank 34; the remaining PIO pins are 3.3 V banks.
set_property IOSTANDARD LVCMOS15 [get_ports {PIO[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PIO[0] PIO[1] PIO[2] PIO[3] PIO[4] PIO[5] PIO[6] PIO[7] PIO[9] PIO[10] PIO[11] PIO[12] PIO[13] PIO[14] PIO[15]}]

set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property BITSTREAM.CONFIG.UNUSEDPIN PULLDOWN [current_design]
