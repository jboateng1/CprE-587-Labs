-------------------------------------------------------------------------
-- Matthew Dwyer
-- Department of Electrical and Computer Engineering
-- Iowa State University
-------------------------------------------------------------------------


-- staged_mac.vhd
-------------------------------------------------------------------------
-- DESCRIPTION: This file contains a basic staged axi-stream mac unit. It
-- multiplies two integer/Q values togeather and accumulates them.
--
-- NOTES:
-- 10/25/21 by MPD::Inital template creation
-------------------------------------------------------------------------

library work;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity staged_mac is
  generic(
      -- Parameters of mac
      C_DATA_WIDTH : integer := 8
      
    );
	port (
        ACLK	: in	std_logic;
		ARESETN	: in	std_logic;       

        -- AXIS slave data interface
		SD_AXIS_TREADY	: out	std_logic;
		SD_AXIS_TDATA	: in	std_logic_vector(C_DATA_WIDTH*2-1 downto 0);  -- Packed data input
		SD_AXIS_TLAST	: in	std_logic;
        SD_AXIS_TUSER   : in    std_logic;  -- Should we treat this first value in the stream as an inital accumulate value?
		SD_AXIS_TVALID	: in	std_logic;
        SD_AXIS_TID     : in    std_logic_vector(7 downto 0);

        -- AXIS master accumulate result out interface
		MO_AXIS_TVALID	: out	std_logic;
		MO_AXIS_TDATA	: out	std_logic_vector(C_DATA_WIDTH-1 downto 0);
		MO_AXIS_TLAST	: out	std_logic;
		MO_AXIS_TREADY	: in	std_logic;
		MO_AXIS_TID     : out   std_logic_vector(7 downto 0)
    );

attribute SIGIS : string; 
attribute SIGIS of ACLK : signal is "Clk"; 

end staged_mac;


architecture behavioral of staged_mac is
    -- Internal Signals
	--signal state : std_logic_vector(1 downto 0) := "00";  -- State machine state//Josh
    signal accumulate : signed(C_DATA_WIDTH-1 downto 0) := (others => '0');  -- Accumulator//Josh
    signal mac_debug : std_logic_vector(31 downto 0) := (others => '0');    -- Debug signal//Josh
	
	-- Mac state
    type STATE_TYPE is (WAIT_FOR_VALUES, MULTIPLY_ACCUMULATE);
    signal state : STATE_TYPE := WAIT_FOR_VALUES;
	
	-- Debug signals, make sure we aren't going crazy
    -- signal mac_debug : std_logic_vector(31 downto 0);//commented out

begin

    -- Interface signals
    SD_AXIS_TREADY <= '1';  -- Always ready to accept data//Josh


    -- Internal signals
	
	
	-- Debug Signals
    mac_debug <= x"00000000";  -- Double checking sanity
   
   process (ACLK) is
    variable data_input : signed(C_DATA_WIDTH-1 downto 0); --//Josh
    variable weight_input : signed(C_DATA_WIDTH-1 downto 0); --//Josh
    variable result : signed(C_DATA_WIDTH*2-1 downto 0); --//Josh
    
   begin 
    if rising_edge(ACLK) then  -- Rising Edge

      -- Reset values if reset is low
      if ARESETN = '0' then  -- Reset
        state       <= WAIT_FOR_VALUES;
        accumulate <= (others => '0');  -- Reset the accumulator. Please double-check this

      else
        case state is  -- State
            -- Wait here until we receive values
            when WAIT_FOR_VALUES =>
                -- Wait here until we recieve valid values
              if SD_AXIS_TVALID = '1' then
                data_input := signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto 0));
                weight_input := signed(SD_AXIS_TDATA(C_DATA_WIDTH*2-1 downto C_DATA_WIDTH));
                result := data_input * weight_input;
                state <= MULTIPLY_ACCUMULATE;
              end if;
			
			when MULTIPLY_ACCUMULATE =>
			  if SD_AXIS_TVALID = '1' then
                data_input := signed(SD_AXIS_TDATA(C_DATA_WIDTH-1 downto 0));
                weight_input := signed(SD_AXIS_TDATA(C_DATA_WIDTH*2-1 downto C_DATA_WIDTH));
                result := data_input * weight_input;
                accumulate <= accumulate + result;  -- Accumulate the result
              end if;
			
			-- Other stages go here	
			
            when others =>
                state <= WAIT_FOR_VALUES;
                -- Not really important, this case should never happen
                -- Needed for proper synthesis and also good programming practice       
        end case;  -- State
      end if;  -- Reset

    end if;  -- Rising Edge
   end process;
   
   -- Output the accumulated result when valid
  MO_AXIS_TDATA <= std_logic_vector(accumulate(C_DATA_WIDTH-1 downto 0));
  MO_AXIS_TLAST <= SD_AXIS_TLAST;
  MO_AXIS_TVALID <= '1' when SD_AXIS_TVALID = '1' and state = MULTIPLY_ACCUMULATE else '0';
  
end architecture behavioral;
