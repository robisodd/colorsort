#include <pebble.h>
#define UPDATE_MS 50 // Screen refresh rate in milliseconds

Window *window;
Layer *root_layer, *drawing_layer;
uint8_t select = 0, weight[3] = {0};
AppTimer *looper = NULL;

bool up_button_depressed = false; // Whether Pebble's   Up   button is being held
bool dn_button_depressed = false; // Whether Pebble's  Down  button is being held
void up_push_in_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = true; layer_mark_dirty(drawing_layer);}  //   UP   button was pushed in
void up_release_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = false;                                }  //   UP   button was released
void dn_push_in_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = true; layer_mark_dirty(drawing_layer);}  //  DOWN  button was pushed in
void dn_release_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = false;                                }  //  DOWN  button was released
void sl_pressed_handler(ClickRecognizerRef recognizer, void *context) {select = (select + 1) % 3;  layer_mark_dirty(drawing_layer);}  // SELECT button was clicked
void click_config_provider(void *context) {
  window_raw_click_subscribe(   BUTTON_ID_UP,     up_push_in_handler, up_release_handler, context);
  window_raw_click_subscribe(   BUTTON_ID_DOWN,   dn_push_in_handler, dn_release_handler, context);
  window_single_click_subscribe(BUTTON_ID_SELECT, sl_pressed_handler);
}

void timer_callback(void *data) {
  layer_mark_dirty(drawing_layer);                    // Schedule redraw of screen
  looper = NULL;
}

void drawing_layer_update(Layer *me, GContext *ctx) {
  uint16_t width = layer_get_frame(me).size.w;
  if(up_button_depressed) weight[select]++;  // increment number
  if(dn_button_depressed) weight[select]--;  // decrement number
  
  uint16_t brightnesstable[64];       // Convert color to brightness calculation
  uint8_t  sortedcolors[64];          // Color in order of brightness
  uint16_t sortedbrightnesstable[64]; // Brightness calculation result in order

  for(uint8_t i=0; i<64; i++) {
    sortedcolors[i] = i;
    GColor8 color = (GColor8) { .argb = i+0b11000000 };
    uint16_t brightness = color.r*weight[0] + color.g*weight[1] + color.b*weight[2];
          brightnesstable[i] = brightness;
    sortedbrightnesstable[i] = brightness;
//     APP_LOG(APP_LOG_LEVEL_INFO, "color %d = brightness %d", i, brightness);
  }
  
  // Insertion Sort:  Adapted from pseudocode found here: https://en.wikipedia.org/wiki/Insertion_sort
  for(uint8_t i=1; i<64; i++) {
    uint16_t temp = sortedbrightnesstable[i];
    uint8_t temp2 = sortedcolors[i];
    uint8_t j = i;
    while(j>0 && sortedbrightnesstable[j-1]>temp) {
      sortedbrightnesstable[j] = sortedbrightnesstable[j-1];
      sortedcolors[j] = sortedcolors[j-1];
      --j;
    }
    sortedbrightnesstable[j] = temp;
    sortedcolors[j] = temp2;
  }  // END Insertion Sort
  
  // Draw black on left, white on right
  for(uint16_t i=0; i<((width-128)/2); i++) {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, GPoint(i, 0), GPoint(i, 140));
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint((width+128)/2 + i, 0), GPoint((width+128)/2 + i, 140));
  }
  
  // Draw vertical lines
  for(uint16_t i=0; i<64; i++) {
    int16_t x = ((width/2) - 64) + (i*2);
    graphics_context_set_stroke_color(ctx, (GColor){.argb=sortedcolors[i]+0b11000000});
    graphics_draw_line(ctx, GPoint(x,   0), GPoint(x,   140));
    graphics_draw_line(ctx, GPoint(x+1, 0), GPoint(x+1, 140));
//     APP_LOG(APP_LOG_LEVEL_INFO, "sortedcolors[%d] = %d (%d)", i, sortedcolors[i], brightnesstable[sortedcolors[i]]);
    brightnesstable[0]+=0;  // To stop error of not using this variable
  }
  
  // Draw Text
  char text[30];
  graphics_context_set_text_color(ctx, GColorBlack);  // White Text
  snprintf(text, sizeof(text), "%cR:%d %cG:%d %cB:%d", select==0?'>':' ', weight[0], select==1?'>':' ',weight[1], select==2?'>':' ',weight[2]);  // What text to draw
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0,145,width,20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text

  // Schedule loop if either button is pressed (and loop isn't already scheduled)
  if(!looper && (up_button_depressed || dn_button_depressed))
    looper = app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}

void init(void) {
  window = window_create();
  root_layer=window_get_root_layer(window);
  drawing_layer = layer_create(layer_get_bounds(root_layer));
  layer_add_child(root_layer, drawing_layer);
  layer_set_update_proc(drawing_layer, drawing_layer_update);
  window_set_click_config_provider(window, click_config_provider);
  window_stack_push(window, true);
}

void deinit(void) {
  layer_destroy(drawing_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
