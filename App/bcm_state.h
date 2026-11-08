/* Final Project Files
 * Filename: bcm_state.h
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides functions and definitions for DOOR STATE Module.
 *
 */

#ifndef BCM_STATE_H
#define BCM_STATE_H

 typedef enum
 {
     DOOR_LOCK,
     DOOR_PENDING_CONFIRMATION,
     DOOR_UNLOCK
 } doorState;

#endif // BCM_STATE_H
