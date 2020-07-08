#include "grid_module_bu16_revb.h"

volatile uint8_t grid_module_bu16_revb_hardware_transfer_complete = 0;
volatile uint8_t grid_module_bu16_revb_mux = 0;
//volatile uint8_t grid_module_bu16_revb_mux_lookup[16] = {0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15};
	
volatile uint8_t grid_module_bu16_revb_mux_lookup[16] =       {12, 13, 8, 9, 4, 5, 0, 1, 14, 15, 10, 11, 6, 7, 2, 3};


void grid_module_bu16_revb_hardware_start_transfer(void){
	
	adc_async_start_conversion(&ADC_0);
	adc_async_start_conversion(&ADC_1);

}

static void grid_module_bu16_revb_hardware_transfer_complete_cb(void){
		
	if (grid_module_bu16_revb_hardware_transfer_complete == 0){
		grid_module_bu16_revb_hardware_transfer_complete++;
		return;
	}
	
		
	struct grid_ui_model* mod = &grid_ui_state;
	

	/* Read conversion results */
	
	uint16_t adcresult_0 = 0;
	uint16_t adcresult_1 = 0;
	
	uint8_t adc_index_0 = grid_module_bu16_revb_mux_lookup[grid_module_bu16_revb_mux+8];
	uint8_t adc_index_1 = grid_module_bu16_revb_mux_lookup[grid_module_bu16_revb_mux+0];
	
	/* Update the multiplexer */
	
	grid_module_bu16_revb_mux++;
	grid_module_bu16_revb_mux%=8;
	
	gpio_set_pin_level(MUX_A, grid_module_bu16_revb_mux/1%2);
	gpio_set_pin_level(MUX_B, grid_module_bu16_revb_mux/2%2);
	gpio_set_pin_level(MUX_C, grid_module_bu16_revb_mux/4%2);
	
	
	
	adc_async_read_channel(&ADC_0, 0, &adcresult_0, 2);
	adc_async_read_channel(&ADC_1, 0, &adcresult_1, 2);
	
	uint8_t adcresult_0_valid = 0;
	
	if (adcresult_0>60000){
		adcresult_0 = 0;
		adcresult_0_valid = 1;
	}
	else if (adcresult_0<200){
		adcresult_0 = 127;
		adcresult_0_valid = 1;
	}
		
	uint8_t adcresult_1_valid = 0;
	
	if (adcresult_1>60000){
		adcresult_1 = 0;
		adcresult_1_valid = 1;
	}
	else if (adcresult_1<200){
		adcresult_1 = 127;
		adcresult_1_valid = 1;
	}
	
	//CRITICAL_SECTION_ENTER()

	if (adcresult_0 != mod->report_ui_array[adc_index_0].helper[0] && adcresult_0_valid){
		
		uint8_t command;
		uint8_t note;
		uint8_t velocity;
		
		if (mod->report_ui_array[adc_index_0].helper[0] == 0){
			
			command = GRID_PARAMETER_MIDI_NOTEON;
			note = adc_index_0;
			velocity = 127;
		}
		else{
			
			command = GRID_PARAMETER_MIDI_NOTEOFF;
			note = adc_index_0;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
				
		uint8_t* message = mod->report_ui_array[adc_index_0].payload;
		uint8_t error = 0;
		
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_offset, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_length, 0, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_offset , GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_length , command, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_PARAM1_offset  , GRID_CLASS_MIDIRELATIVE_PARAM1_length  , note, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_PARAM2_offset  , GRID_CLASS_MIDIRELATIVE_PARAM2_length  , velocity, &error);
							

		mod->report_ui_array[adc_index_0].helper[0] = velocity;
		
		message = mod->report_ui_array[adc_index_0 + 16].payload;
		grid_msg_set_parameter(message, GRID_CLASS_LEDPHASE_PHASE_offset  , GRID_CLASS_LEDPHASE_PHASE_length  , actuator, &error);
		grid_report_ui_set_changed_flag(mod, adc_index_0 + 16);
		
		
		
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	//CRITICAL_SECTION_ENTER()

	if (adcresult_1 != mod->report_ui_array[adc_index_1].helper[0] && adcresult_1_valid){
		
		uint8_t command;
		uint8_t note;
		uint8_t velocity;
		
		if (mod->report_ui_array[adc_index_1].helper[0] == 0){
			
			command = GRID_PARAMETER_MIDI_NOTEON;
			note = adc_index_1;
			velocity = 127;
		}
		else{
			
			command = GRID_PARAMETER_MIDI_NOTEOFF;
			note = adc_index_1;
			velocity = 0;
		}
		
		uint8_t actuator = 2*velocity;
				
		uint8_t* message = mod->report_ui_array[adc_index_1].payload;
		uint8_t error = 0;
				
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_offset, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_length, 0, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_offset , GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_length , command, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_PARAM1_offset  , GRID_CLASS_MIDIRELATIVE_PARAM1_length  , note, &error);
		grid_msg_set_parameter(message, GRID_CLASS_MIDIRELATIVE_PARAM2_offset  , GRID_CLASS_MIDIRELATIVE_PARAM2_length  , velocity, &error);
		
			
		mod->report_ui_array[adc_index_1].helper[0] = velocity;
		
		grid_report_ui_set_changed_flag(mod, adc_index_1);
		
			
		message = mod->report_ui_array[adc_index_1 + 16].payload;
		grid_msg_set_parameter(message, GRID_CLASS_LEDPHASE_PHASE_offset  , GRID_CLASS_LEDPHASE_PHASE_length  , actuator, &error);
		grid_report_ui_set_changed_flag(mod, adc_index_1 + 16);
		
	}
	
	//CRITICAL_SECTION_LEAVE()
	
	
	grid_module_bu16_revb_hardware_transfer_complete = 0;
	grid_module_bu16_revb_hardware_start_transfer();
}

void grid_module_bu16_revb_hardware_init(void){
	

	
	adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	adc_async_register_callback(&ADC_1, 0, ADC_ASYNC_CONVERT_CB, grid_module_bu16_revb_hardware_transfer_complete_cb);
	
	adc_async_enable_channel(&ADC_0, 0);
	adc_async_enable_channel(&ADC_1, 0);

}



void grid_module_bu16_revb_init(struct grid_ui_model* mod){

	grid_led_init(&grid_led_state, 16);
	grid_ui_model_init(mod, 32);
		
	for(uint8_t i=0; i<32; i++){
				
		uint8_t payload_template[30] = {0};
		enum grid_report_type_t type;
		
		uint8_t grid_module_bu16_revb_mux_lookup_led[16] =   {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
		
		if (i<16){ //BUTTON
			
			type = GRID_REPORT_TYPE_BROADCAST;
		
			sprintf(payload_template, GRID_CLASS_MIDIRELATIVE_frame);
				
			uint8_t error = 0;
			
			grid_msg_set_parameter(payload_template, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REP_code, &error);
					
			grid_msg_set_parameter(payload_template, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_offset, GRID_CLASS_MIDIRELATIVE_CABLECHANNEL_length, 0, &error);
			grid_msg_set_parameter(payload_template, GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_offset , GRID_CLASS_MIDIRELATIVE_CHANNELCOMMAND_length , 0, &error);
			grid_msg_set_parameter(payload_template, GRID_CLASS_MIDIRELATIVE_PARAM1_offset  , GRID_CLASS_MIDIRELATIVE_PARAM1_length  , 0, &error);
			grid_msg_set_parameter(payload_template, GRID_CLASS_MIDIRELATIVE_PARAM2_offset  , GRID_CLASS_MIDIRELATIVE_PARAM2_length  , 0, &error);
												
			
		}
		else{ // LED
	
			type = GRID_REPORT_TYPE_LOCAL;
			
			sprintf(payload_template, GRID_CLASS_LEDPHASE_frame);
			
			uint8_t error = 0;
			
			grid_msg_set_parameter(payload_template, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REP_code, &error);
								
			grid_msg_set_parameter(payload_template, GRID_CLASS_LEDPHASE_LAYERNUMBER_offset, GRID_CLASS_LEDPHASE_LAYERNUMBER_length, GRID_LED_LAYER_UI_A, &error);
			grid_msg_set_parameter(payload_template, GRID_CLASS_LEDPHASE_LEDNUMBER_offset , GRID_CLASS_LEDPHASE_LEDNUMBER_length , grid_module_bu16_revb_mux_lookup_led[i-16], &error);
			grid_msg_set_parameter(payload_template, GRID_CLASS_LEDPHASE_PHASE_offset  , GRID_CLASS_LEDPHASE_PHASE_length  , 0, &error);		
			
		}
		
		uint8_t payload_length = strlen(payload_template);

		uint8_t helper_template[2];
		
		helper_template[0] = 0;
		helper_template[1] = 0;
		
		uint8_t helper_length = 2;
		
		uint8_t error = grid_report_ui_init(mod, i, type, payload_template, payload_length, helper_template, helper_length);
		

	}
	
	grid_report_sys_init(mod);
			
	grid_module_bu16_revb_hardware_init();
	grid_module_bu16_revb_hardware_start_transfer();

};
