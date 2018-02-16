/*
 # Copyright 2017, Google, Inc.
 # Licensed under the Apache License, Version 2.0 (the `License`);
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 # 
 #    http://www.apache.org/licenses/LICENSE-2.0
 # 
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an `AS IS` BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
*/
#include "mgos.h"
#include "mgos_gpio.h"
#include "time.h"

const int LED0 = 25;
const int LED1 = 26;
const int LED2 = 32;
const int LED3 = 27;

const int BUTTON0 = 34;
const int BUTTON1 = 33;
const int BUTTON2 = 14;
const int BUTTON3 = 12;

const int TONE0 = 2272;
const int TONE1 = 1912;
const int TONE2 = 1519;
const int TONE3 = 1136;

const int RESET_BUTTON = 35;

const int BUZZER = 13;

typedef struct {
    int* array;
    size_t used;
    size_t size;
} GameButtons;

static GameButtons Moves;
static int CurrentPosition = 0;
static int GamePosition = 0;
static bool CanPushButtons = false;

void reset();
void playButton(int, int);
void playFailSequence();
void turnOffLEDs();
void playSeries(int);

int buttonForLEDPosition(int led) {
    switch(led) {
        case 0:
            return BUTTON0;
        case 1:
            return BUTTON1;
        case 2:
            return BUTTON2;
        case 3:
            return BUTTON3;
        default:
            return -1;
    }
}

void button_push(int pin, void* arg) {

    if (!CanPushButtons)
        return;

    switch(pin) {
        case (int)BUTTON0:
            playButton(LED0, 400000);
            break;
        case (int)BUTTON1:
            playButton(LED1, 400000);
            break;
        case (int)BUTTON2:
            playButton(LED2, 400000);
            break;
        case (int)BUTTON3:
            playButton(LED3, 400000);
            break;
        default:
            LOG(LL_INFO, ("A button I don't know about just got pushed."));
            break;
    }

    int ledToMatch = buttonForLEDPosition(Moves.array[CurrentPosition]);

    if (ledToMatch != pin) {

        //LOG(LL_INFO, ("Button push didn't match correct LED"));

        playFailSequence();
        CanPushButtons = false;
        return;
    }

        // if we are at the game position, it means we've played everything in our current sequence
    if (CurrentPosition == GamePosition) {

        //LOG(LL_INFO, ("We completed the sequence!"));

        CurrentPosition = 0;
        usleep(500000);
        playSeries(++GamePosition);
        return;
    }

    //LOG(LL_INFO, ("We have more sequence to complete."));

    ++CurrentPosition;
    (void) arg;
}

void addMove() {
    if (Moves.used == Moves.size) {
        Moves.size *= 2;
        Moves.array = (int*)realloc(Moves.array, Moves.size * sizeof(int));
    }

    int r = rand() % 4;
    Moves.array[Moves.used++] = r;

    //LOG(LL_INFO, ("Move %i: %i", Moves.used - 1, r));
}

void createGame() {
    Moves.array = (int*)malloc(1 * sizeof(int));
    Moves.used = 0;
    Moves.size = 1;
    addMove();
}

void setup() {
        // LEDs
    mgos_gpio_set_mode(LED0, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_set_mode(LED1, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_set_mode(LED2, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_set_mode(LED3, MGOS_GPIO_MODE_OUTPUT);

        // buzzer
    mgos_gpio_set_mode(BUZZER, MGOS_GPIO_MODE_OUTPUT);

        // buttons
    mgos_gpio_set_button_handler(BUTTON0, MGOS_GPIO_PULL_UP, 
                                 MGOS_GPIO_INT_EDGE_POS, 50, button_push,
                                 NULL);
    mgos_gpio_set_button_handler(BUTTON1, MGOS_GPIO_PULL_UP, 
                                 MGOS_GPIO_INT_EDGE_POS, 50, button_push,
                                 NULL);
    mgos_gpio_set_button_handler(BUTTON2, MGOS_GPIO_PULL_UP, 
                                 MGOS_GPIO_INT_EDGE_POS, 50, button_push,
                                 NULL);
    mgos_gpio_set_button_handler(BUTTON3, MGOS_GPIO_PULL_UP, 
                                 MGOS_GPIO_INT_EDGE_POS, 50, button_push,
                                 NULL);

    mgos_gpio_set_button_handler(RESET_BUTTON, MGOS_GPIO_PULL_UP, 
                                 MGOS_GPIO_INT_EDGE_POS, 50, reset,
                                 NULL);

        // initialize Game array
    createGame();
}

// duration is in microseconds
void playButton(int button, int duration) {
    mgos_gpio_write(button, 1);

    int frequency = 0;
    switch(button) {
        case (int)LED0:
            frequency = TONE0;
            break;
        case (int)LED1:
            frequency = TONE1;
            break;
        case (int)LED2:
            frequency = TONE2;
            break;
        case (int)LED3:
            frequency = TONE3;
            break;
    }

    int elapsed_time = 0;
    while( elapsed_time < duration) {
        mgos_gpio_write(BUZZER, 1);
        usleep(frequency/2);
        mgos_gpio_write(BUZZER, 0);
        usleep(frequency/2);
        elapsed_time += frequency;
    }
    mgos_gpio_write(button, 0);
}

void playSeries(int pos) {

    //LOG(LL_INFO, ("I'm playing sequence %i and my current used count is %i", pos, Moves.used));

    CanPushButtons =  false;
    while (pos >= Moves.used) {
        addMove();
    }
    int led;
    for (int i = 0; i <= pos; ++i) {
        switch(Moves.array[i]) {
            case 0:
                led = LED0;
                break;
            case 1:
                led = LED1;
                break;
            case 2:
                led = LED2;
                break;
            case 3:
                led = LED3;
                break;
            default:
                LOG(LL_INFO, ("We have a bad button!"));
                led = -1;
                break;
        }

        playButton(led, 750000);
        usleep(400000);
    }
    CanPushButtons = true;
}

void playStartSequence() {
    mgos_gpio_write(LED0, 1);
    usleep(200000);
    mgos_gpio_write(LED0, 0);
    mgos_gpio_write(LED1, 1);
    usleep(200000);
    mgos_gpio_write(LED1, 0);
    mgos_gpio_write(LED2, 1);
    usleep(200000);
    mgos_gpio_write(LED2, 0);
    mgos_gpio_write(LED3, 1);
    usleep(200000);
    mgos_gpio_write(LED3, 0);
    mgos_gpio_write(LED2, 1);
    usleep(200000);
    mgos_gpio_write(LED2, 0);
    mgos_gpio_write(LED1, 1);
    usleep(200000);
    mgos_gpio_write(LED1, 0);
    mgos_gpio_write(LED0, 1);
    usleep(200000);
    mgos_gpio_write(LED0, 0);
    mgos_gpio_write(LED1, 1);
    usleep(200000);
    mgos_gpio_write(LED1, 0);
    mgos_gpio_write(LED2, 1);
    usleep(200000);
    mgos_gpio_write(LED2, 0);
    mgos_gpio_write(LED3, 1);
    usleep(200000);
    mgos_gpio_write(LED3, 0);
}

void playFailSequence() {
    turnOffLEDs();
    for (int i = 0; i < 8; ++i) {
        mgos_gpio_toggle(LED0);
        mgos_gpio_toggle(LED1);
        mgos_gpio_toggle(LED2);
        mgos_gpio_toggle(LED3);
        usleep(200000);
    }
}

void turnOffLEDs() {
    mgos_gpio_write(LED0, 0);
    mgos_gpio_write(LED1, 0);
    mgos_gpio_write(LED2, 0);
    mgos_gpio_write(LED3, 0);
}

void reset() {
    free(Moves.array);
    CurrentPosition = 0;
    GamePosition = 0;
    createGame();
    playStartSequence();
    sleep(2);
    playSeries(GamePosition);
}

enum mgos_app_init_result mgos_app_init(void) {
    srand(esp_random());
    setup();

    playStartSequence();
    sleep(2);
    playSeries(0);
    return MGOS_APP_INIT_SUCCESS;
}
