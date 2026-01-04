#include "Flag_State.h"

void Board_Order_info_Init(Board_Order_info_t* Order)
{
		Order[0].All_Led_Close 					= 0;
		Order[0].All_Led_Open 					= 0;
		Order[0].Board_Work 					= 0;
		Order[0].Energy_machine_state 					= 0;
	    Order[0].Set_Color                      = 0;
	    Order[0].start_permission               = 0;
	
		Order[1].All_Led_Close 					= 0;
		Order[1].All_Led_Open 					= 0;
		Order[1].Board_Work 					= 0;
		Order[1].Energy_machine_state 					= 0;
	    Order[1].Set_Color                      = 0;
  	    Order[1].start_permission               = 0;
	
		Order[2].All_Led_Close 					= 0;
		Order[2].All_Led_Open 					= 0;
		Order[2].Board_Work 					= 0;
		Order[2].Energy_machine_state 					= 0;
	    Order[2].Set_Color                      = 0;
	    Order[2].start_permission               = 0;
	
		Order[3].All_Led_Close 					= 0;
		Order[3].All_Led_Open 					= 0;
		Order[3].Board_Work 					= 0;
		Order[3].Energy_machine_state 					= 0;
		Order[3].Set_Color                      = 0;
		Order[3].start_permission               = 0;
	
		Order[4].All_Led_Close 					= 0;
		Order[4].All_Led_Open 					= 0;
		Order[4].Board_Work 					= 0;
		Order[4].Energy_machine_state 					= 0;
		Order[4].Set_Color                      = 0;
		Order[4].start_permission               = 0;
}

void Big_Fu_info_Init(Big_Fu_info_t* Big_Fu)
{
		Big_Fu[0].All_Hit_State  				= 0;
		Big_Fu[0].Color							= 0;
		Big_Fu[0].Hit_LED_state					= 0;
		Big_Fu[0].Last_Color					= 0;
		Big_Fu[0].Single_Hit_State			    = 0;
		Big_Fu[0].Single_Working_State	        = 0;
		Big_Fu[0].Spin_State					= 0;
	  Big_Fu[0].hit_time=0;
	
		Big_Fu[1].All_Hit_State  				= 0;
		Big_Fu[1].Color							= 0;
		Big_Fu[1].Hit_LED_state					= 0;
		Big_Fu[1].Last_Color					= 0;
		Big_Fu[1].Single_Hit_State			    = 0;
		Big_Fu[1].Single_Working_State	        = 0;
		Big_Fu[1].Spin_State					= 0;
	  Big_Fu[1].hit_time=0;
	
		Big_Fu[2].All_Hit_State  				= 0;
		Big_Fu[2].Color							= 0;
		Big_Fu[2].Hit_LED_state					= 0;
		Big_Fu[2].Last_Color					= 0;
		Big_Fu[2].Single_Hit_State			    = 0;
		Big_Fu[2].Single_Working_State	        = 0;
		Big_Fu[2].Spin_State					= 0;
		Big_Fu[2].hit_time=0;
		
		Big_Fu[3].All_Hit_State  				= 0;
		Big_Fu[3].Color							= 0;
		Big_Fu[3].Hit_LED_state					= 0;
		Big_Fu[3].Last_Color					= 0;
		Big_Fu[3].Single_Hit_State			    = 0;
		Big_Fu[3].Single_Working_State	        = 0;
		Big_Fu[3].Spin_State					= 0;
		Big_Fu[3].hit_time=0;
		
		Big_Fu[4].All_Hit_State  				= 0;
		Big_Fu[4].Color							= 0;
		Big_Fu[4].Hit_LED_state					= 0;
		Big_Fu[4].Last_Color					= 0;
		Big_Fu[4].Single_Hit_State			    = 0;
		Big_Fu[4].Single_Working_State	        = 0;
		Big_Fu[4].Spin_State					= 0;
		Big_Fu[4].hit_time=0;
}
