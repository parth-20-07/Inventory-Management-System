#ifndef COMMUNICATION_PROTOCOLS_H
#define COMMUNICATION_PROTOCOLS_H

char periodic_data_update_request[] = ",updt,c,t,b,o,a"; // Parameter for requesting box data updates
char add_box[] = ",pair,";                               // Add new box
char calibrate_box[] = ",cali";                          // Calibrate request to box
char change_box_parameters[] = ",chng,";                 // Change box Parameters
char buzz_box[] = ",buzz";                               // Start buzzer on box
#endif