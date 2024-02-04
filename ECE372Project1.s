@ Mosiah Beal January 2023
@ The program will cycle through the LEDs and can start/stop the timer count in DMTIMER3_TCRR


.text
.global _start
.global INT_DIRECTOR
.EQU TIMER_DURATION, 0xFFFFC000			@ 0.5 seconds
					@0xFFFF0000			@ 2 seconds
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
			LDR R0, =0x00040002			@ Value to enable Aux Function CLK and module CLK
			LDR R1, =0x44E000AC			@ Address for CM_PER_GPIO1_CLKCTRL Register
			STR R0, [R1] 				@ Write turn on value to GPIO1 Clock and Aux Function Clock
			
			@ Initialize GPIO1 and enable debouncing for 5 ms for GPIO1_3 (and all other GPIO1 inputs)
			LDR R1, =0x4804C000			@ Base address of GPIO1
			MOV R2, #0x00000008			@ Value for bit 3
			STR R2, [R1, #0x150]		@ Write to GPIO1_DEBOUNCENABLE register
			MOV R3, #0xA0				@ Number 31 Microsecond debounce intervals-1
			STR R3, [R1, #0x154]		@ Enable GPIO1 debounce for 5 ms
			
			STR R2, [R1, #0x34]			@ Write to GPIO1_IRQSTATUS_SET_0 register 
			LDR R3, [R1, #0x14C]		@ Read FALLING_DETECT register
			ORR R3, R3, R2				@ Modify so bit 3 is set
			STR R3, [R1, #0x14C]		@ Write back to FALLING_DETECT register
										@ to enable GPIO1_3 request on POINTRPEND1 
			@ Initialize INTC
			LDR R1, =0x48200000			@ Base address for INTC register
			MOV R2, #0x2				@ Value to reset INTC
			STR R2, [R1, #0x10]			@ Write to INTC Config register
			
			MOV R2, #0x20				@ Value to unmask INTC_INT_69 Timer3 interrupt
			STR R2, [R1, #0xC8]			@ Write to INTC_MIR_CLEAR2 register

			MOV R2, #0x04				@ Value to unmask INTC INT 98, GPIOINT1A
			STR R2, [R1, #0xE8]			@ Write to INTC_MIR_CLEAR3 register
			
			
			
			@ Turn on Timer3 CLK
			LDR R1, =0x44E00084			@ Address to CM_PER_TIMER3_CLKCTRL
			MOV R2, #0x2				@ Value to enable Timer3 CLK
			STR R2, [R1]				@ Turn on Timer3
			LDR R1, =0x44E0050C			@ Address of PRCMCLKSEL_TIMER3 register
			STR R2, [R1]				@ Select 32 KHz CLK for Timer3
			
			@ Initialize Timer3 registers, with count, overflow interrupt generation
			LDR R1, =0x48042000			@ Base address for Timer3 registers
			MOV R2, #0x1				@ Value to reset Timer3
			STR R2, [R1, #0x10]			@ Write to Timer3 CFG register
			
			MOV R2, #0x2				@ Value to enable Overflow Interrupt
			STR R2, [R1, #0x2C]			@ Write to Timer3 IRQENABLE_SET
			STR R2, [R1, #0x38]			@ Write to Timer3_TCLR to autoreload on overflow
			
			LDR R2, =TIMER_DURATION		@ Count value for 2 seconds
			LDR R0, =TIMER_COUNT		@ Variable in memory which stores timer count remaining
			STR R2, [R0]				@ Initialize remaining timer count to 2 seconds
			STR R2, [R1, #0x40]			@ Timer3 TLDR load register (reload value)
			STR R2, [R1, #0x3C]			@ Write to Timer3 TCRR count register
			
			@ Initialize state of USERLEDs
			LDR R0, =0x01E00000			@ Value to manipulate all USERLEDS
			LDR R1, =0x4804C000			@ Base address of GPIO1
			STR R0, [R1, #0x190]		@ Write value to CLEARDATAOUT so all USERLEDs are off
			
			LDR R2, [R1, #0x134]		@ Read the contents of OE register
			BIC R2, R2, R0				@ Modify the word so corresponding bit is a 0 (enable the output)
			STR R2, [R1, #0x134]		@ Write value to OE register so all USERLEDs are enabled as outputs
			
			@ Initialize pause flag (Will start timer on button press)
			MOV R6, #1					@ Flag for button service handler
			MOV R8, #0					@ Value representing number of times button has been pressed
			
			@ Ensure that processor IRQ enabled in CPSR
			MRS R3, CPSR				@ Copy CPSR to R3
			BIC R3, #0x80				@ Clear bit 7
			MSR CPSR_c, R3				@ Write back to CPSR
			
			LDR R3, =PATTERN			@ Address for an array containing the pattern of LEDS to turn on/off
			B WAIT_LOOP					@ Load pattern to cycle through infinitely
			NOP	
			
WAIT_LOOP:
			NOP
			TST R9, #1						@ Simulate button press if R9 is a 1
			BNE INT_DIRECTOR
			B WAIT_LOOP						@ Check what LED to toggle next



@ PROCEDURES
INT_DIRECTOR:
			STMFD SP!, {R0-R2, R7-R8, LR}	@ Push registers onto stack
			ADD R8, R8, #1					@ Indicate that we entered the Int_Director
			LDR R0, =0x482000F8				@ Load address of INTC_PENDING_IRQ3 register
			LDR R1, [R0]					@ Read PENDING IRQ3 register
			
			TST R9, #1						@ Simulate button press if R9 is a 1
			MOVNE R1, #0x00000004			@ Simulate pushing the button
			TST R1, #0x00000004				@ TEST bit 2
			BEQ TIMER_CHECK					@ Not from GPIOINT1A, Check if from Timer3
			
			LDR R0, =0x4804C02C				@ Load address of GPIO1_IRQSTATUS_0 register
			LDR R1, [R0]					@ Read STATUS register
			
			TST R9, #1						@ Simulate button press if R9 has a 1
			MOVNE R1, #0x00000008			@ Simulate pushing the button
			TST R1, #0x00000008				@ Test bit 3
			BNE BUTTON_SERVICE				@ 	If bit 3 =1, button was pushed
			BEQ PASS_ON						@ 	If bit 3 =0, button was not pushed, return to mainline procedure

TIMER_CHECK:
			LDR R1, =0x482000D8				@ Address of INTC_PENDING_IRQ2 register
			LDR R0, [R1]					@ Read value
			TST R0, #0x20  					@ Check if interrupt from Timer3
			BEQ PASS_ON						@ No, return. Yes, check for overflow
			
			LDR R1, =0x48042028				@ Address of Timer3 IRQSTATUS register
			LDR R0, [R1]					@ Read value
			TST R0, #2						@ Check bit 1
			BNE TOGGLE						@ If overflow, go toggle LED
			BEQ PASS_ON						@ Else return to wait loop

PASS_ON:
			LDR R0, =0x48200048				@ Address of INTC_CONTROL register
			MOV R1, #01						@ Value to clear bit 0
			STR R1, [R0]					@ Write value to clear bit 0
			
			LDMFD SP!, {R0-R2, R7-R8, LR}	@ Restore registers from stack
			TST R9, #1						@ Simulate button press if R9 has a 1
			BNE WAIT_LOOP
			SUBS PC, LR, #4					@ Return execution to mainline procedure


@ Button Pressed
BUTTON_SERVICE:
			LDR R0, =0x4804C02C				@ Load address of GPIO1_IRQSTATUS_0 register
			MOV R1, #0x00000008				@ Value to turn off GPIO1_3 interrupt request
											@ Also turns off INTC interrupt request
			STR R1, [R0]					@ Write to GPIO1_IRQSTATUS_0 register
			
			@ Turn off Timer3 interrupt request and enable INTC for next IRQ
			LDR R1, =0x48042028				@ Address of Timer3 IRQSTATUS register
			MOV R2, #0x2					@ Value to reset Timer3 Overflow IRQ request
			STR R2, [R1]					@ Write
			
			@ Determine if the timer was paused
			TST R6, #1						@ Is the flag set? 
			MOVNE R6, #0					@ Yes, then clear flag (1->x)
			BNE RESUME						@ Go unpause timer
			
			TST R6, #1
			MOVEQ R6, #1					@ No, then set flag (x->1)
			BEQ PAUSE						@ Go pause timer
			
			@ Ensure there is always an exit
			B EXIT

START_CYCLE:
			LDR R1, =0x48042000				@ Base address of Timer3 register
			LDR R7, =TIMER_DURATION			@ Value for 2 seconds
			LDR R8, =TIMER_COUNT
			STR R7, [R8]					@ Store value in timer_count
				
			MOV R0, #0x03					@ Value to resume timer and autoload
			STR R0, [R1, #0x38]				@ Write to TCLR register
			B EXIT
			
@ FIXME	
RESUME:
			LDR R1, =0x48042000				@ Base address of Timer3 register
			LDR R10, [R1, #0x3C]			@ Read current timer count	@ Debugging purposes
			LDR R8, =TIMER_COUNT
			LDR R7, [R8]					@ Get saved count from variable
			STR R7, [R1, #0x3C]				@ Write saved count to TCRR
			
			MOV R2, #0x2					@ Value to enable Overflow Interrupt
			STR R2, [R1, #0x2C]				@ Write to Timer3 IRQENABLE_SET
			
			LDR R2, =TIMER_DURATION			@ Count value for 2 seconds
			STR R2, [R1, #0x40]				@ Timer3 TLDR load register (reload value)
			
			MOV R0, #0x03					@ Value to resume timer and autoload
			STR R0, [R1, #0x38]				@ Write to TCLR register
			B EXIT

@ FIXME		
PAUSE:
			@ Pause timer
			LDR R1, =0x48042000				@ Base address of Timer3 registers
			LDR R7, [R1, #0x3C]				@ Read current timer count in TCRR
			LDR R8, =TIMER_COUNT
			STR R7, [R8]			        @ Store count in timer_duration variable
			
			MOV R0, #0x1					@ Value to reset Timer3
			STR R0, [R1, #0x10]				@ Write to Timer3 CFG register
											@ This should pause the clock
			B EXIT					
			
			
			EXIT:
			@ Turn off NEWIRQA bit in INTC_CONTROL, so the processor can respond to new IRQ
			LDR R0, =0x48200048				@ Address of INTC_CONTROL register
			MOV R1, #01						@ Value to clear bit 0
			STR R1, [R0]					@ Write value to clear bit 0
			
			@ Return to mainline procedure
			LDMFD SP!, {R0-R2, R7-R8, LR}	@ Restore registers from stack
			TST R9, #1						@ Simulate button press if R9 has a 1
			MOVNE R9, #0					@ Remove the simulated button press when leaving the button service
			BNE WAIT_LOOP					@ Simulate returning to mainline procedure
			SUBS PC, LR, #4					@ Return from IRQ interrupt procedure



@ Toggle LEDS
TOGGLE:
			@ Turn off Timer3 interrupt request and enable INTC for next IRQ
			LDR R1, =0x48042028				@ Address of Timer3 IRQSTATUS register
			MOV R2, #0x2					@ Value to reset Timer3 Overflow IRQ request
			STR R2, [R1]					@ Write
			
			@ Determine if the timer is paused
			TST R6, #1						@ Are we paused right now
			
			@ Yes, return to wait loop
			BNE BACK
			
			@ No, timer is not paused
			
			@ Were the LEDs on when we entered?
			LDR R1, =0x4804C000				@ Base address of GPIO1
			LDR R0, [R1, #0x13C]			@ Read value from DATAOUT
			
			@ Mask and shift R0 so it contains only the 4 bits corresponding to the USERLEDs
			CMP R0, #0x0					@ Are all LEDs off?
			LDRNE R0, =0x01E00000			@ No, then go turn off all LEDs
			
			@ Turn off LEDs
			STRNE R0, [R1, #0x190]			@ Write value to CLEARDATAOUT
			BNE BACK						@ Leave
			
			@ LEDs were off, determine next LED to toggle
			BLEQ GET_LED			
			
GET_LED:
			
			LDR R0, [R3]					@ If the LEDs were off, get the next LED from the pattern
			CMP R0, #0x1					@ Is this the end of the pattern?
			
			SUBEQ R3, R3, #24				@ If so, decrement R3 by 28 so it points to the first value in the pattern
			LDREQ R0, [R3]					@ If it was the end of the pattern, load the new value instead
			
			ADDNE R3, R3, #4				@ If not, increment R3 so it points to the next value in the pattern
			
			@ Turn on LEDs
			LDR R1, =0x4804C000				@ Base address of GPIO1
			STR R0, [R1, #0x194]			@ Write value to SETDATAOUT
			B BACK							@ Leave		
			
			BACK:
			@ Turn off NEWIRQA bit in INTC_CONTROL, so the processor can respond to new IRQ
			LDR R0, =0x48200048				@ Address of INTC_CONTROL register
			MOV R1, #01						@ Value to clear bit 0
			STR R1, [R0]					@ Write value to clear bit 0
			
			@ Return to mainline procedure
			LDMFD SP!, {R0-R2, R7-R8, LR}	@ Restore registers from stack
			TST R9, #1						@ Simulate button press if R9 has a 1
			MOVNE R9, #0					@ Remove the simulated button press when leaving the button service
			BNE WAIT_LOOP					@ Simulate returning to mainline procedure
			SUBS PC, LR, #4					@ Return from IRQ interrupt procedure
			

.data
.align 2									@ Align data with word
@ UserLED3			0x01000000
@ UserLED2			0x00800000
@ UserLED1			0x00400000
@ UserLED0			0x00200000
@ All LEDS			0x01E00000
@ LEDS 1 and 2		0x00C00000
@ END OF PATTERN 	0x1
PATTERN:  		.WORD		0x01000000, 0x00800000, 0x00400000, 0x00200000, 0x00400000, 0x00800000, 0x1					@ 3, 2, 1, 0, 1, 2, done
TIMER_COUNT: 	.WORD		0xFFFF0000		@ Timer count (initially 2 seconds)
STACK1:			.rept 		256				@ Reserve 256 bytes for stack and init with 0x00
				.byte 		0x00
				.endr

STACK2:			.rept 		256				@ Reserve 256 bytes for stack and init with 0x00
				.byte 		0x00
				.endr

.END
