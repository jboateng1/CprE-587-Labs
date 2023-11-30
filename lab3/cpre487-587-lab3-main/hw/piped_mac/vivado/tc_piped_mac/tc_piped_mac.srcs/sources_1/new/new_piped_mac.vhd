-------------------------------------------------------------------------
-- Matthew Dwyer
-- Department of Electrical and Computer Engineering
-- Iowa State University
-------------------------------------------------------------------------


-- piped_mac.vhd
-------------------------------------------------------------------------
-- DESCRIPTION: This file contains a basic piplined axi-stream mac unit. It
-- multiplies two integer/Q values togeather and accumulates them.
--
-- NOTES:
-- 10/25/21 by MPD::Inital template creation
-------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity piped_mac is
    generic(
        C_DATA_WIDTH : integer := 16
    );
    port (
        ACLK    : in  std_logic;
        ARESETN : in  std_logic;
        
        -- AXIS slave data interface
        SD_AXIS_TREADY : out std_logic;
        SD_AXIS_TDATA  : in  std_logic_vector(C_DATA_WIDTH*8-1 downto 0); -- Packed data input
        SD_AXIS_TLAST  : in  std_logic;
        SD_AXIS_TUSER  : in  std_logic; 
        SD_AXIS_TVALID : in  std_logic;
        SD_AXIS_TID    : in  std_logic_vector(7 downto 0);

        -- AXIS master accumulate result out interface
        MO_AXIS_TVALID : out std_logic;
        MO_AXIS_TDATA  : out std_logic_vector(C_DATA_WIDTH-1 downto 0);
        MO_AXIS_TLAST  : out std_logic;
        MO_AXIS_TREADY : in  std_logic;
        MO_AXIS_TID    : out std_logic_vector(7 downto 0)
    );
end piped_mac;

architecture behavioral of piped_mac is
    -- Define the stages of the pipeline
    type PIPE_STAGES is (INPUT_STAGE, MULTIPLY_STAGE, ACCUMULATE_STAGE);
    signal current_stage : PIPE_STAGES := INPUT_STAGE;

    -- Internal signals for pipeline registers
    signal input_reg : std_logic_vector(C_DATA_WIDTH*2-1 downto 0);
    signal multiply_reg : signed(C_DATA_WIDTH-1 downto 0);
    signal accumulate_reg : signed(C_DATA_WIDTH-1 downto 0);

    -- Signals for multiplication operands
    signal activation : signed((C_DATA_WIDTH/2)-1 downto 0);
    signal weight : signed((C_DATA_WIDTH/2)-1 downto 0);
    signal product : signed(C_DATA_WIDTH-1 downto 0);

    -- Debug signals, make sure we aren't going crazy
    signal mac_debug : std_logic_vector(31 downto 0);

begin
    -- Interface signals
    SD_AXIS_TREADY <= '1'; -- Always ready to accept data

    -- Debug Signals
    mac_debug <= x"00000000"; -- Double checking sanity

    process (ACLK) is
    begin 
        if rising_edge(ACLK) then  -- Rising Edge
            -- Reset values if reset is low
            if ARESETN = '0' then  -- Reset
                current_stage <= INPUT_STAGE;
                input_reg <= (others => '0');
                multiply_reg <= (others => '0');
                accumulate_reg <= (others => '0');
            else
                -- Input Stage
                if current_stage = INPUT_STAGE and SD_AXIS_TVALID = '1' then
                    input_reg <= SD_AXIS_TDATA;
                    activation <= signed(SD_AXIS_TDATA((C_DATA_WIDTH/2)-1 downto 0));
                    weight <= signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto C_DATA_WIDTH/2));
                    if SD_AXIS_TUSER = '1' then
                        -- Treat the first value as an initial accumulate value
                        accumulate_reg <= signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto 0));
                    end if;
                    current_stage <= MULTIPLY_STAGE;
                end if;

                -- Multiply Stage
                if current_stage = MULTIPLY_STAGE then
                    product <= activation * weight;
                    multiply_reg <= product;
                    current_stage <= ACCUMULATE_STAGE;
                end if;

                -- Accumulate Stage
                if current_stage = ACCUMULATE_STAGE then
                    accumulate_reg <= accumulate_reg + product;
                    -- Move to the next stage or reset to INPUT_STAGE
                    current_stage <= INPUT_STAGE;
                end if;
            end if; -- Reset
        end if; -- Rising Edge
    end process;

    -- Set output data when accumulate stage is complete
    MO_AXIS_TVALID <= '1' when current_stage = INPUT_STAGE else '0';
    MO_AXIS_TDATA <= std_logic_vector(accumulate_reg);
    MO_AXIS_TLAST <= '1' when SD_AXIS_TLAST = '1' and current_stage = INPUT_STAGE else '0';
    MO_AXIS_TID <= SD_AXIS_TID;
end architecture behavioral;
