#include <stdio.h>
#include <thread.h>
#include <xtimer.h>
#include <periph/gpio.h>
#include <mutex.h>
#include <msg.h>

//Gloabal thread IDs since thread send messages to each other
kernel_pid_t Inputs_id;
kernel_pid_t Process_id;
kernel_pid_t Output_id;

//Global declaration of mutex, since all threads are using the same
mutex_t busy;

//Global declaration of pins from Port_A (pin 22, 24, 26 on the board)
gpio_t A = GPIO_PIN(PORT_A,0);
gpio_t B = GPIO_PIN(PORT_A,2);
gpio_t C = GPIO_PIN(PORT_A,4);

//First thread body
void * AND_Inputs(void* arg)
{
	// to avoid compiler error	
	(void)arg;
	
	//Initiasing pin A and B as input pins with pull up
	gpio_init(A,GPIO_IN_PU);
    gpio_init(B,GPIO_IN_PU);
    
    //Array which stores Low/High from input pins
	int input[2];
	
	//Declaration of messages
	msg_t msg_in, msg_out;

	while(1)
	{
		msg_receive(&msg_in);   //waits for a message
        mutex_lock (&busy);		//Mutex locks thread
		
		//input array is set true or false depending of input
		if(gpio_read(A) != 0)	
        	input[0] = 1;

		else input[0] = 0;

		if(gpio_read(B) != 0)
        	input[1] = 1;
        	
        else input[1] = 0;
		
		//content pointer of out message points to base address of array
		msg_out.content.ptr=input;

    	mutex_unlock (&busy);	//release mutex
		xtimer_usleep(1000);	//sleep for 1ms
		msg_send(&msg_out, Process_id);	//send out message to next thread
	}

}

//second thread body
void * AND_Process(void* arg)
{
	(void)arg;
	msg_t msg_in, msg_out;

	while(1)
	{
        msg_receive(&msg_in);//waits for a message
		mutex_lock (&busy);
        msg_out.content.value = *(int *)msg_in.content.ptr  & *((int *)msg_in.content.ptr+1); //compares value in array addresses
										// and stores output in next output message
        mutex_unlock (&busy);
      	xtimer_usleep(250);	//simulated processing time
	msg_send(&msg_out, Output_id);	//send output message to Output thread
	}
}

//third thread body
void * AND_Output(void* arg)
{
	(void)arg;
	msg_t msg_in;
	gpio_init(C,GPIO_OUT); //declares pin C as output pin

	while(1)
	{
		msg_receive(&msg_in);	//waits for a message 
        	mutex_lock (&busy);
        	
        //set output pin high if contnent of in pu message is true otherwise low
		if(msg_in.content.value == 1)	
			gpio_set(C);
		else
			gpio_clear(C);

        mutex_unlock (&busy);
		xtimer_usleep(1000);
	}
}

//declaration of stack sizes
char t1_stack[THREAD_STACKSIZE_MAIN];	
char t2_stack[THREAD_STACKSIZE_MAIN];
char t3_stack[THREAD_STACKSIZE_MAIN];

int main(void)
{
	msg_t msg_out;
	mutex_init(&busy); //initialize mutex
	
	//threads creation
    Inputs_id = thread_create (t1_stack, sizeof(t1_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_WOUT_YIELD,  AND_Inputs, NULL, NULL);
	Process_id = thread_create (t2_stack, sizeof(t2_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_WOUT_YIELD,  AND_Process, NULL, NULL);
	Output_id = thread_create (t3_stack, sizeof(t3_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_WOUT_YIELD,  AND_Output, NULL, NULL);

	while (1) {

        msg_send(&msg_out, Inputs_id); //sends message to input thread in a loop to wake it up
	
    	}
    	
	return 0;
}


