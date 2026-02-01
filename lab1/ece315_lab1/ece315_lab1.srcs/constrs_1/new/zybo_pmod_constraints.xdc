# --- Keypad Connections (PMOD JA) ---

# --- Keypad Rows (outputs) ---
set_property PACKAGE_PIN G13 [get_ports {keypad_rows_tri_o[0]}]
set_property PACKAGE_PIN B11 [get_ports {keypad_rows_tri_o[1]}]
set_property PACKAGE_PIN T14 [get_ports {keypad_rows_tri_o[2]}]
set_property PACKAGE_PIN T15 [get_ports {keypad_rows_tri_o[3]}]

# --- Keypad Columns (inputs) ---
set_property PACKAGE_PIN E15 [get_ports {keypad_cols_tri_i[0]}]
set_property PACKAGE_PIN U12 [get_ports {keypad_cols_tri_i[1]}]
set_property PACKAGE_PIN G1  [get_ports {keypad_cols_tri_i[2]}]
set_property PACKAGE_PIN V7  [get_ports {keypad_cols_tri_i[3]}]

set_property IOSTANDARD LVCMOS33 [get_ports {keypad_rows_tri_o[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {keypad_cols_tri_i[*]}]

#---------------------------------------------------------------------

# --- Buttons (buttons_4bits from GPIO) ---
set_property PACKAGE_PIN R18 [get_ports {axi_gpio_0_tri_i[0]}]
set_property PACKAGE_PIN P16 [get_ports {axi_gpio_0_tri_i[1]}]
set_property PACKAGE_PIN V16 [get_ports {axi_gpio_0_tri_i[2]}]
set_property PACKAGE_PIN Y16 [get_ports {axi_gpio_0_tri_i[3]}]

# --- Switches (switches_4bits from GPIO2) ---
set_property PACKAGE_PIN G15 [get_ports {axi_gpio_0_tri2_i[0]}]
set_property PACKAGE_PIN P15 [get_ports {axi_gpio_0_tri2_i[1]}]
set_property PACKAGE_PIN W13 [get_ports {axi_gpio_0_tri2_i[2]}]
set_property PACKAGE_PIN T11 [get_ports {axi_gpio_0_tri2_i[3]}]

set_property IOSTANDARD LVCMOS33 [get_ports {axi_gpio_0_tri_i[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {axi_gpio_0_tri2_i[*]}]

#---------------------------------------------------------------------

# --- Green LEDs (from GPIO) ---
set_property PACKAGE_PIN M14 [get_ports {gpio_leds_tri_o[0]}]
set_property PACKAGE_PIN N14 [get_ports {gpio_leds_tri_o[1]}]
set_property PACKAGE_PIN R14 [get_ports {gpio_leds_tri_o[2]}]
set_property PACKAGE_PIN T15 [get_ports {gpio_leds_tri_o[3]}]

# --- RGB LED (from GPIO2) ---
# This uses the on-board RGB LED (only 1 RGB LED on Zybo)
set_property PACKAGE_PIN G14 [get_ports {gpio_leds_tri2_o[0]}]  # R
set_property PACKAGE_PIN D18 [get_ports {gpio_leds_tri2_o[1]}]  # G
set_property PACKAGE_PIN H17 [get_ports {gpio_leds_tri2_o[2]}]  # B

set_property IOSTANDARD LVCMOS33 [get_ports {gpio_leds_tri_o[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_leds_tri2_o[*]}]

#---------------------------------------------------------------------

# --- SSD1 (digit_1_tri_o) ---
set_property PACKAGE_PIN W7  [get_ports {digit_1_tri_o[0]}]  # a
set_property PACKAGE_PIN W6  [get_ports {digit_1_tri_o[1]}]  # b
set_property PACKAGE_PIN V6  [get_ports {digit_1_tri_o[2]}]  # c
set_property PACKAGE_PIN U6  [get_ports {digit_1_tri_o[3]}]  # d
set_property PACKAGE_PIN U7  [get_ports {digit_1_tri_o[4]}]  # e
set_property PACKAGE_PIN V8  [get_ports {digit_1_tri_o[5]}]  # f
set_property PACKAGE_PIN U8  [get_ports {digit_1_tri_o[6]}]  # g

# --- SSD2 (digit_2_tri_o) ---
set_property PACKAGE_PIN V9  [get_ports {digit_2_tri_o[0]}]
set_property PACKAGE_PIN U9  [get_ports {digit_2_tri_o[1]}]
set_property PACKAGE_PIN T8  [get_ports {digit_2_tri_o[2]}]
set_property PACKAGE_PIN V4  [get_ports {digit_2_tri_o[3]}]
set_property PACKAGE_PIN U4  [get_ports {digit_2_tri_o[4]}]
set_property PACKAGE_PIN V1  [get_ports {digit_2_tri_o[5]}]
set_property PACKAGE_PIN U1  [get_ports {digit_2_tri_o[6]}]

set_property IOSTANDARD LVCMOS33 [get_ports {digit_1_tri_o[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports {digit_2_tri_o[*]}]
