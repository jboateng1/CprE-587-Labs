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
      C_DATA_WIDTH : integer := 32
      --C_ACCUM_WIDTH : integer := 16
      
    );
	port (
        ACLK	: in	std_logic;
		ARESETN	: in	std_logic;       

        -- AXIS slave data interface
		SD_AXIS_TREADY	: out	std_logic;
		SD_AXIS_TDATA	: in	std_logic_vector(C_DATA_WIDTH*4-1 downto 0);  -- Packed data input
		SD_AXIS_TLAST	: in	std_logic;
        SD_AXIS_TUSER   : in    std_logic;  -- Should we treat this first value in the stream as an inital accumulate value?
		SD_AXIS_TVALID	: in	std_logic;
        SD_AXIS_TID     : in    std_logic_vector(7 downto 0);

        -- AXIS master accumulate result out interface
		MO_AXIS_TVALID	: out	std_logic;
		MO_AXIS_TDATA	: out	std_logic_vector(15 downto 0);
		--MO_AXIS_TDATA	: out	std_logic_vector(C_DATA_WIDTH-1 downto 0);
		MO_AXIS_TLAST	: out	std_logic;
		MO_AXIS_TREADY	: in	std_logic;
		MO_AXIS_TID     : out   std_logic_vector(7 downto 0)
    );
    
    end entity staged_mac;

architecture behavioral of staged_mac is 
    type STATE_TYPE is (IDLE, LOAD, MULTIPLY, ACCUMULATE, OUTPUT); 
    signal state : STATE_TYPE := IDLE; 
    signal accumulator : signed(15 downto 0) := (others => '0'); 
    signal activation : signed(7 downto 0); 
    signal weight : signed(7 downto 0); 
    signal product : signed(15 downto 0); -- Product of two 8-bit numbers can be 16 bits 
    
begin 

    SD_AXIS_TREADY <= '1'; -- Always ready to accept data 
    process(ACLK) is 

    begin 
    
        if rising_edge(ACLK) then 
            if ARESETN = '0' then 
                state <= IDLE; 
                accumulator <= (others => '0'); 

            else 
                case state is 
                    when IDLE => 
                        if SD_AXIS_TVALID = '1' then 
                            -- Extract only the first 16 bits of SD_AXIS_TDATA 
                            activation <= signed(SD_AXIS_TDATA(7 downto 0)); 
                            weight <= signed(SD_AXIS_TDATA(15 downto 8)); 
                            state <= LOAD; 
                            if SD_AXIS_TUSER = '1' and SD_AXIS_TVALID = '1' then 
                                -- Treat the first value as an initial accumulate value, if valid 
                                accumulator <= signed(SD_AXIS_TDATA(15 downto 0)); 
                            end if; 
                        end if; 

                        
                    when LOAD => 
                        state <= MULTIPLY; 
                      
                    when MULTIPLY => 
                        product <= activation * weight; 
                        state <= ACCUMULATE; 
                     
                    when ACCUMULATE => 
                        accumulator <= accumulator + product; 
                        if SD_AXIS_TLAST = '1' then 
                            state <= OUTPUT; 
                        else 
                            state <= IDLE; 
                        end if; 
                     
                    when OUTPUT => 
                        if MO_AXIS_TREADY = '1' then 
                            MO_AXIS_TDATA <= std_logic_vector(accumulator); 
                            MO_AXIS_TVALID <= '1'; 
                            MO_AXIS_TLAST <= SD_AXIS_TLAST; 
                            MO_AXIS_TID <= SD_AXIS_TID; 
                            state <= IDLE; 

                        end if; 
                         
                    when others => 
                        state <= IDLE; 
                end case; 
            end if; 
        end if; 
    end process; 

end architecture behavioral; 
