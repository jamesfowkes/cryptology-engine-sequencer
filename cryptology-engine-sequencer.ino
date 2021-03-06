#include <TaskAction.h>

////
////
static const char * SEQUENCE = "FAIC"; // <---- Set the correct sequence here
////
////

////
////
static const float TRIGGER_VOLTAGE = 0.5; // <---- Set the voltage under which the switch is ON
////
////

static const int NUMBER_OF_SWITCHES = 10;

static const int SWITCH_PINS[NUMBER_OF_SWITCHES] = {A0, A1, A2, A3, A4, A5, 3, 4, 5, 6};

static const int MP3_TRIGGER_PIN = 2;

static const int LED_PINS[] = {7, 8, 9, 10};

static const int COMPLETE_PIN = 10;

static bool STATE_1_SWITCHES[NUMBER_OF_SWITCHES] = {false, false, false, false, false, true, false, false, false, false};
static bool STATE_2_SWITCHES[NUMBER_OF_SWITCHES] = {true, false, false, false, false, true, false, false, false, false};
static bool STATE_3_SWITCHES[NUMBER_OF_SWITCHES] = {true, false, false, false, false, true, false, false, true, false};
static bool STATE_4_SWITCHES[NUMBER_OF_SWITCHES] = {true, false, true, false, false, true, false, false, true, false};

static int s_switch_debounce[] = {0,0,0,0,0,0,0,0,0,0};
static const int DEBOUNCE_MAX = 50;

static bool s_switch_states[NUMBER_OF_SWITCHES];

static const int STATE_FAILED = 5;
static int s_state = STATE_FAILED;

static void halt()
{
	Serial.println("Complete and halting.");
	while(true) {};
}

static void setup_io()
{
	int i;
	for (i=0; i< NUMBER_OF_SWITCHES; i++)
	{
		pinMode(SWITCH_PINS[i], INPUT_PULLUP);
	}

	for (i=0; i< 4; i++)
	{
		pinMode(LED_PINS[i], OUTPUT);
	}

	pinMode(MP3_TRIGGER_PIN, OUTPUT);
	pinMode(COMPLETE_PIN, OUTPUT);
	pinMode(13, OUTPUT);
}

typedef int (*STATE_TEST_FN)(bool const * const);

static bool arrays_match(bool const * const arr1, bool const * const arr2, int length=10)
{
	int i;
	bool match = true;
	for(i=0; i<length; i++)
	{
		match = match && (arr1[i] == arr2[i]);
	}
	return match;
}

static bool array_is_false(bool const * const arr, int length=10)
{
	int i;
	bool all_false = true;
	for(i=0; i<length; i++)
	{
		all_false = all_false && !arr[i];
	}
	return all_false;
}

static int at_start_test(bool const * const switch_states)
{
	return arrays_match(switch_states, STATE_1_SWITCHES) ? 1 : 0;
}

static int state1_test(bool const * const switch_states)
{
	bool stay_in_this_state = arrays_match(switch_states, STATE_1_SWITCHES);

	if (stay_in_this_state) { return 1; }

	if (arrays_match(switch_states, STATE_2_SWITCHES)) { return 2; }

	return STATE_FAILED;
}

static int state2_test(bool const * const switch_states)
{
	bool stay_in_this_state = arrays_match(switch_states, STATE_2_SWITCHES);

	if (stay_in_this_state) { return 2; }

	if (arrays_match(switch_states, STATE_3_SWITCHES)) { return 3; }

	return STATE_FAILED;
}

static int state3_test(bool const * const switch_states)
{
	bool stay_in_this_state = arrays_match(switch_states, STATE_3_SWITCHES);

	if (stay_in_this_state) { return 3; }

	if (arrays_match(switch_states, STATE_4_SWITCHES)) { return 4; }

	return STATE_FAILED;
}

static int state4_test(bool const * const switch_states)
{
	return arrays_match(switch_states, STATE_4_SWITCHES) ? 4 : STATE_FAILED;
}

static int failed_test(bool const * const switch_states)
{
	return array_is_false(switch_states) ? 0 : STATE_FAILED;
}

static STATE_TEST_FN s_state_tests[] = {
	at_start_test,
	state1_test,
	state2_test,
	state3_test,
	state4_test,
	failed_test,
};

static void handle_state(int state)
{
	switch(state)
	{
	case 0:
		digitalWrite(LED_PINS[0], LOW);
		digitalWrite(LED_PINS[1], LOW);
		digitalWrite(LED_PINS[2], LOW);
		digitalWrite(LED_PINS[3], LOW);
		digitalWrite(COMPLETE_PIN, LOW);
		digitalWrite(MP3_TRIGGER_PIN, HIGH);
		break;
	case 1:
		digitalWrite(LED_PINS[0], HIGH);
		digitalWrite(LED_PINS[1], LOW);
		digitalWrite(LED_PINS[2], LOW);
		digitalWrite(LED_PINS[3], LOW);
		digitalWrite(COMPLETE_PIN, LOW);
		digitalWrite(MP3_TRIGGER_PIN, HIGH);
		break;
	case 2:
		digitalWrite(LED_PINS[0], HIGH);
		digitalWrite(LED_PINS[1], HIGH);
		digitalWrite(LED_PINS[2], LOW);
		digitalWrite(LED_PINS[3], LOW);
		digitalWrite(COMPLETE_PIN, LOW);
		digitalWrite(MP3_TRIGGER_PIN, HIGH);
		break;
	case 3:
		digitalWrite(LED_PINS[0], HIGH);
		digitalWrite(LED_PINS[1], HIGH);
		digitalWrite(LED_PINS[2], HIGH);
		digitalWrite(LED_PINS[3], LOW);
		digitalWrite(COMPLETE_PIN, LOW);
		digitalWrite(MP3_TRIGGER_PIN, HIGH);
		break;
	case 4:
		digitalWrite(LED_PINS[0], HIGH);
		digitalWrite(LED_PINS[1], HIGH);
		digitalWrite(LED_PINS[2], HIGH);
		digitalWrite(LED_PINS[3], HIGH);
		digitalWrite(COMPLETE_PIN, HIGH);
		digitalWrite(MP3_TRIGGER_PIN, LOW);

		halt();

		break;
	case STATE_FAILED:
		digitalWrite(LED_PINS[0], LOW);
		digitalWrite(LED_PINS[1], LOW);
		digitalWrite(LED_PINS[2], LOW);
		digitalWrite(LED_PINS[3], LOW);
		digitalWrite(COMPLETE_PIN, LOW);
		digitalWrite(MP3_TRIGGER_PIN, HIGH);
		break;
	}
}

static void toggle_debug_led()
{
	static bool state = false;
	digitalWrite(13, state ? HIGH : LOW);
	state = !state;
}

static void print_states(Stream& stream, bool * states, const int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		stream.print(states[i] ? '1' : '0');
		if (i < n-1)
		{
			stream.print(", ");
		}
	}
	Serial.println("");
}

static void debug_task_fn(TaskAction * this_task)
{
	(void)this_task;

	Serial.print("Switch states: ");
	print_states(Serial, s_switch_states, NUMBER_OF_SWITCHES);
	Serial.print("Application state ");
	Serial.println(s_state);

	toggle_debug_led();
}
static TaskAction s_debug_task(debug_task_fn, 1000, INFINITE_TICKS);

static bool get_switch_state(int i)
{
	if (i <= 5)
	{
		// Treat as analog pin, needs to be <0.5V to trigger
		return analogRead(SWITCH_PINS[i]) < (TRIGGER_VOLTAGE * 1023.0f / 5.0f);
	}
	else
	{
		return digitalRead(SWITCH_PINS[i]) == LOW;
	}
}

static void debounce_switch(int i)
{
	bool state = get_switch_state(i);

	s_switch_debounce[i] += state ? 1 : -1;
	s_switch_debounce[i] = max(s_switch_debounce[i], 0);
	s_switch_debounce[i] = min(s_switch_debounce[i], DEBOUNCE_MAX);

	if (s_switch_debounce[i] == 0) { s_switch_states[i] = false; }
	if (s_switch_debounce[i] == DEBOUNCE_MAX) { s_switch_states[i] = true; }
}

static void debounce_task_fn(TaskAction * this_task)
{
	(void)this_task;
	int i;
	for (i = 0; i < NUMBER_OF_SWITCHES; i++)
	{
		debounce_switch(i);
	}
}
static TaskAction s_debounce_task(debounce_task_fn, 5, INFINITE_TICKS);

static void init_switch_states()
{
	int i;
	for (i=0; i <= DEBOUNCE_MAX; i++)
	{
		debounce_task_fn(NULL);	
	}
}

static void setup_state(int state, char const * const seq, bool s1_switches[10], bool s2_switches[10], bool s3_switches[10], bool s4_switches[10])
{
	int seq_position = 0 + (int)(seq[state] - 'A');

	switch(state)
	{
	case 0:
		s1_switches[seq_position] = true;
		// Fall through
	case 1:
		s2_switches[seq_position] = true;
		// Fall through
	case 2:
		s3_switches[seq_position] = true;
		// Fall through
	case 3:
		s4_switches[seq_position] = true;
		break;
	}

}

void setup()
{
	Serial.begin(115200);
	
	setup_io();

	setup_state(0, SEQUENCE, STATE_1_SWITCHES, STATE_2_SWITCHES, STATE_3_SWITCHES, STATE_4_SWITCHES);
	print_states(Serial, STATE_1_SWITCHES, NUMBER_OF_SWITCHES);
	setup_state(1, SEQUENCE, STATE_1_SWITCHES, STATE_2_SWITCHES, STATE_3_SWITCHES, STATE_4_SWITCHES);
	print_states(Serial, STATE_2_SWITCHES, NUMBER_OF_SWITCHES);
	setup_state(2, SEQUENCE, STATE_1_SWITCHES, STATE_2_SWITCHES, STATE_3_SWITCHES, STATE_4_SWITCHES);
	print_states(Serial, STATE_3_SWITCHES, NUMBER_OF_SWITCHES);
	setup_state(3, SEQUENCE, STATE_1_SWITCHES, STATE_2_SWITCHES, STATE_3_SWITCHES, STATE_4_SWITCHES);
	print_states(Serial, STATE_4_SWITCHES, NUMBER_OF_SWITCHES);

	init_switch_states();
}

void loop()
{
	s_debug_task.tick();
	s_debounce_task.tick();

	s_state = s_state_tests[s_state](s_switch_states);

	handle_state(s_state);
}
