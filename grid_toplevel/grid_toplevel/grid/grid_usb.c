/*
 * grid_usb.c
 *
 * Created: 7/6/2020 12:07:54 PM
 *  Author: suku
 */ 

#include "grid_usb.h"
#include "../usb/class/midi/device/audiodf_midi.h"

static bool     grid_usb_serial_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	//grid_sys_alert_set_alert(&grid_sys_state, 255,255,0,2,300);
//	cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);
	
	//cdcdf_acm_write(cdcdf_demo_buf, count); /* Echo data */
	return false;                           /* No error. */
}
static bool grid_usb_serial_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	
	//grid_sys_alert_set_alert(&grid_sys_state, 255,0,255,2,300);

//	cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS); /* Another read */
	return false;                                                                                 /* No error. */
}
static bool grid_usb_serial_statechange_cb(usb_cdc_control_signal_t state)
{
	
	//grid_sys_alert_set_alert(&grid_sys_state, 0,255,255,2,300);
	
	if (state.rs232.DTR || 1) {
		/* After connection the R/W callbacks can be registered */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)grid_usb_serial_bulkout_cb);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)grid_usb_serial_bulkin_cb);
		/* Start Rx */
		//cdcdf_acm_read((uint8_t *)cdcdf_demo_buf, CONF_USB_COMPOSITE_CDC_ACM_DATA_BULKIN_MAXPKSZ_HS);
	}
	return false; /* No error. */
}
void grid_usb_serial_init()
{
	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)grid_usb_serial_statechange_cb);
}



static bool grid_usb_midi_bulkout_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	grid_sys_alert_set_alert(&grid_sys_state, 255,255,0,2,300);
	return false;
}
static bool grid_usb_midi_bulkin_cb(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	
	grid_sys_alert_set_alert(&grid_sys_state, 255,0,255,2,300);
	return false;
}




void grid_usb_midi_init()
{
	
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_READ, (FUNC_PTR)grid_usb_midi_bulkout_cb);
	audiodf_midi_register_callback(AUDIODF_MIDI_CB_WRITE, (FUNC_PTR)grid_usb_midi_bulkin_cb);


}

void grid_keyboard_init(struct grid_keyboard_model* kb){
	
	for (uint8_t i=0; i<GRID_KEYBOARD_KEY_maxcount; i++)
	{
		kb->hid_key_array[i].b_modifier = false;
		kb->hid_key_array[i].key_id = 255;
		kb->hid_key_array[i].state = HID_KB_KEY_UP;
		
		
		kb->key_list[i].ismodifier = 0;
		kb->key_list[i].ispressed = 0;
		kb->key_list[i].keycode = 255;
		
	}
	
	kb->key_active_count = 0;
	
}

uint8_t grid_keyboard_cleanup(struct grid_keyboard_model* kb){
	
	uint8_t changed_flag = 0;
	
	// Remove all inactive (released) keys
	for(uint8_t i=0; i<kb->key_active_count; i++){
		
		if (kb->key_list[i].ispressed == false){
			
			changed_flag = 1;
			
			kb->key_list[i].ismodifier = 0;
			kb->key_list[i].ispressed = 0;
			kb->key_list[i].keycode = 255;	
					
			// Pop item, move each remaining after this forvard one index
			for (uint8_t j=i+1; j<kb->key_active_count; j++){
				
				kb->key_list[j-1] = kb->key_list[j];
				
				kb->key_list[j].ismodifier = 0;
				kb->key_list[j].ispressed = 0;
				kb->key_list[j].keycode = 255;
				
			}
			
			kb->key_active_count--;
			i--; // Retest this index, because it now points to a new item
		}
		
	}
	
	if (changed_flag == 1){
			
		uint8_t debugtext[100] = {0};
		snprintf(debugtext, 99, "cound: %d | activekeys: %d, %d, %d, %d, %d, %d", kb->key_active_count, kb->key_list[0].keycode, kb->key_list[1].keycode, kb->key_list[2].keycode, kb->key_list[3].keycode, kb->key_list[4].keycode, kb->key_list[5].keycode);
		grid_debug_print_text(debugtext);
			
			
		// USB SEND
	}
	
	return changed_flag;
	
}


uint8_t grid_keyboard_keychange(struct grid_keyboard_model* kb, struct grid_keyboard_key_desc* key){
	
	uint8_t item_index = 255;
	uint8_t remove_flag = 0;
	uint8_t changed_flag = 0;
	

	grid_keyboard_cleanup(kb);
	

	for(uint8_t i=0; i<kb->key_active_count; i++){
		
		if (kb->key_list[i].keycode == key->keycode && kb->key_list[i].ismodifier == key->ismodifier){
			// key is already in the list
			item_index = i;
			
			if (kb->key_list[i].ispressed == true){
				
				if (key->ispressed == true){
					// OK nothing to do here
				}
				else{
					// Release the damn key
					kb->key_list[i].ispressed = false;
					changed_flag = 1;
				}
				
			}
			
		}
		
	}
	
	
	uint8_t print_happened = grid_keyboard_cleanup(kb);
	
	
	if (item_index == 255){
		
		// item not in list
		
		if (kb->key_active_count< GRID_KEYBOARD_KEY_maxcount){
			
			if (key->ispressed == true){
				
				kb->key_list[kb->key_active_count] = *key;
				kb->key_active_count++;
				changed_flag = 1;
				
			}
		
		}
		else{
			grid_debug_print_text("activekeys limit hit!");
		}
		
	}
	
	
	if (changed_flag == 1){
		
		uint8_t debugtext[100] = {0};
		snprintf(debugtext, 99, "cound: %d | activekeys: %d, %d, %d, %d, %d, %d", kb->key_active_count, kb->key_list[0].keycode, kb->key_list[1].keycode, kb->key_list[2].keycode, kb->key_list[3].keycode, kb->key_list[4].keycode, kb->key_list[5].keycode);	
		
		if (!print_happened){
			
			
			grid_debug_print_text(debugtext);
		}
			
		
		for(uint8_t i=0; i<GRID_KEYBOARD_KEY_maxcount; i++){
		
			kb->hid_key_array[i].b_modifier = false;
			kb->hid_key_array[i].key_id = kb->key_list[i].keycode;
			kb->hid_key_array[i].state = kb->key_list[i].ispressed;
		
		}
		
		hiddf_keyboard_keys_state_change(kb->hid_key_array, kb->key_active_count);
		
		
		// USB SEND
	}
	
}
