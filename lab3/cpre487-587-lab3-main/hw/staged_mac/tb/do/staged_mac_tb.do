force sim:/staged_mac/ACLK 1 0, 0 {5 ps} -r 10
run 20
force sim:/staged_mac/ARESETN 1 0 -cancel 20
force sim:/staged_mac/ARESETN 0 20
run 50

# No Back Pressure
force sim:/staged_mac/MO_AXIS_TREADY 1 0
run 20

# 1. Basic Single Operation
# Expected Multiplication Output: 0x0246 8ACF 0ECA 8642
# Expected Truncated Output: 0x8ACF 0ECA
force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#1111111122222222 0
force sim:/staged_mac/SD_AXIS_TID 16#1 0
force sim:/staged_mac/SD_AXIS_TLAST 1 0
force sim:/staged_mac/SD_AXIS_TUSER 0 0
run 20

force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 200


# 2. Basic Single Operation
force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000AAAA0000BBBB 0
force sim:/staged_mac/SD_AXIS_TID 16#2 0
force sim:/staged_mac/SD_AXIS_TLAST 1 0
force sim:/staged_mac/SD_AXIS_TUSER 0 0
run 20

force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 200


# 3. Inital Load Single Operation
force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000000000001111 0
force sim:/staged_mac/SD_AXIS_TID 16#2 0
force sim:/staged_mac/SD_AXIS_TLAST 0 0
force sim:/staged_mac/SD_AXIS_TUSER 1 0
run 20

force sim:/staged_mac/SD_AXIS_TUSER 0 0
force sim:/staged_mac/SD_AXIS_TLAST 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000AAAA0000BBBB 0
run 20

force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 200


# 4. Basic Multi Operation
force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000AAAA0000BBBB 0
force sim:/staged_mac/SD_AXIS_TID 16#3 0
force sim:/staged_mac/SD_AXIS_TLAST 0 0
force sim:/staged_mac/SD_AXIS_TUSER 0 0
run 20
force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 50

force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000AAAA0000BBBB 0
run 20
force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 50

force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#0000AAAA0000BBBB 0
force sim:/staged_mac/SD_AXIS_TLAST 1 0
run 20
force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 100


# Apply Back Pressure
force sim:/staged_mac/MO_AXIS_TREADY 0 0
run 20

# 5. Basic Single Operation
force sim:/staged_mac/SD_AXIS_TVALID 1 0
force sim:/staged_mac/SD_AXIS_TDATA 16#1111111122222222 0
force sim:/staged_mac/SD_AXIS_TID 16#1 0
force sim:/staged_mac/SD_AXIS_TLAST 1 0
force sim:/staged_mac/SD_AXIS_TUSER 0 0
run 20

force sim:/staged_mac/SD_AXIS_TVALID 0 0
run 200

force sim:/staged_mac/MO_AXIS_TREADY 1 0
run 50
