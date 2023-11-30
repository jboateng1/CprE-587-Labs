relaunch_sim

# Clock and reset
add_force {/piped_mac/ACLK} -radix hex {0 0ns} {1 5000ps} -repeat_every 10000ps
add_force {/piped_mac/ARESETN} -radix hex {0 0ns}
run 30ns
add_force {/piped_mac/ARESETN} -radix hex {1 0ns}
# No back pressure
add_force {/piped_mac/MO_AXIS_TREADY} -radix hex {1 0ns}



# 2. Basic Double Operation 1010101020202020
add_force {/piped_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/piped_mac/SD_AXIS_TLAST} -radix hex {0 0ns}
add_force {/piped_mac/SD_AXIS_TID} -radix hex {2 0ns}
add_force {/piped_mac/SD_AXIS_TDATA} -radix hex {1111111122222222 0ns}
add_force {/piped_mac/SD_AXIS_TUSER} -radix hex {0 0ns}
run 10ns
add_force {/piped_mac/SD_AXIS_TLAST} -radix hex {1 0ns}
add_force {/piped_mac/SD_AXIS_TDATA} -radix hex {1010101020202020 0ns}
run 10ns
add_force {/piped_mac/SD_AXIS_TLAST} -radix hex {0 0ns}
add_force {/piped_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
run 50ns
# Expected:
#   0x0608_0604 [Overflow]
