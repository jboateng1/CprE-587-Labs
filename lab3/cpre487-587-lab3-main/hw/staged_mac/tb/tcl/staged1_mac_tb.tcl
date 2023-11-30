relaunch_sim

# Clock and reset
add_force {/staged_mac/ACLK} -radix hex {0 0ns} {1 5000ps} -repeat_every 10000ps
add_force {/staged_mac/ARESETN} -radix hex {0 0ns}
run 30ns
add_force {/staged_mac/ARESETN} -radix hex {1 0ns}
# No back pressure
add_force {/staged_mac/MO_AXIS_TREADY} -radix hex {1 0ns}


# 1. Basic Single Operation
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TLAST} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TID} -radix hex {1 0ns}
add_force {/staged_mac/SD_AXIS_TDATA} -radix hex {1111111122222222 0ns}
add_force {/staged_mac/SD_AXIS_TUSER} -radix hex {0 0ns}
run 10ns
add_force {/staged_mac/SD_AXIS_TVALID} -radix hex {0 0ns}
run 50ns
# Expected:
#   8ACF_0ECA

