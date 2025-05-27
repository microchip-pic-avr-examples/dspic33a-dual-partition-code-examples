# Lab 2 - Flash Protection Regions
This lab is designed to explore flash protection regions and their behavior in dual partition mode.

The current flash protection region configurations are diagrammed below for reference throughout the lab. These settings can also be viewed in partition1 &rarr; Source Files &rarr; config_bits.c.
![Figure 1](./images/lab2_figure1.png)<br>

## Required Software
* Serial terminal program
* MPLAB X - v6.25 or later
* XC-DSC v3.21 or later

## Required Hardware
* Curiosity Platform Development Board (EV74H48A)
* dsPIC33AK512MPS512 DIM (EV80L65A)

## Setup
1. With the board unplugged, insert the DIM into the DIM socket.
2. Connect the board to the host PC through the USB-C connector.
3. Reset example0 projects. This lab is designed to use the example0 project as the base for all of steps below. Please make sure that any prior modifications to the example from other labs have been reverted. Changes made in other labs might impact the behavior of this lab.
4. Open the config_bits.c file associated with partition1.X. This is located in MPLAB X under partition1 &rarr; Source Files &rarr; config_bits.c.
5. Navigate to the FIRT_IRT bit and set this to "ON" to enable the Immutable Root of Trust (IRT) regions.<br> 
![Figure 2](./images/lab2_figure2.png)<br>
6. Open a terminal program to the following settings: 460800 8-n-1.

## Lab Steps

### Overlapping Regions
Each flash protection region defined in config_bits.c has a partition select bit called 'FPRnCTRL_PSEL', where 'n' is the region number. This bit controls which panel the region will apply to: PANEL1, PANEL2, or BOTH. 

![Figure 3](./images/lab2_figure3.png)

When running in dual boot mode, it's possible to have overlapping flash protection regions with different FPRnCTRL_PSEL values. In the following examples, we'll explore how overlapping regions impact permissions.  

#### Part 1
The following steps will show how overlapping flash protection regions work when both regions apply to the same panel and have differing permissions. 

We'll be working with PR0 and PR3, where:

**PR0**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: Both
* Address range: 0x802000 - 0x802FFF

**PR3**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Enabled
* Panel: 2
* Address range: 0x802000 - 0x802FFF

1. Open the example0/partition1.X MPLAB X project.
2. Compile and program the example. A menu should print on the screen.<br>
![Figure 4](./images/lab2_figure4.png)<br>
3. Enter 'e' to try and erase a page in a flash protection region. 
4. Enter '3' to target PR3. 
5. Despite PR3 having writes enabled, the erase fails because PR0 overlap with PR3 and is write/erase protected, applying to both panel 1 and panel 2. Next, we'll try and make PR0 writable so we can successfully erase a page from PR3.
6. Enter 'u' to try and unlock a flash protection region. 
7. Enter '0' to target PR0. This should unlock successfully. Although PR0 is unlocked, it's still write/erase protected. To enable write/erase permissions, we can erase a page in PR0. The erase functionality requires enabling of write/erase permissions in the target region.  
8. Enter 'e' to try and erase a page in a flash protection region. 
9. Enter '0' to target PR0. This should erase successfully. PR0 should now have write/erase enabled. 
10. Enter 'e' to try and erase a page in a flash protection region. 
11. Enter '3' to try once again to target PR3. This should now erase successfully, as there are no write protections enabled in the address range covered by PR3. 

#### Part 2
The following steps demonstrate how overlapping flash protection regions work when the regions apply to different panels and have differing permissions.  

We'll be working with PR1 and PR4, where:

**PR1**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: 1
* Address range: 0x803000 - 0x803FFF

**PR4**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Enabled
* Panel: 2
* Address range: 0x803000 - 0x803FFF

1. Enter 'e' to try and erase a page in a flash protection region. 
2. Enter '4' to target PR4. This should erase successfully. PR4 overlaps with PR1, but PR1 is associated with panel 1, so it has no impact on PR4, which applies to panel 2.

### Panel Erase
The panel erase function erases the inactive panel including the UCA1 and UCA2 pages (depending on which partition is currently mapped to the inactive space). In the following examples, we'll walk through the panel erase function and how flash protection regions can impact the ability to perform a panel erase.

#### Part 1
The following steps will show the panel erase functionality and how flash protection regions can prevent a panel erase. 

We'll be working with PR0, PR5, PR6, and PR7, where:

**PR0**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: Both
* Address range: 0x802000 - 0x802FFF

**PR5**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: 2
* Address range: 0x805000 - 0x805FFF

**PR6**
* Type: IRT
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: 2
* Address range: 0x806000 - 0x807FFF

**PR7**
* Type: Firmware
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: Both
* Address range: 0x80C000 - 0x80FFFF

1. Open the example0/partition1.X MPLAB X project.
2. Compile and program the example. A menu should print on the screen.<br>
![Figure 4](./images/lab2_figure4.png)<br> 
Note in the terminal that partition 1 is active.<br> 
![Figure 5](./images/lab2_figure5.png)<br>
3. Enter 'p' to try and erase the inactive partition (partition 2). This should fail. PR5 and PR6 are write/erase protected and apply to panel 2, preventing the panel erase.
4. Open partition1 &rarr; Source Files &rarr; config_bits.c. Update PR5 and PR6 to allow for write and erase operations.<br>
![Figure 6](./images/lab2_figure6.png)<br>
![Figure 7](./images/lab2_figure7.png)<br>
5. Re-program the example. 
6. Enter 'p' to try and erase the inactive partition (partition 2). This should fail again. PR0 and PR7 are also write protected and apply to both partition 1 and partition 2.
7. Open partition1 &rarr; Source Files &rarr; config_bits.c. Enable writes for PR0 and PR7.<br>
![Figure 8](./images/lab2_figure8.png)<br>
![Figure 9](./images/lab2_figure9.png)<br>
8. Re-program the example. 
9. Enter 'p' to try and erase the inactive partition (partition 2). This should successfully erase the inactive partition. Note that the partition 2 sequence number is now all 0xF.<br>
![Figure 10](./images/lab2_figure10.png)<br>
Note that the write protections of partition 1 in PR1 and PR2 do not block panel 2 from being erased. Only flash protection regions assigned as the inactive partition or both will impact the erase.

#### Part 2
The following steps will show that the UCB flash protection settings are not erased on a panel erase. 

We'll be working with PR6, where:

**PR6**
* Type: IRT
* Read: Enabled
* Execute: Enabled
* Write: Disabled
* Panel: 2
* Address range: 0x806000 - 0x807FFF
 
1. Enter 'u' to try and unlock a flash protection region. 
2. Enter '6' to target PR6. This is an IRT region assigned to panel 2 and should fail to unlock. Unlike Firmware regions, IRT regions are locked and cannot be unlocked. Their permissions are set and cannot be updated in code. Note here that the panel erase does not erase the UCB flash protection settings. Only the inactive partition's code space and UCA1 or UCA2 (depending on which partition is currently inactive) are erased. 


At the end of your exploration, reset the example0/partition1.X and example0/partition2.X projects so that they can be used for the next labs.