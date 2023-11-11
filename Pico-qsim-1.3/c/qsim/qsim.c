#include "qsim.h"
//#include "LCD_1in14.h"
#include "LCD_1in3.h"
#include "gates.h"
#include "measure.h"
#include "img_hex.h"
#include "simulate.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include "hardware/structs/systick.h"


uint8_t circuit_width = 12;
uint8_t circuit_depth = 4;
int circuit_number = 1;
int delay_interval = 1500;

char bin_strs[16][5] = {
    "0000",
    "0001",
    "0010",
    "0011",
    "0100",
    "0101",
    "0110",
    "0111",
    "1000",
    "1001",
    "1010",
    "1011",
    "1100",
    "1101",
    "1110",
    "1111",
};

char help_gates[10][65] = {
    "- : Identity",
    "H: Hadamard",
    "X: NOT gate",
    "R: fixed Rx(-pi/4)",
    "V: sqrt(NOT) gate",
    "c: ctrl operator",
    "x: SWAP gates",
    "All gates can have",
    "c added and x gates",
    "must be paired"
};

char s_q_gates[5][1] = {
    "-",
    "H",
    "X",
    "R",
    "V",
};

char t_q_gates[2][1] = {
    "x",
    "c"	    
};

/* set address */
bool reserved_addr(uint8_t addr) {
return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void draw_help_gates(uint16_t* BlackImage) {
    Paint_Clear(WHITE);

    char buff_instr[64];
    for (int i = 0; i < 10; i++){
        sprintf(buff_instr, help_gates[i]);
        Paint_DrawString_EN(0,i*20,buff_instr,&Font16, WHITE, BLACK);
    }
    LCD_1IN3_Display(BlackImage);
    
    // wait for button presses
    uint8_t keyA = 15; 
    uint8_t keyB = 17;
    uint8_t keyX = 19;
    uint8_t keyY = 21;
    while(1)
    {
        if(DEV_Digital_Read(keyA) == 0){
            return;
        }
        if(DEV_Digital_Read(keyB) == 0){
            return;
        }
        if(DEV_Digital_Read(keyX) == 0){
            return;
        }
        if(DEV_Digital_Read(keyY) == 0){
            return;
        }
    }
}

void draw_circuit(char circuit[14][4][8], uint16_t* BlackImage)
{
    int down_offset = 50;
    int across_offset = 30;
    // TODO: properly center the across offset
    Paint_Clear(WHITE);
    for (int i = 0; i < circuit_width; i++)
    {
        for (int j = 0; j < circuit_depth; j++)
        {
            Paint_DrawString_EN(i*16+across_offset, j*20+down_offset, circuit[i][j], &Font20, WHITE, BLACK);
        }
    }
    char buff_instr[64];
    sprintf(buff_instr, "Circuit Number: %i", circuit_number);
    Paint_DrawString_EN(across_offset,190,buff_instr,&Font16, WHITE, BLACK);
    //LCD_1IN14_Display(BlackImage);
    LCD_1IN3_Display(BlackImage);
}

void random_circuit(char circuit[circuit_width][circuit_depth][8]){
    for (int i = 0; i < circuit_width; i++)
    {
	if (i%2==0){

	    for (int j = 0; j < circuit_depth; j++)
	    {
		int rand_gate_index = rand() % 5;
				
		// single qubit chunk
		// strcpy(circuit[i][j], s_q_gates[rand_gate_index]);
		switch (rand_gate_index){
		    case 0:
			strcpy(circuit[i][j], "X");
			break;
		    case 1:
			strcpy(circuit[i][j], "R");
			break;
	            case 2:
		        strcpy(circuit[i][j], "H");
			break;
		    case 3:
		        strcpy(circuit[i][j], "V");
			break;
		    case 4:
		        strcpy(circuit[i][j], "-");	
			break;
		    default:
			strcpy(circuit[i][j], "-");
			break;
		
		}
	    }
	}
	else {
		int rand_2q_gate = rand() % 2;
		// 2q chunk
	    int first_index = rand() % 3;
	    int second_index = first_index + (rand() % (4 + - first_index -1 ) + 1 );
		    
		if (rand_2q_gate==0){
		    // cx gate
	            strcpy(circuit[i][first_index], "c");
		    strcpy(circuit[i][second_index], "X");
		}else{
		    strcpy(circuit[i][first_index], "x");
		    strcpy(circuit[i][second_index], "x");
		
		}
		// strcpy(circuit[i][0], "c");
		// strcpy(circuit[i][3], "X");
	}
	DEV_Delay_ms(0.5);
    }

}

void init_circuit(char circuit[circuit_width][circuit_depth][8])
{
    for (int i = 0; i < circuit_width; i++)
    {
        for (int j = 0; j < circuit_depth; j++)
        {
            strcpy(circuit[i][j], "-");
        }
    }
}

void next_gate(char* in_gate)
{
    if(strcmp(in_gate, "-") == 0){
        strcpy(in_gate, "X");
    }
    else if(strcmp(in_gate, "X") == 0){
        strcpy(in_gate, "H");
    }
    else if(strcmp(in_gate, "H") == 0){
        strcpy(in_gate, "R");
    }
    else if(strcmp(in_gate, "R") == 0){
        strcpy(in_gate, "V");
    }
    else if(strcmp(in_gate, "V") == 0){
        strcpy(in_gate, "c");
    }
    else if(strcmp(in_gate, "c") == 0){
        strcpy(in_gate, "x");
    }
    else if(strcmp(in_gate, "x") == 0){
        strcpy(in_gate, "-");
    }
    else {
        strcpy(in_gate, "A");
    }
}

void draw_results(double complex stateVec[16], UWORD* BlackImage)
{
    Paint_Clear(WHITE);
    int ctr_x = 0;
    int ctr_y = 1;
    char buff[64];
    sprintf(buff, "Output Statevector:");
    Paint_DrawString_EN(0,0,buff,&Font16, WHITE, BLACK);
    for (int i = 0; i < 16; i++)
    {
        if (stateVec[i] != 0)
        {
            char buff[64];
            sprintf(buff, " %s: %.4f ", bin_strs[i], pow(cabs(stateVec[i]),2));
            //sprintf(buff, "%s: %.3f", bin_strs[i], creal(stateVec[i]));
            Paint_DrawString_EN(ctr_x*120, ctr_y*14, buff, &Font16, BLACK, WHITE);
            ++ctr_y;
            //if (ctr_y > 0 && ctr_y % 8 == 0) // for when we go to 5+ qubits... -MC
            //{
            //    // move to next column
            //    ++ctr_x;
            //    ctr_y = 0;
            //}
        }
    }
    //LCD_1IN14_Display(BlackImage);
    LCD_1IN3_Display(BlackImage);

    /*
    char buff[80];
    sprintf(buff, "%.5f", creal(stateVec[0]));
    Paint_DrawString_EN(1, 100, buff, &Font12, WHITE, BLACK);
    LCD_1IN14_Display(BlackImage);
    */

    // wait for button presses
    uint8_t keyA = 15; 
    uint8_t keyB = 17;
    uint8_t keyX = 19;
    uint8_t keyY = 21;
    DEV_Delay_ms(delay_interval);
    return;
}

void draw_sim_results(uint8_t res[4], uint32_t time_diff, UWORD* BlackImage)
{
    Paint_Clear(WHITE);
    int ctr_x = 0;
    int ctr_y = 1;
    char buff[64];
    sprintf(buff, "Simulation Outputs:");
    Paint_DrawString_EN(0,0,buff,&Font16, WHITE, BLACK);
    for (int i = 0; i < 8; i++)
    {
        char buff[64];
        uint8_t r = res[i] & 0xF; // just the last 4 bits, just in case... -MC
        sprintf(buff, " Simulation %i: %s ", i, bin_strs[r]);
        Paint_DrawString_EN(ctr_x*120, ctr_y*14, buff, &Font16, BLACK, WHITE);
        ++ctr_y;
    }
    sprintf(buff, "Exec Time: %i cycles", time_diff);
    Paint_DrawString_EN(ctr_x*120, (ctr_y+1)*14, buff, &Font16, WHITE, BLACK);
    //LCD_1IN14_Display(BlackImage);
    LCD_1IN3_Display(BlackImage);

    // wait for button presses
    uint8_t keyA = 15; 
    uint8_t keyB = 17;
    uint8_t keyX = 19;
    uint8_t keyY = 21;
    while(1)
    {
        if(DEV_Digital_Read(keyA) == 0){
            return;
        }
        if(DEV_Digital_Read(keyB) == 0){
            return;
        }
        if(DEV_Digital_Read(keyX) == 0){
            return;
        }
        if(DEV_Digital_Read(keyY) == 0){
            return;
        }

    }
    return;
}

clock_t clock()
{
    return (clock_t) time_us_64() / 10000;
}

int qsim(void)
{
    // let's get busy...
    stdio_init_all();
    // have a delay, let the screen come online
    DEV_Delay_ms(100);
    //printf("LCD_1in3_test Demo\r\n");
    if(DEV_Module_Init()!=0){
        return -1;
    }
    DEV_SET_PWM(50);
    /* LCD Init */
    LCD_1IN3_Init(HORIZONTAL);
    LCD_1IN3_Clear(WHITE);
    
    //LCD_SetBacklight(1023);
    //UDOUBLE Imagesize = LCD_1IN14_HEIGHT*LCD_1IN14_WIDTH*2;
    UDOUBLE Imagesize = LCD_1IN3_HEIGHT*LCD_1IN3_WIDTH*2;
    UWORD *BlackImage;
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    //Paint_NewImage((UBYTE *)BlackImage,LCD_1IN14.WIDTH,LCD_1IN14.HEIGHT, 0, WHITE);
    Paint_NewImage((UBYTE *)BlackImage,LCD_1IN3.WIDTH,LCD_1IN3.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(WHITE);

    Paint_DrawImage(img_buf,0,0,240,240);
    LCD_1IN3_Display(BlackImage);
    DEV_Delay_ms(2000);

    //Paint_DrawCircle(130, 20, 15, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    //Paint_DrawNum (50, 40 ,9.87654321, &Font20,3,  WHITE,  BLACK);
    Paint_DrawString_EN(1, 1, "Welcome to", &Font20, WHITE, BLACK);
    Paint_DrawString_EN(20, 20, "QSim v0.1", &Font20, WHITE, BLACK);

    //LCD_1IN14_Display(BlackImage);
    LCD_1IN3_Display(BlackImage);
    DEV_Delay_ms(2000);
    DEV_SET_PWM(20); // dim the lights

    uint8_t keyA = 15; 
    uint8_t keyB = 17; 
    uint8_t keyX = 19;
    uint8_t keyY = 21;

    uint8_t up = 2;
	uint8_t dowm = 18;
	uint8_t left = 16;
	uint8_t right = 20;
	uint8_t ctrl = 3;
   

    SET_Infrared_PIN(keyA);    
    SET_Infrared_PIN(keyB);
    SET_Infrared_PIN(keyX);
    SET_Infrared_PIN(keyY);
		 
	SET_Infrared_PIN(up);
    SET_Infrared_PIN(dowm);
    SET_Infrared_PIN(left);
    SET_Infrared_PIN(right);
    SET_Infrared_PIN(ctrl);
    
    Paint_Clear(WHITE);

    // setup cursor variables
    int cursor_x = 0;
    int cursor_y = 0;

    // init circuit and state
    double complex stateVec[16] = {0};
    stateVec[0] = 1 + 0 * I;
    char circuit[14][4][8];

    // init circuit
    init_circuit(circuit);

    // setup entanglement circuit
    //strcpy(circuit[0][0], "H");
    //strcpy(circuit[1][0], "c");
    //strcpy(circuit[1][1], "X");

    // setup entanglement circuit
    strcpy(circuit[0][0], "H");
    strcpy(circuit[1][0], "c");
    strcpy(circuit[1][1], "X");
    strcpy(circuit[2][1], "c");
    strcpy(circuit[2][2], "X");
    strcpy(circuit[3][2], "c");
    strcpy(circuit[3][3], "X");

    while(1)
    {
	random_circuit(circuit);
        DEV_Delay_ms(1000);

	measure(circuit, stateVec);
	draw_results(stateVec, BlackImage);
	circuit_number += 1;


	init_circuit(circuit);
	random_circuit(circuit);
        draw_circuit(circuit, BlackImage);

        //LCD_1IN14_Display(BlackImage);
        LCD_1IN3_Display(BlackImage);
	DEV_Delay_ms(delay_interval);

    }

    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
    DEV_Module_Exit();
    return 0;
}
