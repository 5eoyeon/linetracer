#include "msp.h"
#include "Clock.h"
#include <stdio.h>
#include <stdlib.h>

uint16_t first_left;
uint16_t first_right;

uint16_t period_left;
uint16_t period_right;

uint32_t rot_cnt;

int IRsensor[8] = {0,};

typedef enum Phase1_mode {
    start,
    vertex,
    edge,
    end
} MODE;

uint16_t speed[2] = {2600, 2600};

/* INIT */
void SysTick_Init(void) {
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void LED_Init(void) {
    P2->SEL0 &= ~0x07;
    P2->SEL1 &= ~0x07;

    P2->DIR |= 0x07;
    P2->DIR |= 0x01;
}

void IR_Init(void) {
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;
    P5->DIR |= 0x08;
    P5->OUT &= ~0x08;

    P9->SEL0 &= ~0x04;
    P9->SEL1 &= ~0x04;
    P9->DIR |= 0x04;
    P9->OUT &= ~0x04;

    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;
    P7->DIR &= ~0xFF;
}

void PWM_Init34(uint16_t period, uint16_t duty3, uint16_t duty4) {
    P2->DIR |= 0xC0;
    P2->SEL0 |= 0xC0;
    P2->SEL1 &= ~0xC0;

    TIMER_A0->CCTL[0] = 0x800;
    TIMER_A0->CCR[0] = period;

    TIMER_A0->EX0 = 0x0000;

    TIMER_A0->CCTL[3] = 0x0040;
    TIMER_A0->CCR[3] = duty3;
    TIMER_A0->CCTL[4] = 0x0040;
    TIMER_A0->CCR[4] = duty4;

    TIMER_A0->CTL = 0x02F0;
}

void PWM_Duty3(uint16_t duty3) {
    TIMER_A0->CCR[3] = duty3;
}

void PWM_Duty4(uint16_t duty4) {
    TIMER_A0->CCR[4] = duty4;
}

void Motor_Init(void) {
    P3->SEL0 &= ~0xC0;
    P3->SEL1 &= ~0xC0;
    P3->DIR |= 0xC0;
    P3->OUT &= ~0xC0;

    P5->SEL0 &= ~0x30;
    P5->SEL1 &= ~0x30;
    P5->DIR |= 0x30;
    P5->OUT &= ~0x30;

    P2->SEL0 &= ~0xC0;
    P2->SEL1 &= ~0xC0;
    P2->DIR |= 0xC0;
    P2->OUT &= ~0xC0;

    PWM_Init34(15000, 0, 0);
}

void Timer_A3_capture_init()
{
    P10->SEL0 |= 0x30;
    P10->SEL1 &= ~0x30;
    P10->DIR &= ~0x30;

    TIMER_A3->CTL &= ~0x0030;
    TIMER_A3->CTL = 0x0200;

    TIMER_A3->CCTL[0] = 0x4910;
    TIMER_A3->CCTL[1] = 0x4910;
    TIMER_A3->EX0 &= ~0x0007;

    NVIC->IP[3] = (NVIC->IP[3] & 0x0000FFFF) | 0x40400000;
    NVIC->ISER[0] = 0x0000C000;
    TIMER_A3->CTL |= 0x0024;
}

void Init() {
    Clock_Init48MHz();
    LED_Init();
    Motor_Init();
    SysTick_Init();
    IR_Init();
    Timer_A3_capture_init();
}

/* CLOCK */
void SysTick_Wait1ms() {
    SysTick->LOAD = 48000;
    SysTick->VAL = 0;
    while((SysTick->CTRL & 0x00010000)==0) {};
}

void SysTick_Wait1us(int delay) {
    SysTick->LOAD = 48 * delay;
    SysTick->VAL = 0;
    while((SysTick->CTRL & 0x00010000)==0) {};
}

/* LED */
void TurnOn_led(int color) {
    if(color == 1) {
        P2->OUT |= 0x1;
    } else if(color == 2) {
        P2->OUT |= 0x2;
    } else if(color == 3) {
        P2->OUT |= 0x4;
    } else if(color == 4) {
        P2->OUT |= 0x7;
    }
}

void TurnOff_led() {
    P2->OUT &= ~0x07;
}

void Check() {
    int i;
    TurnOn_led(4);
    Clock_Delay1ms(900);
    TurnOff_led();
}

/* IR SENSOR */
void TurnOn_IRled() {
    P5->OUT |= 0x08;
    P9->OUT |= 0x04;
}

void TurnOff_IRled() {
    P5->OUT &= ~0x08;
    P9->OUT &= ~0x04;
}

void Charge() {
    P7->OUT = 0xFF;
    Clock_Delay1us(10);
}

void Read_IR_Sensor() {
    TurnOn_IRled();
    P7->DIR = 0xFF;
    Charge();
    P7->DIR = 0x00;
    Clock_Delay1ms(1);
    int i;
    for(i = 0; i < 8; i++) IRsensor[i] = P7->IN & (1 << i);
    Clock_Delay1ms(1);
    TurnOff_IRled();
}

/* MOVING */
void Move() {
    P3->OUT |= 0xC0;
    PWM_Duty3(speed[1]*1.02);
    PWM_Duty4(speed[0]);
}

void Left_Forward() {
    P5->OUT &= ~0x10;
}

void Left_Backward() {
    P5->OUT |= 0x10;
}

void Right_Forward() {
    P5->OUT &= ~0x20;
}

void Right_Backward() {
    P5->OUT |= 0x20;
}

void Move_Forward() {
    Left_Forward();
    Right_Forward();
}

void Move_Backward() {
    Left_Backward();
    Right_Backward();
}

void Rotate_Right() {
    Left_Forward();
    Right_Backward();
}

void Rotate_Left() {
    Left_Backward();
    Right_Forward();
}

void Stop() {
    P2->OUT &= ~0xC0;
    P3->OUT &= ~0xC0;
    P5->OUT &= ~0xC0;
}

/* ROTATION */
uint32_t get_left_rpm()
{
    return 2000000 / period_left;
}

void TA3_0_IRQHandler()
{
    TIMER_A3->CCTL[0] &= ~0x0001;
    period_right = TIMER_A3->CCR[0] - first_right;
    first_right = TIMER_A3->CCR[0];
}

void TA3_N_IRQHandler()
{
    TIMER_A3->CCTL[1] &= ~0x0001;
    rot_cnt++;
}

void Right_90() {
    rot_cnt = 0;
    while (1) {
        if(rot_cnt > 70*(speed[0]/1000)) {
            rot_cnt = 0;
            break;
        } else {
            Rotate_Right();
            Move();
            Clock_Delay1ms(10);
        }
    }
    while (1){
        if(rot_cnt > 10) {
            rot_cnt = 0;
            break;
        } else {
            Stop();
            return;
        }
    }
}

void Right_180() {
    rot_cnt = 0;
    while (1) {
        if(rot_cnt > 180*(speed[0]/1000)) {
            rot_cnt = 0;
            break;
        } else {
            Rotate_Right();
            Move();
            Clock_Delay1ms(10);
        }
    }
    while (1){
        if(rot_cnt > 10) {
            rot_cnt = 0;
            break;
        } else {
            Stop();
            return;
        }
    }
}

void Rotate_edge() {
    rot_cnt = 0;
    while (1) {
        if(rot_cnt > 37*(speed[0]/1000)) {
            rot_cnt = 0;
            break;
        } else {
            Rotate_Right();
            Move();
            Clock_Delay1ms(10);
        }
    }
    while (1){
        if(rot_cnt > 10) {
            rot_cnt = 0;
            break;
        } else {
            Stop();
            return;
        }
    }
}

void Rotate_To_Right_Specific_Edge(int target, int right) {
    int edge_found = 0;
    int prev = 0;

    Read_IR_Sensor();

        if(IRsensor[0] && IRsensor[1] && IRsensor[2] || right) {
            if(right) Right_90();
            else {
                edge_found++;
                prev = 1;
            }
        }

    Rotate_Left();
    while (edge_found != target) {
        Read_IR_Sensor();

        if (prev == 0 && (IRsensor[0] && IRsensor[1])) {
            edge_found++;
            prev = 1;
            Clock_Delay1ms(1);
        } else if (prev == 1 && !IRsensor[0]) {
            prev = 0;
            Clock_Delay1ms(1);
        }

        Move();
        Clock_Delay1ms(1);
    }

    Rotate_edge();
}

/* PHASE 1 */
int Detect_Vertex(MODE preMode) {
    if(preMode == start) {
        if((IRsensor[0]|| IRsensor[1] || IRsensor[2] || IRsensor[5]|| IRsensor[6] || IRsensor[7]) || (!IRsensor[3] && !IRsensor[4])) return 1;
        else return 0;
    } else if(preMode == edge) {
        if(!IRsensor[3] && !IRsensor[4]) return 1;
        else return 0;
    } else return 1;
}

MODE Find_Vertex(MODE mode) {
    int is_vertex = 0;
    if(mode == start) {
        int start_line = 1;
        while(is_vertex == 0) {
            Move_Forward();
            Move();
            Clock_Delay1ms(1);
            Read_IR_Sensor();
            if(start_line == 0) is_vertex = Detect_Vertex(mode);
            if(start_line && !(IRsensor[0] && IRsensor[1] && IRsensor[2] && IRsensor[5] && IRsensor[6] && IRsensor[7])) start_line = 0;
            Clock_Delay1ms(1);
        }

        Move_Forward();
        Move();
        Clock_Delay1ms(100);
        Stop();
        return vertex;
    } else if(mode == edge) {
        while(is_vertex == 0) {
            Read_IR_Sensor();
            is_vertex = Detect_Vertex(mode);
            Clock_Delay1ms(1);

            if(is_vertex) break;

            if(!IRsensor[3]) Rotate_Left();
            else if(!IRsensor[4]) Rotate_Right();
            else Move_Forward();

            Move();
            Clock_Delay1ms(1);
        }
        Stop();
        return vertex;
    } else return end;
}

int Detect_Edge_Cnt() {
    rot_cnt = 0;
    int cur_edge = 0;
    int prev = 0;

    Read_IR_Sensor();
    if(IRsensor[0] && IRsensor[1]) prev = 1;
    while (1) {
        if(rot_cnt > 330*(speed[0]/1000)) {
            rot_cnt = 0;
            break;
        } else {
            Read_IR_Sensor();
            if(prev == 0 && (IRsensor[0] && IRsensor[1])) {
                cur_edge++;
                prev = 1;
                Clock_Delay1ms(1);
            }
            else if(prev == 1 && !IRsensor[0]) {
                prev = 0;
                Clock_Delay1ms(1);
            }
            Rotate_Left();
            Move();
            Clock_Delay1ms(10);
        }
    }

    while (1){
        if(rot_cnt > 10) {
            rot_cnt = 0;
            break;
        } else {
            Stop();
            break;
        }
    }

    return cur_edge;
}

int total_v = 0;
int total_e = 0;

void Phase1() {
    MODE mode = start;

    while(mode != end) {
        TurnOff_led();
        mode = Find_Vertex(mode);
        Clock_Delay1ms(1);
        if(mode == vertex) {
            int cur_e = 0;
            int right = 0;
            Read_IR_Sensor();
            if(IRsensor[0] && IRsensor[1] && IRsensor[2]) right = 1;

            if(right == 0) {
                Move_Backward();
                Move();
                Clock_Delay1ms(150);
                Stop();
            }

            cur_e = Detect_Edge_Cnt();
            Stop();
            Clock_Delay1ms(1);

            if((cur_e%2 == 1) && (total_v != 0)) {
                TurnOn_led(3);
                Clock_Delay1ms(500);
                TurnOff_led();

                mode = end;
            } else {
                if(total_v == 0) {
                    if(cur_e%2 == 1) {
                        TurnOn_led(3);
                        Clock_Delay1ms(500);
                        TurnOff_led();
                    } else {
                        TurnOn_led(1);
                        Clock_Delay1ms(500);
                        TurnOff_led();
                    }
                }

                if(cur_e % 2 == 0) {
                    TurnOn_led(2);
                    Clock_Delay1ms(500);
                    TurnOff_led();
                }

                total_v += 1;
                total_e += cur_e;

                Rotate_To_Right_Specific_Edge(1, right);

                mode = edge;
                Clock_Delay1ms(1);
            }
        }
    }

    if(mode == end) {
        Rotate_To_Right_Specific_Edge(1, 0);

        while(!(IRsensor[1] && IRsensor[2] && IRsensor[3] && IRsensor[4] && IRsensor[5] && IRsensor[6])) {
            Read_IR_Sensor();

            if(!IRsensor[3]) Rotate_Left();
            else if(!IRsensor[4]) Rotate_Right();
            else Move_Forward();
            Move();
            Clock_Delay1ms(1);
        }

        Right_180();

        while(!(IRsensor[1] && IRsensor[2] && IRsensor[3] && IRsensor[4] && IRsensor[5] && IRsensor[6])) {
            Read_IR_Sensor();

            if(!IRsensor[3]) Rotate_Left();
            else if(!IRsensor[4]) Rotate_Right();
            else Move_Backward();

            Move();
            Clock_Delay1ms(1);
        }
        Stop();
        total_e -= 1;
    }

    return;
}

/* PHASE 2 */
void Phase2() {
    MODE mode = start;

    total_e /= 2;
    int visit_v = 0;
    int visit_e = 0;

    int level = 1;
    int right = 0;
    int start_line = 0;
    while(mode != end) {
        mode = Find_Vertex(mode);
        Clock_Delay1ms(1);

        if(mode == vertex) {
            right = 0;
            Read_IR_Sensor();
            if(IRsensor[0] && IRsensor[1] && IRsensor[2]) right = 1;

            if(visit_v % total_v == 0) {
                if(visit_v == total_v) level++;
                if(visit_e == total_v) start_line = 1;
                if(visit_e == total_e) {
                    start_line = 1;
                    mode = end;
                    break;
                }
            }

            visit_v++;
            visit_e++;

            Rotate_To_Right_Specific_Edge(level + start_line, right);
            start_line = 0;
            mode = edge;
        }
    }

    if(mode == end) {
        Stop();

        Read_IR_Sensor();
        while(!IRsensor[3] && !IRsensor[4]) {
            Move_Backward();
            Move();
            Clock_Delay1ms(1);
            Read_IR_Sensor();
        }

        if(IRsensor[0] && IRsensor[1] && IRsensor[2] && IRsensor[3] && IRsensor[4] && IRsensor[5] && IRsensor[6] && IRsensor[7]) Stop();
        else {
            Rotate_To_Right_Specific_Edge(1, right);
            while(IRsensor[3] && IRsensor[4]) {
                Move_Forward();
                Move();
                Clock_Delay1ms(1);
                Read_IR_Sensor();
            }
            Stop();
        }
    }

    return;
}

/* TEST */
void main(void)
{
    Init();

    Phase1();
    Check();
    Clock_Delay1ms(1000);

    speed[0] = 3600;
    speed[1] = 3600;

    Phase2();
    Check();

    return;
}
