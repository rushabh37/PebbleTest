#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_CITY 2
#include <pebble.h>

static char context_char = 'c';
  
static void *app_context = &context_char;
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_city_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static GFont s_time_font;
static GFont s_weather_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// Store incoming information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];
static char city_layer_buffer[32];

static char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  static char s_battery_buffer[10];
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "Bat: %d", new_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  // Putting Seconds give pretty bad battery performance
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
  // Set Date
  static char date_buffer[] = "00/00";
  strftime(date_buffer, sizeof("00/00"), "%m/%d", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ARSENAL);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  // Create GFont
  // Resource ID is prefixed
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIME_FONT_34));
  
  
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 68, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);

  // Improve the layout to be more like a watchface
  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  // text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(0, 140, 144, 25));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  
  // Create City Layer
  s_city_layer = text_layer_create(GRect(0, 110, 144, 25));
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  text_layer_set_text(s_city_layer, "");
  
  // Create Date Layer
  s_date_layer = text_layer_create(GRect(60, 40, 84, 25));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // Create Battery Layer
  s_battery_layer = text_layer_create(GRect(60, 5, 84, 25));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  
  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WEATHER_FONT_SAN_20));
  text_layer_set_font(s_weather_layer, s_weather_font);
  text_layer_set_font(s_city_layer, s_weather_font);
  text_layer_set_font(s_date_layer, s_weather_font);
  text_layer_set_font(s_battery_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_city_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
  // Get the current battery level
  battery_handler(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD START");
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy weather elements
  fonts_unload_custom_font(s_weather_font);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD FINISH");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox Received!");
  if(app_context == context) {
    
    // Read first item
    Tuple *t = dict_read_first(iterator);
  
    // For all items
    // Whenever we set the text layer use copy the string to local instance. 
    // Using dictionary pointer might result in unexpected update in the value
    while(t != NULL) {
      switch(t->key) {
        case KEY_TEMPERATURE:
          snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)t->value->int32);
          break;
        case KEY_CONDITIONS:
          snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
          break;
        case KEY_CITY:
          snprintf(city_layer_buffer, sizeof(city_layer_buffer), "%s", t->value->cstring);
          text_layer_set_text(s_city_layer, city_layer_buffer);
          break;
        default:
          APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
          break;
      }
      // Look for next item
      t = dict_read_next(iterator);
    }
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  char reason_str[100];
  snprintf(reason_str, 100, "%s", translate_error(reason));
  APP_LOG(APP_LOG_LEVEL_ERROR, reason_str);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);
  
  // Register with TickTimerService
  // SECOND_UNIT uses lot of watch power hence we use MINUTE_UNIT
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  app_message_set_context(app_context);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  APP_LOG(APP_LOG_LEVEL_INFO, "App Message Open");
  app_message_open(app_message_inbox_size_maximum(), 
                   app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  APP_LOG(APP_LOG_LEVEL_INFO,"DEINIT FINISH");
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_INFO, "APP INIT COMPETED");
  app_event_loop();
  deinit();
}