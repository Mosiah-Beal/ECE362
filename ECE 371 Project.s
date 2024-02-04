@ CycleLED
@ Cycles USERLEDs 0-3 on and off with a 1 second delay. A pushbutton can be used to interrupt the cycling.
@ The first push will pause the cycling and turn off all/any USERLEDs. A second push will resume the cycle from were it was.
@ Registers R8 and R9 are used for debugging purposes
@ R9 can be used to simulate an interrupt generated by the pushbutton. 
@ R8 keeps track of the number of times that the INT_DIRECTOR has been entered.

@ Once the modules have been initialized, ag to deterR6 is used as a flmine whether the LEDs should be cycled on and off.
@ If R6 is set (#0x1), then all of the LEDS will be turned off and the cycling use clock cycles 'toggling' none of the LEDs on and off.
@ If R6 is clear (#0x0), then the LEDs will be turned on and off in a cyclical pattern dictated by an array called PATTERN.
@ R7 is used to for delay loops. The delay loop does not restore R7 to its original value at the end, so R7 is decremented to 0 after each call to the DELAY procedure.
 
@ R0-R3 are used to toggle the USERLEDs on and off
@ R0 is used for the value needed to toggle the USERLEDs
@ R1 is the base address of the GPIO1 module
@ R2 is the word that is read in, modified, and written to the GPIO1_OE register
@ R3 is a pointer in an array that tracks where in the pattern/cycle we are

@ Uses R0-R3, R6-R9
@ Mosiah Beal November 2022
.text
.global _start
.global INT_DIRECTOR
_start:
			@ Set up stack in SVC mode
			LDR R13, =STACK1
			ADD R13, R13, #0x1000
			CPS #0x12					@ Switch to IRQ mode
			
			@ Set up stack in IRQ mode
			LDR R13, =STACK2
			ADD R13, R13, #0x1000
			CPS #0x13					@ Switch back to SVC mode
			
			@ Turn on GPIO1 Clock
			MOV R0, #0x02				@ Value to enable clock for a GPIO module
			LDR R1, =0x44E000AC			@ Address for CM_PER_GPIO1_CLKCTRL Register
			STR R0, [R1] 				@ Write turn on value to GPIO1 Clock
			
			@ Prepare registers for initialization
			@ LDR R1, =0x4804C000			@ Base address of GPIO1
			@ #0x190					@ Offset to CLEAR_DATA_OUT register
			@ #0x194					@ Offset to SET_DATA_OUT register
			@ #0x134					@ Offset to OE register
			@ #0x14C					@ Offset to GPIO1_FALLING_DETECT register
			@ #0x34						@ Offset to IRQSTATUS_SET_0 register
			@ #0x2C						@ Offset to IRQSTATUS_0 register
			@ #0x150					@ Offset to DEBOUNCENABLE register
			@ #0x154					@ Offset to DEBOUNCINGTIME register
			
			@ Detect falling edge on GPIO1_3 and enable to assert POINTRPEND1
			LDR R1, =0x4804C000			@ Base address of GPIO1
			MOV R2, #0x00000008			@ Value for bit 3
			LDR R3, [R1, #0x14C]		@ Read FALLING_DETECT register
			ORR R3, R3, R2				@ Modify so bit 3 is set
			STR R3, [R1, #0x14C]		@ Write back to FALLING_DETECT register
			STR R2, [R1, #0x150]		@ Write to GPIO1_IRQSTATUS_SET_0 register 
										@ to enable GPIO1_3 request on POINTRPEND1 
			
			@ Try to enable debouncing of GPIO1_3 (Didn't work, leaving as evidence/trail)
			@MOV R2, #0x00000008		@ Value for bit 3			
			@STR R2, [R1, #0x34]		@ Write to GPIO1_DEBOUNCENABLE register so bit 3 is debounced
			
			@MOV R2, #0xF				@ Debounce time of 16*31 microseconds ~ 0.5ms
			@STR R2, [R1, #0x154]		@ Write to GPIO1_DEBOUNCTIME register so bit 3 is debounced
			
			@ Initialize INTC
			LDR R1, =0x482000E8			@ Address of INTC_MIR_CLEAR3 register
			MOV R2, #0x04				@ Value to unmask INTC INT 98, GPIOINT1A
			STR R2, [R1]				@ Write to INTC_MIR_CLEAR3 register
			
			@ Ensure that processor IRQ enabled in CPSR
			MRS R3, CPSR				@ Copy CPSR to R3
			BIC R3, #0x80				@ Clear bit 7
			MSR CPSR_c, R3				@ Write back to CPSR
			
			@ Initialize pause flag to clear	(Start with cycling through the pattern)
			MOV R6, #0x0				@ Flag for button service handler
			
			@ Load registers for PatternLooping
			LDR R1, =0x4804C000			@ Base address of GPIO1
			MOV R2, #28					@ Difference in address between the beginning of the pattern and the end:
										@ Immediate number = (4 * (length of array) - 1), this is used to cycle through the pattern forever 
			LDR R3, =PATTERN			@ Load R3 with the address for an array containing the pattern of LEDS to turn on/off
			
			MOV R8, #0				@ Value representing number of times INT_DIRECTOR has been entered
			B WAIT_LOOP				@ Load pattern to cycle through infinitely
			NOP
			
			
WAIT_LOOP:
			NOP
			TST R9, #1						@ Simulate IRQ interrupt generated by button press if R9 is a 1
			BNE INT_DIRECTOR
			
@ Label used for a simulated button press.
RETURN:		
			BL GET_LED						@ Get next led depending on current state of flag
			BL TOGGLE						@ Toggle the LED(s) on, wait 1 second, then toggle back off
			B WAIT_LOOP						@ Check what LED to toggle next

GET_LED:
			TST R6, #1						@ Is the flag set? (Should the LEDS be off?)
			MOVNE R0, #0x0					@ Yes, then set R0 to all leds off
			MOVNE PC, LR					@ Go turn off
			
			LDR R0, [R3]					@ If flag was clear, get the next LED from the pattern
			CMP R0, #0						@ Is this the end of the pattern?
			
			SUBEQ R3, R3, R2				@ If so, decrement R3 by 28 so it points to the first value in the pattern
			ADDNE R3, R3, #4				@ If not, increment R3 so it points to the next value in the pattern
			
			MOV PC, LR						@ Return to mainline procedure
			
TOGGLE:
			@ This procedure uses the set bin in R0 to enable the output of a USERLED
			@ It then uses R0 to turn on the USERLED
			@ Then delays 1 second using a delay loop
			@ Finally, turns off the LED and disables the output
			
			STMFD R13!, {R0-R2, R7, LR}		@ Save used registers	
			@ Turn on LED
			STR R0, [R1, #0x194]			@ Write value to SETDATAOUT so the output of USER LED pin is high
			LDR R2, [R1, #0x134]			@ Read the contents of OE register
			BIC R2, R2, R0					@ Modify the word so corresponding bit is a 0 (enable the output)
			STR R2, [R1, #0x134]			@ Write the modified word back into OE register (LED should turn on now)
			
			@ Wait 1 second
			MOV R7, #0x00200000			@ delay 1 second
			@MOV R7, #0x00000002		@ shorten delay for testing/stepping through program.
			BL DELAY		
			
			@ Turn off LED
			STR R0, [R1, #0x190]			@ Write value to CLEARDATAOUT so the output of USER LED pin is low
			LDR R2, [R1, #0x134]			@ Read the contents of OE register (in case it was updated since we started waiting)
			ORR R2, R2, R0					@ Modify the word so bit is a 1 (disabled output)
			STR R2, [R1, #0x134]			@ Write the modified word back into OE register
			
			LDMFD R13!, {R0-R2, R7, LR}		@ Restore used registers
			MOV PC, LR
			
DELAY:
			@ This procedure decrements R7 until it reaches 0, and it does not restore R7 to the original value
			SUBS R7, R7, #1					@ Decrement Delay Counter			
			BNE DELAY						@ Loop until R7 reaches 0
			MOV PC, LR						@ Return to mainline procedure

			
INT_DIRECTOR:
			STMFD SP!, {R0-R3, LR}			@ Push registers onto stack
			ADD R8, R8, #1					@ Indicate that we entered the Int_Director
			LDR R0, =0x482000F8				@ Load address of INTC_PENDING_IRQ3 register
			LDR R1, [R0]					@ Read PENDING IRQ3 register
			
			TST R9, #1						@ Simulate button press if R9 is a 1
			MOVNE R1, #0x00000004			@ Simulate pushing the button
			TST R1, #0x00000004				@ TEST bit 2
			BEQ PASS_ON						@ Not from GPIOINT1A, return to mainline procedure
			
			LDR R0, =0x4804C02C				@ Load address of GPIO1_IRQSTATUS_0 register
			LDR R1, [R0]					@ Read STATUS register
			
			TST R9, #1						@ Simulate button press if R9 has a 1
			MOVNE R1, #0x00000008			@ Simulate pushing the button
			TST R1, #0x00000008				@ Test bit 3
			BNE BUTTON_SERVICE				@ 	If bit 3 =1, button was pushed
			BEQ PASS_ON						@ 	If bit 3 =0, button was not pushed, return to mainline procedure

PASS_ON:
			LDMFD SP!, {R0-R3, LR}			@ Restore registers from stack
			TST R9, #1						@ Simulate button press if R9 has a 1
			BNE RETURN
			SUBS PC, LR, #4					@ Return execution to mainline procedure

BUTTON_SERVICE:
			MOV R1, #0x00000008				@ Value to turn off GPIO1_3 interrupt request
											@ Also turns off INTC interrupt request
			STR R1, [R0]					@ Write to GPIO1_IRQSTATUS_0 register
			
			@ Toggle LED
			MOV R0, #0x01000000				@ USERLED3
			LDR R1, =0x4804C000				@ Base address of GPIO1
			
			TST R6, #1						@ Is the flag set? (was the light on?)
			MOVNE R6, #0					@ Yes, then clear flag (1->0)
			BNE TURN_OFF					@ Go turn off
			
			MOVEQ R6, #1 					@ No, then set flag (0->1) - go turn on
			BEQ TURN_ON						@ Go turn on
			NOP
			
TURN_ON:
			@ Turn on LED
			@STR R0, [R1, #0x194]			@ Write value to SETDATAOUT so the output of USER LED pin is high
			@LDR R2, [R1, #0x134]			@ Read the contents of OE register
			@BIC R2, R2, R0					@ Modify the word so corresponding bit is a 0 (enable the output)
			@STR R2, [R1, #0x134]			@ Write the modified word back into OE register (LED should turn on now)
			B EXIT
				
TURN_OFF:
			@ Turn off LED
			STR R0, [R1, #0x190]			@ Write value to CLEARDATAOUT so the output of USER LED pin is low
			LDR R2, [R1, #0x134]			@ Read the contents of OE register (in case it was updated since we started waiting)
			ORR R2, R2, R0					@ Modify the word so bit is a 1 (disabled output)
			STR R2, [R1, #0x134]			@ Write the modified word back into OE register
			B EXIT					
			
			EXIT:
			@ Turn off NEWIRQA bit in INTC_CONTROL, so the processor can respond to new IRQ
			LDR R0, =0x48200048				@ Address of INTC_CONTROL register
			MOV R1, #01						@ Value to clear bit 0
			STR R1, [R0]					@ Write value to clear bit 0
			
			@ Return to mainline procedure
			LDMFD SP!, {R0-R3, LR}			@ Restore registers from stack
			TST R9, #1						@ Simulate button press if R9 has a 1
			BNE RETURN						@ Simulate returning to mainline procedure
			SUBS PC, LR, #4					@ Return from IRQ interrupt procedure
		
.data
.align 2									@ Align data with word
@ UserLED3			0x01000000
@ UserLED2			0x00800000
@ UserLED1			0x00400000
@ UserLED0			0x00200000
@ All LEDS			0x01E00000
@ LEDS 1 and 2		0x00C00000
@ END OF PATTERN 	0x0
PATTERN:  		.WORD		0x01000000, 0x00800000, 0x00400000, 0x00200000, 0x00400000, 0x00800000, 0x01000000, 0x0					@ 3, 2, 1, 0, 1, 2, 3, done

STACK1:			.rept 		256				@ Reserve 256 bytes for stack and init with 0x00
				.byte 		0x00
				.endr

STACK2:			.rept 		256				@ Reserve 256 bytes for stack and init with 0x00
				.byte 		0x00
				.endr

.END
