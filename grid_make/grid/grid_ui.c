#include "grid_ui.h"



void grid_port_process_ui(struct grid_ui_model* ui, struct grid_port* por){
	
	// Priorities: Always process local, try to process direct, broadcast messages are last. 
	
	
	uint8_t ui_available = 0;
	uint8_t core_available = 0;
	uint8_t message_local_action_available = 0;

	// UI STATE
		
	for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
		
		for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){
			
			if (grid_ui_event_istriggered(&grid_ui_state.element_list[j].event_list[k])){

				ui_available++;

				
			}
			
			if (grid_ui_event_istriggered_local(&grid_ui_state.element_list[j].event_list[k])){
				
				message_local_action_available++;

				
			}
			
		}
		
	}		
	
	// CORE SYSTEM
	for (uint8_t i=0; i<grid_core_state.element_list_length; i++){
		
		for (uint8_t j=0; j<grid_core_state.element_list[i].event_list_length; j++){
			
			if (grid_ui_event_istriggered(&grid_core_state.element_list[i].event_list[j])){
				
				core_available++;
			}
			
		}
		
	}	
	
	
	
	//NEW PING
	struct grid_port* port[4] = {&GRID_PORT_N, &GRID_PORT_E, &GRID_PORT_S, &GRID_PORT_W};
	
	for (uint8_t k = 0; k<4; k++){
		
		if (port[k]->ping_flag == 1){
		
			if (grid_buffer_write_init(&port[k]->tx_buffer, port[k]->ping_packet_length)){
				//Success
				for(uint32_t i = 0; i<port[k]->ping_packet_length; i++){
					grid_buffer_write_character(&port[k]->tx_buffer, port[k]->ping_packet[i]);
				}
				grid_buffer_write_acknowledge(&port[k]->tx_buffer);
			}
			port[k]->ping_flag = 0;
		}		
			
	}			

	grid_d51_task_next(ui->task);
		

	
	//LOCAL MESSAGES
	if (message_local_action_available){
		
	
		struct grid_msg message;
		grid_msg_init(&message);
		grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);
			
			
		// Prepare packet header
		uint8_t payload[GRID_PARAMETER_PACKET_maxlength] = {0};				
		uint32_t offset=0;
		
		
		
		// UI STATE

		for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
			
			for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){
			
				if (offset>GRID_PARAMETER_PACKET_marign){
					continue;
				}		
				else{
					
					CRITICAL_SECTION_ENTER()
					if (grid_ui_event_istriggered_local(&grid_ui_state.element_list[j].event_list[k])){
						
						offset += grid_ui_event_render_action(&grid_ui_state.element_list[j].event_list[k], &payload[offset]);
						grid_ui_event_reset(&grid_ui_state.element_list[j].event_list[k]);
					
					}
					CRITICAL_SECTION_LEAVE()
					
				}
			
			}
			
			

			
		}
		
		
		
		grid_msg_body_append_text(&message, payload, offset);
		grid_msg_packet_close(&message);
			
		uint32_t message_length = grid_msg_packet_get_length(&message);
			
		// Put the packet into the UI_TX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.tx_buffer, message_length)){
				
			for(uint32_t i = 0; i<message_length; i++){
					
				grid_buffer_write_character(&GRID_PORT_U.tx_buffer, grid_msg_packet_send_char(&message, i));
			}
				
			grid_buffer_write_acknowledge(&GRID_PORT_U.tx_buffer);
			
// 			uint8_t debug_string[200] = {0};
// 			sprintf(debug_string, "Space: RX: %d/%d  TX: %d/%d", grid_buffer_get_space(&GRID_PORT_U.rx_buffer), GRID_BUFFER_SIZE, grid_buffer_get_space(&GRID_PORT_U.tx_buffer), GRID_BUFFER_SIZE);
// 			grid_debug_print_text(debug_string);


		}
		else{
			// LOG UNABLE TO WRITE EVENT
		}
			
	}
	
	
	
	
	// Bandwidth Limiter for Broadcast messages
	
	if (por->cooldown > 0){
		por->cooldown--;
	}
	
	
	if (por->cooldown > 10){
		// dummy calls to make sure subtask after return are counted properly
		grid_d51_task_next(ui->task);		
		grid_d51_task_next(ui->task);		
		grid_d51_task_next(ui->task);	
		return;
	}

	
	
	grid_d51_task_next(ui->task);	

	struct grid_msg message;
	grid_msg_init(&message);
	grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);
	
	// BROADCAST MESSAGES : CORE SYSTEM	
	if (core_available){
			
		for (uint8_t i=0; i<grid_core_state.element_list_length; i++){
			
			for (uint8_t j=0; j<grid_core_state.element_list[i].event_list_length; j++){
				
				if (grid_msg_packet_get_length(&message)>GRID_PARAMETER_PACKET_marign){
					continue;
				}
				else{
					
					CRITICAL_SECTION_ENTER()
					if (grid_ui_event_istriggered(&grid_core_state.element_list[i].event_list[j])){
						
						uint32_t offset = grid_msg_body_get_length(&message); 
						message.body_length += grid_ui_event_render_action(&grid_core_state.element_list[i].event_list[j], &message.body[offset]);
						grid_ui_event_reset(&grid_core_state.element_list[i].event_list[j]);
						
					}
					CRITICAL_SECTION_LEAVE()
									
					
				}						
				

				
			}
			
		}

	}
	
	grid_d51_task_next(ui->task);
	// BROADCAST MESSAGES : UI STATE
	if (ui_available){
		
		for (uint8_t j=0; j<grid_ui_state.element_list_length; j++){
		
			for (uint8_t k=0; k<grid_ui_state.element_list[j].event_list_length; k++){ //j=1 because init is local
			
				if (grid_msg_packet_get_length(&message)>GRID_PARAMETER_PACKET_marign){
					continue;
				}		
				else{
								
					if (grid_ui_event_istriggered(&grid_ui_state.element_list[j].event_list[k])){

						uint32_t offset = grid_msg_body_get_length(&message); 

						message.body_length += grid_ui_event_render_event(&grid_ui_state.element_list[j].event_list[k], &message.body[offset]);
					
						offset = grid_msg_body_get_length(&message); 

						CRITICAL_SECTION_ENTER()
						message.body_length += grid_ui_event_render_action(&grid_ui_state.element_list[j].event_list[k], &message.body[offset]);
						grid_ui_event_reset(&grid_ui_state.element_list[j].event_list[k]);
						CRITICAL_SECTION_LEAVE()
						
					}
					
				}
			
			}
		
		}
		
	}

	grid_d51_task_next(ui->task);	
	if (core_available + ui_available){

		
		//por->cooldown += (2+por->cooldown/2);
		por->cooldown += 10;
		//por->cooldown = 3;
		
		

		grid_msg_packet_close(&message);
		uint32_t length = grid_msg_packet_get_length(&message);
		

		// Put the packet into the UI_RX buffer
		if (grid_buffer_write_init(&GRID_PORT_U.rx_buffer, length)){
	
			for(uint16_t i = 0; i<length; i++){
				
				grid_buffer_write_character(&GRID_PORT_U.rx_buffer, grid_msg_packet_send_char(&message, i));
			}
			
			grid_buffer_write_acknowledge(&GRID_PORT_U.rx_buffer);

			
		}
		else{
			// LOG UNABLE TO WRITE EVENT
		}
		
		
		
	
	}
	
}


void grid_ui_model_init(struct grid_ui_model* mod, uint8_t element_list_length){
	
	mod->status = GRID_UI_STATUS_INITIALIZED;

	mod->page_activepage = 0;

	mod->element_list_length = element_list_length;	

	printf("UI MODEL INIT: %d\r\n", element_list_length);
	mod->element_list = malloc(element_list_length*sizeof(struct grid_ui_element));
;
	
}

struct grid_ui_template_buffer* grid_ui_template_buffer_create(struct grid_ui_element* ele){
	

	
	struct grid_ui_template_buffer* this = NULL;
	struct grid_ui_template_buffer* prev = ele->template_buffer_list_head;

	this = malloc(sizeof(struct grid_ui_template_buffer));

	if (this == NULL){
		printf("error.ui.MallocFailed\r\n");
	}
	else{

		this->status = 0;
		this->next = NULL;
		this->page_number = 0;
		this->parent = ele;

		
		this->template_parameter_list = malloc(ele->template_parameter_list_length*sizeof(int32_t));

		if (this->template_parameter_list == NULL){
			printf("error.ui.MallocFailed\r\n");
		}
		else{

			if (ele->template_initializer!=NULL){
				ele->template_initializer(this);
			}

			printf("LIST\r\n");

			if (prev != NULL){

				this->page_number++;

				while(prev->next != NULL){

					this->page_number++;
					prev = prev->next;

				}

				prev->next = this;
				return this;

			}
			else{
				prev = this;
				return this;
				//this is the first item in the list
			}


			

		}
	}
	
	// FAILED
	return NULL;
}


void grid_ui_element_init(struct grid_ui_model* parent, uint8_t index, enum grid_ui_element_t element_type){
	

	struct grid_ui_element* ele = &parent->element_list[index];

	parent->element_list[index].event_clear_cb = NULL;
	parent->element_list[index].page_change_cb = NULL;


	ele->parent = parent;
	ele->index = index;

	ele->template_parameter_list_length = 0;
	ele->template_parameter_list = NULL;

	ele->template_buffer_list_head = NULL;

	ele->status = GRID_UI_STATUS_INITIALIZED;
	
	ele->type = element_type;

	
	if (element_type == GRID_UI_ELEMENT_SYSTEM){
		
		ele->event_list_length = 6;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_HEARTBEAT); // Heartbeat
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_MAPMODE_PRESS); // Mapmode press
		grid_ui_event_init(ele, 3, GRID_UI_EVENT_MAPMODE_RELEASE); // Mapmode release
		grid_ui_event_init(ele, 4, GRID_UI_EVENT_CFG_RESPONSE); //
		grid_ui_event_init(ele, 5, GRID_UI_EVENT_CFG_REQUEST); //

		ele->template_initializer = NULL;
		ele->template_parameter_list_length = 0;
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_AC); // Absolute Value Change (7bit)

		ele->template_initializer = &grid_element_potmeter_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_P_LIST_length;
		
		ele->event_clear_cb = &grid_element_potmeter_event_clear_cb;
		ele->page_change_cb = &grid_element_potmeter_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_BUTTON){
		
		ele->event_list_length = 2;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_BC);	// Button Change

		ele->template_initializer = &grid_element_button_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_B_LIST_length;

		ele->event_clear_cb = &grid_element_button_event_clear_cb;
		ele->page_change_cb = &grid_element_button_page_change_cb;

	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		ele->event_list_length = 3;
		
		ele->event_list = malloc(ele->event_list_length*sizeof(struct grid_ui_event));
		
		grid_ui_event_init(ele, 0, GRID_UI_EVENT_INIT); // Element Initialization Event
		grid_ui_event_init(ele, 1, GRID_UI_EVENT_EC);	// Encoder Change
		grid_ui_event_init(ele, 2, GRID_UI_EVENT_BC);	// Button Change

		ele->template_initializer = &grid_element_encoder_template_parameter_init;
		ele->template_parameter_list_length = GRID_LUA_FNC_E_LIST_length;
		
		ele->event_clear_cb = &grid_element_encoder_event_clear_cb;
		ele->page_change_cb = &grid_element_encoder_page_change_cb;

	}
	else{
		//UNKNOWN ELEMENT TYPE
		printf("error.unknown_element_type\r\n");
		ele->template_initializer = NULL;
	}

	if (ele->template_initializer != NULL){

		struct grid_ui_template_buffer* buf = grid_ui_template_buffer_create(ele);

		if (buf != NULL){

			ele->template_parameter_list = buf->template_parameter_list;
		}
		else{

		}

	}
	
}

void grid_ui_event_init(struct grid_ui_element* parent, uint8_t index, enum grid_ui_event_t event_type){
	


	printf("EV INIT\r\n");

	struct grid_ui_event* eve = &parent->event_list[index];
	eve->parent = parent;
	eve->index = index;

	eve->cfg_changed_flag = 0;
	
	eve->status = GRID_UI_STATUS_INITIALIZED;
	
	eve->type   = event_type;	
	eve->status = GRID_UI_STATUS_READY;


	// Initializing Event String	
	for (uint32_t i=0; i<GRID_UI_EVENT_STRING_maxlength; i++){
		eve->event_string[i] = 0;
	}
	
	eve->event_string_length = 0;
	

	// Initializing Action String
	for (uint32_t i=0; i<GRID_UI_ACTION_STRING_maxlength; i++){
		eve->action_string[i] = 0;
	}		
	
	// Initializing Action String
	for (uint32_t i=0; i<GRID_UI_ACTION_CALL_maxlength; i++){
		eve->action_call[i] = 0;
	}	
	
	eve->action_string_length = 0;
	

	uint8_t eventstring[GRID_UI_EVENT_STRING_maxlength] = {0};
	grid_ui_event_generate_eventstring(eve->parent->type, event_type, eventstring);	
	grid_ui_event_register_eventstring(eve->parent, event_type, eventstring);

	uint8_t actionstring[GRID_UI_ACTION_STRING_maxlength] = {0};
	grid_ui_event_generate_actionstring(eve->parent->type, event_type, actionstring);	
	grid_ui_event_register_actionstring(eve->parent, event_type, actionstring);

	uint8_t callstring[GRID_UI_ACTION_CALL_maxlength] = {0};
	grid_ui_event_generate_callstring(eve->parent->type, event_type, callstring, eve->parent->index);	
	grid_ui_event_register_callstring(eve->parent, event_type, callstring);

	eve->cfg_changed_flag = 0; // clear changed flag
	
	eve->cfg_changed_flag = 0;
	eve->cfg_default_flag = 1;
	eve->cfg_flashempty_flag = 1;

	if (event_type == GRID_UI_EVENT_INIT){

		printf("initevent\r\n");

	}
	
}


void grid_ui_nvm_store_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm){
	
    grid_nvm_ui_bulk_store_init(nvm, ui);

}

void grid_ui_nvm_load_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm){
	
	grid_nvm_ui_bulk_read_init(nvm, ui);

}

void grid_ui_nvm_clear_all_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm){
	
	
	grid_nvm_erase_all(&grid_nvm_state);
	// grid_nvm_ui_bulk_clear_init(nvm, ui);

}


uint8_t grid_ui_recall_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page, uint8_t element, enum grid_ui_event_t event_type){
	
	// need implementation

	printf("RECALL!!! \r\n");

	

	struct grid_msg message;

	grid_msg_init(&message);
	grid_msg_init_header(&message, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_POSITION, GRID_SYS_DEFAULT_ROTATION);


	uint8_t payload[GRID_PARAMETER_PACKET_maxlength] = {0};
	uint8_t payload_length = 0;
	uint32_t offset = 0;

	struct grid_ui_element* ele = &ui->element_list[element];
	uint8_t event_index = grid_ui_event_find(ele, event_type);
	struct grid_ui_event* eve = NULL;

	if (event_index != 255){

		// Event actually exists

		eve = &ele->event_list[event_index]; 

		sprintf(payload, GRID_CLASS_CONFIG_frame_start);
		payload_length = strlen(payload);

		grid_msg_body_append_text(&message, payload, payload_length);

		grid_msg_text_set_parameter(&message, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
		
		
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONMAJOR_offset, GRID_CLASS_CONFIG_VERSIONMAJOR_length, GRID_PROTOCOL_VERSION_MAJOR);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONMINOR_offset, GRID_CLASS_CONFIG_VERSIONMINOR_length, GRID_PROTOCOL_VERSION_MINOR);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_VERSIONPATCH_offset, GRID_CLASS_CONFIG_VERSIONPATCH_length, GRID_PROTOCOL_VERSION_PATCH);
				
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_PAGENUMBER_offset, GRID_CLASS_CONFIG_PAGENUMBER_length, page);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ELEMENTNUMBER_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, element);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_EVENTTYPE_offset, GRID_CLASS_CONFIG_EVENTTYPE_length, event_type);
		grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, 0);

		if (ui->page_activepage == page){
			// currently active page needs to be sent
			grid_msg_body_append_text(&message, eve->action_string, eve->action_string_length);
			grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, eve->action_string_length);
		}		
		else{

			// use nvm_toc to find the configuration to be sent

			struct grid_nvm_toc_entry* entry = NULL;
			entry = grid_nvm_toc_entry_find(&grid_nvm_state, page, element, event_type);

			if (entry != NULL){
				
				printf("FOUND %d %d %d 0x%x (+%d)!\r\n", entry->page_id, entry->element_id, entry->event_type, entry->config_string_offset, entry->config_string_length);

				uint8_t buffer[entry->config_string_length+10];

				uint32_t len = grid_nvm_toc_generate_actionstring(nvm, entry, buffer);

				// reset body pointer because cfg in nvm already has the config header
				message.body_length = 0;
				grid_msg_body_append_text(&message, buffer, len);

				grid_msg_text_set_parameter(&message, 0, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_REPORT_code);
			
			}
			else{
				printf("NOT FOUND, Send default!\r\n");
				uint8_t actionstring[GRID_UI_ACTION_STRING_maxlength] = {0};
				grid_ui_event_generate_actionstring(eve->parent->type, event_type, actionstring);	
				grid_msg_body_append_text(&message, actionstring, strlen(actionstring));
				grid_msg_text_set_parameter(&message, 0, GRID_CLASS_CONFIG_ACTIONLENGTH_offset, GRID_CLASS_CONFIG_ACTIONLENGTH_length, eve->action_string_length);

			}

			// if no toc entry is found but page exists then send efault configuration

		}
	
		sprintf(payload, GRID_CLASS_CONFIG_frame_end);
		payload_length = strlen(payload);

		grid_msg_body_append_text(&message, payload, payload_length);

		printf("CFG: %s\r\n", message.body);
		grid_msg_packet_close(&message);
		grid_msg_packet_send_everywhere(&message);
	}
	else{

		printf("warning."__FILE__".event does not exist!\r\n");
	}



	

	
}

uint8_t grid_ui_page_load(struct grid_ui_model* ui, struct grid_nvm_model* nvm, uint8_t page){

	ui->page_activepage = page;

	for (uint8_t i=0; i<ui->element_list_length; i++){

		struct grid_ui_element* ele = &ui->element_list[i];

		for (uint8_t j=0; j<ele->event_list_length; j++){

			struct grid_ui_event* eve = &ele->event_list[j];

			struct grid_nvm_toc_entry* entry = NULL;
			entry = grid_nvm_toc_entry_find(&grid_nvm_state, page, ele->index, eve->type);

			if (entry != NULL){
				
				printf("Page Load: FOUND %d %d %d 0x%x (+%d)!\r\n", entry->page_id, entry->element_id, entry->event_type, entry->config_string_offset, entry->config_string_length);

				if (entry->config_string_length){
					uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};

					grid_nvm_toc_generate_actionstring(nvm, entry, temp);
					
					//printf("%s \r\n", temp);
					grid_ui_event_register_actionstring(ele, eve->type, temp);
					
					eve->cfg_changed_flag = 0; // clear changed flag
				}
				else{
					printf("Page Load: NULL length\r\n");
				}
				

			}
			else{
				printf("Page Load: NOT FOUND, Send default!\r\n");
				grid_ui_event_generate_actionstring(eve->parent->type, eve->type, eve->action_string);

				eve->action_string_length = strlen(eve->action_string);
				eve->cfg_changed_flag = 0; // clear changed flag

			}

			grid_ui_smart_trigger_local(ui, ele->index, eve->type);

		}

	}
	
}

uint8_t grid_ui_page_store(struct grid_ui_model* ui, struct grid_nvm_model* nvm){

	for (uint8_t i=0; i<ui->element_list_length; i++){

		struct grid_ui_element* ele = &ui->element_list[i];

		for (uint8_t j=0; j<ele->event_list_length; j++){

			struct grid_ui_event* eve = &ele->event_list[j];

			if (eve->cfg_changed_flag){
				
				printf("CHANGED %d %d\r\n", i, j);
				grid_nvm_config_store(&grid_nvm_state, ele->parent->page_activepage, ele->index, eve->type, eve->action_string);

				eve->cfg_changed_flag = 0; // clear changed flag
			}




		}

	}
	
}

uint8_t grid_ui_nvm_store_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve){
	
	printf("STORE NOT IMPLEMENTED\r\n");

	// struct grid_msg message;

	// grid_msg_init(&message);
	// grid_msg_init_header(&message, GRID_SYS_LOCAL_POSITION, GRID_SYS_LOCAL_POSITION, GRID_SYS_DEFAULT_ROTATION);


	// uint8_t payload[GRID_PARAMETER_PACKET_maxlength] = {0};
	// uint8_t payload_length = 0;
	// uint32_t offset = 0;



	// // BANK ENABLED
	// offset = grid_msg_body_get_length(&message);

	// sprintf(payload, GRID_CLASS_CONFIGURATION_frame_start);
	// payload_length = strlen(payload);

	// grid_msg_body_append_text(&message, payload, payload_length);

	// grid_msg_text_set_parameter(&message, offset, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code);
	// grid_msg_text_set_parameter(&message, offset, GRID_CLASS_CONFIGURATION_BANKNUMBER_offset, GRID_CLASS_CONFIGURATION_BANKNUMBER_length, eve->parent->parent->index);
	// grid_msg_text_set_parameter(&message, offset, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_offset, GRID_CLASS_CONFIGURATION_ELEMENTNUMBER_length, eve->parent->index);
	// grid_msg_text_set_parameter(&message, offset, GRID_CLASS_CONFIGURATION_EVENTTYPE_offset, GRID_CLASS_CONFIGURATION_EVENTTYPE_length, eve->type);

	// offset = grid_msg_body_get_length(&message);
	// grid_msg_body_append_text_escaped(&message, eve->action_string, eve->action_string_length);





	// sprintf(payload, GRID_CLASS_CONFIGURATION_frame_end);
	// payload_length = strlen(payload);

	// grid_msg_body_append_text(&message, payload, payload_length);


	// grid_msg_packet_close(&message);

	// grid_nvm_clear_write_buffer(nvm);

	// uint32_t message_length = grid_msg_packet_get_length(&message);

	// if (message_length){

	// 	nvm->write_buffer_length = message_length;
	
	// 	for(uint32_t i = 0; i<message_length; i++){
		
	// 		nvm->write_buffer[i] = grid_msg_packet_send_char(&message, i);
	// 	}

	// }

	// uint32_t event_page_offset = grid_nvm_calculate_event_page_offset(nvm, eve);
	// nvm->write_target_address = GRID_NVM_LOCAL_BASE_ADDRESS + GRID_NVM_PAGE_OFFSET*event_page_offset;

	// int status = 0;
	
	
	// uint8_t debugtext[200] = {0};

	// if (eve->cfg_default_flag == 1 && eve->cfg_flashempty_flag == 0){
		
	// 	//sprintf(debugtext, "Cfg: Default B:%d E:%d Ev:%d => Page: %d Status: %d", eve->parent->parent->index, eve->parent->index, eve->index, event_page_offset, status);
	// 	flash_erase(nvm->flash, nvm->write_target_address, 1);
	// 	eve->cfg_flashempty_flag = 1;
	// 	status = 1;
	// }
	
	
	// if (eve->cfg_default_flag == 0 && eve->cfg_changed_flag == 1){
		
	// 	//sprintf(debugtext, "Cfg: Store B:%d E:%d Ev:%d => Page: %d Status: %d", eve->parent->parent->index, eve->parent->index, eve->index, event_page_offset, status);		
	// 	flash_write(nvm->flash, nvm->write_target_address, nvm->write_buffer, GRID_NVM_PAGE_SIZE);
	// 	status = 1;
	// }


	// //grid_debug_print_text(debugtext);

	// eve->cfg_changed_flag = 0;
	
	// return status;
	
}



uint8_t grid_ui_nvm_load_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve){
	

	printf("LOAD NOT IMPLEMENTED !!!! \r\n");
		
	// grid_nvm_clear_read_buffer(nvm);
	
	// uint32_t event_page_offset = grid_nvm_calculate_event_page_offset(nvm, eve);	
	// nvm->read_source_address = GRID_NVM_LOCAL_BASE_ADDRESS + GRID_NVM_PAGE_OFFSET*event_page_offset;	
	

	// int status = flash_read(nvm->flash, nvm->read_source_address, nvm->read_buffer, GRID_NVM_PAGE_SIZE);	
		
	// uint8_t copydone = 0;
	
	// uint8_t cfgfound = 0;
		
	// for (uint16_t i=0; i<GRID_NVM_PAGE_SIZE; i++){
			
			
	// 	if (copydone == 0){
				
	// 		if (nvm->read_buffer[i] == '\n'){ // END OF PACKET, copy newline character
	// 			GRID_PORT_U.rx_double_buffer[i] = nvm->read_buffer[i];
	// 			GRID_PORT_U.rx_double_buffer_status = i+1;
	// 			GRID_PORT_U.rx_double_buffer_read_start_index = 0;
	// 			copydone = 1;
				
	// 			cfgfound=2;
					
	// 		}
	// 		else if (nvm->read_buffer[i] == 255){ // UNPROGRAMMED MEMORY, lets get out of here
	// 			copydone = 1;
	// 		}
	// 		else{ // NORMAL CHARACTER, can be copied
	// 			GRID_PORT_U.rx_double_buffer[i] = nvm->read_buffer[i];
				
	// 			cfgfound=1;
	// 		}
				
				
	// 	}
			
			
	// }
	
	// return cfgfound;
	
	
}
uint8_t grid_ui_nvm_clear_event_configuration(struct grid_ui_model* ui, struct grid_nvm_model* nvm, struct grid_ui_event* eve){
		
		uint32_t event_page_offset = grid_nvm_calculate_event_page_offset(nvm, eve);
		
		

		flash_erase(nvm->flash, GRID_NVM_LOCAL_BASE_ADDRESS + GRID_NVM_PAGE_OFFSET*event_page_offset, 1);

		
		
		return 1;
		
}



void grid_ui_event_register_eventstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* event_string){
	
	uint8_t event_index = 255;
	
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}
	
	if (event_index == 255){
		grid_debug_print_text("Event Not Found");
		return; // EVENT NOT FOUND
	}
	
	struct grid_ui_event* eve = &ele->event_list[event_index];

	strcpy(eve->event_string, event_string);

	eve->event_string_length = strlen(eve->event_string);
	
	grid_ui_smart_trigger(ele->parent, ele->index, event_type);
	
	
}



void grid_ui_event_register_actionstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* action_string){
		
	uint8_t event_index = 255;
	
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}
	
	if (event_index == 255){
		grid_debug_print_text("Event Not Found");
		return; // EVENT NOT FOUND
	}


	struct grid_ui_event* eve = &ele->event_list[event_index];

	strcpy(eve->action_string, action_string);
	
	eve->action_string_length = strlen(eve->action_string);
	
	ele->event_list[event_index].cfg_changed_flag = 1;


	
	
	if (event_type == GRID_UI_EVENT_EC){

		uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};
		action_string[strlen(action_string)-3] = '\0';
		sprintf(temp, "ele[%d]."GRID_LUA_FNC_E_ACTION_ENCODERCHANGE_short" = function (a) local this = ele[%d] %s end", ele->index, ele->index, &action_string[6]);
		printf("%s \r\n", temp);
		grid_lua_dostring(&grid_lua_state, temp);
		//printf("ENCODER ACTION: %s\r\n", temp);
	}
	else if (event_type == GRID_UI_EVENT_BC){

		uint8_t temp[GRID_UI_ACTION_STRING_maxlength] = {0};
		action_string[strlen(action_string)-3] = '\0';
		sprintf(temp, "ele[%d]."GRID_LUA_FNC_E_ACTION_BUTTONCHANGE_short" = function (a) local this = ele[%d] %s end", ele->index, ele->index, &action_string[6]);
		grid_lua_dostring(&grid_lua_state, temp);
		//printf("BUTTON ACTION: %s\r\n", temp);
	}

	grid_lua_debug_memory_stats(&grid_lua_state, "R.A.S.");
	lua_gc(grid_lua_state.L, LUA_GCCOLLECT);
	
}



void grid_ui_event_register_callstring(struct grid_ui_element* ele, enum grid_ui_event_t event_type, uint8_t* call_string){
		
	uint8_t event_index = 255;
	
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}
	
	if (event_index == 255){
		grid_debug_print_text("Event Not Found");
		return; // EVENT NOT FOUND
	}

	struct grid_ui_event* eve = &ele->event_list[event_index];

	strcpy(eve->action_call, call_string);
	
	
	
}

void grid_ui_event_generate_eventstring(enum grid_ui_element_t element_type, enum grid_ui_event_t event_type, uint8_t* targetstring){
	
	
	if (element_type == GRID_UI_ELEMENT_BUTTON){

		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_EVENTSTRING_INIT_BUT);	break;
			case GRID_UI_EVENT_BC:		sprintf(targetstring, GRID_EVENTSTRING_BC);			break;
		}		
		
	}
	else if (element_type== GRID_UI_ELEMENT_POTENTIOMETER){
		
		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_EVENTSTRING_INIT_POT);	break;
			case GRID_UI_EVENT_AC:		sprintf(targetstring, GRID_EVENTSTRING_AC);			break;
		}			

		
	}
	// else if (element_type == GRID_UI_ELEMENT_ENCODER){

	// 	switch(event_type){
	// 		case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_EVENTSTRING_INIT_ENC);	break;
	// 		case GRID_UI_EVENT_EC:		sprintf(targetstring, GRID_EVENTSTRING_EC);			break;
	// 		case GRID_UI_EVENT_BC:		sprintf(targetstring, GRID_EVENTSTRING_BC);			break;
	// 	}	
			
	// }
	
}




void grid_ui_event_generate_actionstring(enum grid_ui_element_t element_type, enum grid_ui_event_t event_type, uint8_t* targetstring){
	

	if (element_type == GRID_UI_ELEMENT_BUTTON){
				
		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_BC:		sprintf(targetstring, GRID_ACTIONSTRING_BC);			break;
		}
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_AC:		sprintf(targetstring, GRID_ACTIONSTRING_AC);		break;
		}
		
	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		switch(event_type){
			case GRID_UI_EVENT_INIT:        sprintf(targetstring, GRID_ACTIONSTRING_INIT_ENC);	break;
			case GRID_UI_EVENT_EC:        	sprintf(targetstring, GRID_ACTIONSTRING_EC);	break;
			case GRID_UI_EVENT_BC:			sprintf(targetstring, GRID_ACTIONSTRING_BC);			break;
		}
			
	}
	
	
}

void grid_ui_event_generate_callstring(enum grid_ui_element_t element_type, enum grid_ui_event_t event_type, uint8_t* targetstring, uint8_t element_number){


	if (element_type == GRID_UI_ELEMENT_BUTTON){
				
		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_BC:		sprintf(targetstring, GRID_ACTIONSTRING_BC);			break;
		}
		
	}
	else if (element_type == GRID_UI_ELEMENT_POTENTIOMETER){
		
		switch(event_type){
			case GRID_UI_EVENT_INIT:	sprintf(targetstring, GRID_ACTIONSTRING_INIT_BUT);		break;
			case GRID_UI_EVENT_AC:		sprintf(targetstring, GRID_ACTIONSTRING_AC);		break;
		}
		
	}
	else if (element_type == GRID_UI_ELEMENT_ENCODER){
		
		switch(event_type){
			case GRID_UI_EVENT_INIT:        sprintf(targetstring, GRID_ACTIONSTRING_INIT_ENC);	break;
			case GRID_UI_EVENT_EC:        	sprintf(targetstring, "ele[%d]."GRID_LUA_FNC_E_ACTION_ENCODERCHANGE_short"()", element_number);	break;
			case GRID_UI_EVENT_BC:			sprintf(targetstring, "ele[%d]."GRID_LUA_FNC_E_ACTION_BUTTONCHANGE_short"()", element_number);	break;
		}
			
	}
	
	printf("TARGETSTRING: %s \r\n", targetstring);
	
}


uint8_t grid_ui_event_find(struct grid_ui_element* ele, enum grid_ui_event_t event_type){

	uint8_t event_index = 255;
		
	for(uint8_t i=0; i<ele->event_list_length; i++){
		if (ele->event_list[i].type == event_type){
			event_index = i;
		}
	}

		
		
	return event_index;
	
}

void grid_ui_event_trigger(struct grid_ui_element* ele, uint8_t event_index){

	if (event_index == 255){
		
		return;
	}
	
	struct grid_ui_event* eve = &ele->event_list[event_index];


		
	eve->trigger = GRID_UI_STATUS_TRIGGERED;

}

void grid_ui_event_trigger_local(struct grid_ui_element* ele, uint8_t event_index){

	if (event_index == 255){
		
		return;
	}
	
	struct grid_ui_event* eve = &ele->event_list[event_index];


		
	eve->trigger = GRID_UI_STATUS_TRIGGERED_LOCAL;

}


void grid_ui_smart_trigger(struct grid_ui_model* mod, uint8_t element, enum grid_ui_event_t event){

	uint8_t event_index = grid_ui_event_find(&mod->element_list[element], event);
	
	if (event_index == 255){
		
		return;
	}
	
	grid_ui_event_trigger(&mod->element_list[element], event_index);

}


void grid_ui_smart_trigger_local(struct grid_ui_model* mod, uint8_t element, enum grid_ui_event_t event){

	uint8_t event_index = grid_ui_event_find(&mod->element_list[element], event);
	
	if (event_index == 255){
		
		return;
	}

    grid_ui_event_trigger_local(&mod->element_list[element], event_index);
    
}


void grid_ui_event_reset(struct grid_ui_event* eve){
	
	eve->trigger = GRID_UI_STATUS_READY;
}

uint8_t grid_ui_event_istriggered(struct grid_ui_event* eve){
		
		
	if (eve->trigger == GRID_UI_STATUS_TRIGGERED){
		
					
		return 1;
				
	}
	else{
		
		return 0;
	}
			
}


uint8_t grid_ui_event_istriggered_local(struct grid_ui_event* eve){
		
		
	if (eve->trigger == GRID_UI_STATUS_TRIGGERED_LOCAL){
		
					
		return 1;
				
	}
	else{
		
		return 0;
	}
			
}

uint32_t grid_ui_event_render_event(struct grid_ui_event* eve, uint8_t* target_string){

	uint8_t page = eve->parent->parent->page_activepage;
	uint8_t element = eve->parent->index;
	uint8_t event = eve->type;
	uint8_t param = 7;

	sprintf(target_string, GRID_CLASS_EVENT_frame);

	grid_msg_set_parameter(target_string, GRID_INSTR_offset, GRID_INSTR_length, GRID_INSTR_EXECUTE_code, NULL);

	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_PAGENUMBER_offset, GRID_CLASS_EVENT_PAGENUMBER_length, page, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_ELEMENTNUMBER_offset, GRID_CLASS_EVENT_ELEMENTNUMBER_length, element, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_EVENTTYPE_offset, GRID_CLASS_EVENT_EVENTTYPE_length, event, NULL);
	grid_msg_set_parameter(target_string, GRID_CLASS_EVENT_EVENTPARAM_offset, GRID_CLASS_EVENT_EVENTPARAM_length, param, NULL);

	return strlen(target_string);

}

uint32_t grid_ui_event_render_action(struct grid_ui_event* eve, uint8_t* target_string){

	
	uint8_t temp[500] = {0};

	uint32_t i=0;
	


	// copy action string
	for(true; i<eve->action_string_length; i++){
		temp[i] = eve->action_string[i];

	}

	// new php style implementation
	uint32_t code_start = 0;
	uint32_t code_end = 0;
	uint32_t code_length = 0;
	uint32_t code_type = 0;  // 0: nocode, 1: expr, 2: lua



	uint32_t total_substituted_length = 0;



	for(i=0; i<eve->action_string_length ; i++){

		target_string[i-total_substituted_length] = temp[i];



		if (0 == strncmp(&temp[i], "<?expr ", 7)){

			//printf("<?expr \r\n");
			code_start = i;
			code_end = i;
			code_type = 1; // 1=expr


		}
		else if (0 == strncmp(&temp[i], "<?lua ", 6)){

			//printf("<?lua \r\n");
			code_start = i;
			code_end = i;
			code_type = 2; // 2=lua


		}
		else if (0 == strncmp(&temp[i], " ?>", 3)){

			code_end = i + 3; // +3 because  ?>
			//printf(" ?>\r\n");

			if (code_type == 2){ //LUA
				
				temp[i] = 0; // terminating zero for lua dostring

				uint32_t cycles[5] = {0};

				cycles[0] = grid_d51_dwt_cycles_read();

				// // element index
				// uint8_t load_script[100] = {0};
				// sprintf(load_script, "grid_load_template_variables(%d)", eve->parent->index);
				// grid_lua_dostring(&grid_lua_state, load_script);

				// uint8_t str_to_do[100] = {0};
				// sprintf(str_to_do, "this = "GRID_LUA_KW_ELEMENT_short"[%d]", eve->parent->index);    
				// grid_lua_dostring(&grid_lua_state, str_to_do);
							
				cycles[1] = grid_d51_dwt_cycles_read();

				if (strlen(eve->action_call)){

					//printf("Actioncall: %s\r\n", eve->action_call);
					grid_lua_dostring(&grid_lua_state, eve->action_call);

				}
				else{
					printf("error: lua but no action_call \r\n");
					grid_lua_dostring(&grid_lua_state, &temp[code_start+6]); // +6 is length of "<?lua "
					
				}







				cycles[2] = grid_d51_dwt_cycles_read();
				
				// uint8_t store_script[100] = {0};
				// sprintf(store_script, "grid_store_template_variables(%d)", eve->parent->index);
				// grid_lua_dostring(&grid_lua_state, store_script);






				uint32_t code_stdo_length = strlen(grid_lua_state.stdo);

				temp[i] = ' '; // reverting terminating zero to space

				i+= 3-1; // +3 because  ?> -1 because i++
				code_length = code_end - code_start;

				strcpy(&target_string[code_start-total_substituted_length], grid_lua_state.stdo);

				
				uint8_t errorlen = 0;

				if (strlen(grid_lua_state.stde)){

					
					printf(grid_lua_state.stde);

					uint8_t errorbuffer[100] = {0};

					sprintf(errorbuffer, GRID_CLASS_DEBUGTEXT_frame_start);
					strcat(errorbuffer, grid_lua_state.stde);
					sprintf(&errorbuffer[strlen(errorbuffer)], GRID_CLASS_DEBUGTEXT_frame_end);
					
					errorlen = strlen(errorbuffer);

					strcpy(&target_string[code_start-total_substituted_length+code_stdo_length], errorbuffer);

					grid_lua_clear_stde(&grid_lua_state);

				}


				total_substituted_length += code_length - code_stdo_length - errorlen;


				cycles[3] = grid_d51_dwt_cycles_read();



				//printf("Lua: %s \r\nTime [us]: %d %d %d\r\n", grid_lua_state.stdo, (cycles[1]-cycles[0])/120, (cycles[2]-cycles[1])/120, (cycles[3]-cycles[2])/120);
				//grid_lua_debug_memory_stats(&grid_lua_state, "Ui");
				grid_lua_clear_stdo(&grid_lua_state);

			}
			else if(code_type == 1){ // expr

				temp[i] = 0; // terminating zero for strlen
				grid_expr_set_current_event(&grid_expr_state, eve);


				uint32_t cycles[5] = {0};

				cycles[0] = grid_d51_dwt_cycles_read();

				grid_expr_evaluate(&grid_expr_state, &temp[code_start+7], strlen( &temp[code_start+7])); // -2 to not include {

				cycles[1] = grid_d51_dwt_cycles_read();

				uint32_t code_stdo_length = grid_expr_state.output_string_length;
				
				temp[i] = ' '; // reverting terminating zero to space

				i+= 3-1; // +3 because  ?> -1 because i++
				code_length = code_end - code_start;

				char* stdo = &grid_expr_state.output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-grid_expr_state.output_string_length];

				//printf("Expr: %s \r\nTime [us]: %d\r\n", stdo, (cycles[1]-cycles[0])/120);

				strcpy(&target_string[code_start-total_substituted_length], stdo);


				total_substituted_length += code_length - code_stdo_length;
		

			}

			code_type = 0;
			
		}
		


	}


	// Call the event clear callback

	if (eve->parent->event_clear_cb != NULL){

		eve->parent->event_clear_cb(eve);
	}
	
	
	return eve->action_string_length - total_substituted_length;
		
}


