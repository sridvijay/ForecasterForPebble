#include "pebble.h"

static Window *window;

static TextLayer *temperature_layer;
static char temperature[16];
//static char realFeel[16];
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static char realfeel[16];

static AppSync sync;
static uint8_t sync_buffer[32];

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_REALFEEL_KEY = 0x2
};

static uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUN,
  RESOURCE_ID_IMAGE_RAIN,
  RESOURCE_ID_IMAGE_THUNDER,
  RESOURCE_ID_IMAGE_SNOW,
  RESOURCE_ID_IMAGE_FOG,
  RESOURCE_ID_IMAGE_WIND,
  RESOURCE_ID_IMAGE_CLWIND,
  RESOURCE_ID_IMAGE_ICE,
  RESOURCE_ID_IMAGE_STRM,
  RESOURCE_ID_IMAGE_CLOUD,
  RESOURCE_ID_IMAGE_PRTCLD
};


// This usually won't need to be modified

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case WEATHER_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }

      icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      break;
    case WEATHER_REALFEEL_KEY:
      text_layer_set_text(temperature_layer, new_tuple->value->cstring);
      strcpy(realfeel, new_tuple->value->cstring);
      
      break;
  }
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    Window *window = (Window *)context; // This context defaults to the window, but may be changed with \ref window_set_click_context.
    text_layer_set_text(temperature_layer, realfeel);
}

void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context; // This context defaults to the window, but may be changed with \ref window_set_click_context.
}

void config_provider(Window *window) {
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, select_long_click_release_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  icon_layer = bitmap_layer_create(GRect(12, -5, 120, 120));
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

  temperature_layer = text_layer_create(GRect(0, 100, 144, 68));
  text_layer_set_text_color(temperature_layer, GColorWhite);
  text_layer_set_background_color(temperature_layer, GColorClear);
  text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentCenter);
  text_layer_set_text(temperature_layer, temperature);

  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 0),
    TupletCString(WEATHER_TEMPERATURE_KEY, "70\u00B0F"),
    TupletCString(WEATHER_REALFEEL_KEY, "70\u00B0F"),
  };
    
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);

  layer_add_child(window_layer, text_layer_get_layer(temperature_layer));
}

static void window_unload(Window *window) {
  app_sync_deinit(&sync);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  text_layer_destroy(temperature_layer);
  bitmap_layer_destroy(icon_layer);
}

static void init() {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  const int inbound_size = 64;
  const int outbound_size = 16;
  app_message_open(inbound_size, outbound_size);


  const bool animated = true;
  window_stack_push(window, animated);
}



static void deinit() {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
