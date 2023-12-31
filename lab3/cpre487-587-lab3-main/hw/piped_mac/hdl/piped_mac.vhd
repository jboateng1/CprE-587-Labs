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
    signal input_reg : std_logic_vector(C_DATA_WIDTH*8-1 downto 0);
    signal multiply_reg : signed(C_DATA_WIDTH-1 downto 0);
    signal accumulate_reg : signed(C_DATA_WIDTH-1 downto 0);
    signal multi_valid : std_logic;
    signal multi_last : std_logic;
    signal acc_valid : std_logic;
    signal acc_last : std_logic;
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
                --current_stage <= INPUT_STAGE;
                input_reg <= (others => '0');
                multiply_reg <= (others => '0');
                accumulate_reg <= (others => '0');
                multi_last <= '0';
                multi_valid <= '0';
                acc_last <= '0';      
                acc_valid <= '0';  
                weight <= (others => '0');
                activation <= (others => '0');
                
            else
                -- Input Stage
                if acc_valid = '1' then
                    accumulate_reg <= (accumulate_reg + multiply_reg);
                    acc_last <= multi_last;
                    
                end if;
                
                acc_valid <= multi_valid;
                
                if multi_valid = '1' then
                    --input_reg <= SD_AXIS_TDATA;
                    --activation <= signed(SD_AXIS_TDATA((C_DATA_WIDTH/2)-1 downto 0));
                    --weight <= signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto C_DATA_WIDTH/2));
                    multiply_reg <= activation * weight;
                    multi_last <= SD_AXIS_TLAST;
                    
                    
                    --current_stage <= MULTIPLY_STAGE;
                end if;
                multi_valid <= SD_AXIS_TVALID;
                activation <= signed(SD_AXIS_TDATA((C_DATA_WIDTH/2)-1 downto 0));
                weight <= signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto C_DATA_WIDTH/2));
            end if; -- Reset
        end if; -- Rising Edge
    end process;

    -- Set output data when accumulate stage is complete
    MO_AXIS_TLAST <= acc_last;
    MO_AXIS_TVALID <= acc_valid;
    --MO_AXIS_TVALID <= '1' when current_stage = INPUT_STAGE else '0';
    MO_AXIS_TDATA <= std_logic_vector(accumulate_reg);
    --MO_AXIS_TLAST <= '1' when SD_AXIS_TLAST = '1'  else '0';
    MO_AXIS_TID <= SD_AXIS_TID;
end architecture behavioral;
